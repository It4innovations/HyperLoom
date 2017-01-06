#include "taskmanager.h"
#include "server.h"

#include "libloom/compat.h"
#include "libloom/loomplan.pb.h"
#include "libloom/loomcomm.pb.h"
#include "libloom/log.h"

#include <algorithm>
#include <limits.h>

using namespace loom;
using namespace loom::base;

TaskManager::TaskManager(Server &server)
    : server(server), cstate(server), report(false)
{
}

void TaskManager::add_plan(Plan &&plan, bool report)
{
    this->report = report;
    cstate.set_plan(std::move(plan));
    distribute_work(Scheduler(cstate).schedule());
}

void TaskManager::distribute_work(const TaskDistribution &distribution)
{
    if (distribution.size() == 0) {
        return;
    }

    if (logger->level() >= spdlog::level::debug) {
        size_t count = 0;
        for (auto &p : distribution) {
            count += p.second.size();
        }
        logger->debug("Distributing {} task(s)", count);
    }

    for (auto& p : distribution) {
        for (loom::base::Id id : p.second) {
            start_task(p.first, id);
        }
    }
}

void TaskManager::start_task(WorkerConnection *wc, Id task_id)
{
    const PlanNode &node = cstate.get_node(task_id);
    for (loom::base::Id id : node.get_inputs()) {
        TaskState &state = cstate.get_state(id);
        WStatus st = state.get_worker_status(wc);
        if (st == WStatus::NONE) {
            WorkerConnection *owner = state.get_first_owner();
            assert(owner);
            owner->send_data(id, wc->get_address());
            state.set_worker_status(wc, WStatus::TRANSFER);
        }
    }

    if (report) {
        report_task_start(wc, node);
    }

    wc->send_task(node);
    cstate.set_running_task(node, wc);
}

void TaskManager::report_task_start(WorkerConnection *wc, const PlanNode &node)
{
    auto event = std::make_unique<loomcomm::Event>();
    event->set_type(loomcomm::Event_Type_TASK_START);
    event->set_time(uv_now(server.get_loop()) - cstate.get_base_time());
    event->set_id(node.get_client_id());
    event->set_worker_id(wc->get_worker_id());
    server.report_event(std::move(event));
}

void TaskManager::report_task_end(WorkerConnection *wc, const PlanNode &node)
{
    auto event = std::make_unique<loomcomm::Event>();
    event->set_type(loomcomm::Event_Type_TASK_END);
    event->set_time(uv_now(server.get_loop()) - cstate.get_base_time());
    event->set_id(node.get_client_id());
    event->set_worker_id(wc->get_worker_id());
    server.report_event(std::move(event));
}

void TaskManager::remove_state(TaskState &state)
{
    logger->debug("Removing state id={}", state.get_id());
    assert(state.get_ref_counter() == 0);
    loom::base::Id id = state.get_id();

    state.foreach_worker([id](WorkerConnection *wc, WStatus status) {
        assert(status == WStatus::OWNER);
        wc->remove_data(id);
    });
    cstate.remove_state(id);
}

void TaskManager::on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    const PlanNode &node = cstate.get_node(id);

    cstate.set_task_finished(node, size, length, wc);

    if (report) {
        report_task_end(wc, node);
    }

    if (node.is_result()) {
        logger->debug("Job id={} [RESULT] finished", id);
        TaskState &state = cstate.get_state(id);
        WorkerConnection *owner = state.get_first_owner();
        assert(owner);
        owner->send_data(id, server.get_dummy_worker().get_address());

        if (state.dec_ref_counter()) {
            remove_state(state);
        }
        if (cstate.is_finished()) {
            logger->debug("Plan is finished");
        }
    } else {
        logger->debug("Job id={} finished (size={}, length={})", id, size, length);
    }

    // Remove duplicates
    std::vector<loom::base::Id> inputs = node.get_inputs();
    std::sort(inputs.begin(), inputs.end());
    inputs.erase(std::unique(inputs.begin(), inputs.end()), inputs.end());

    for (loom::base::Id id : inputs) {
        TaskState &state = cstate.get_state(id);
        if (state.dec_ref_counter()) {
            remove_state(state);
        }
    }

    cstate.add_ready_nexts(node);

    if (cstate.has_pending_tasks()) {
        server.need_task_distribution();
    }
}

void TaskManager::on_data_transfered(Id id, WorkerConnection *wc)
{
    logger->debug("Data id={} transfered to {}", id, wc->get_address());
    cstate.set_data_transfered(id, wc);
}

void TaskManager::register_worker(WorkerConnection *wc)
{
    cstate.add_worker(wc);
}

void TaskManager::run_task_distribution()
{
    distribute_work(Scheduler(cstate).schedule());
}
