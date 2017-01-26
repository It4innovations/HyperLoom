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

void TaskManager::add_plan(const loomplan::Plan &plan)
{
    cstate.add_plan(plan);
    distribute_work(schedule(cstate));
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
        for (TaskNode *node : p.second) {
            start_task(p.first, *node);
        }
    }
}

void TaskManager::start_task(WorkerConnection *wc, TaskNode &node)
{
    for (TaskNode *input_node : node.get_inputs()) {
        if (input_node->get_worker_status(wc) == TaskStatus::NONE) {
            WorkerConnection *owner = input_node->get_random_owner();
            assert(owner);
            owner->send_data(input_node->get_id(), wc->get_address());
            input_node->set_worker_status(wc, TaskStatus::TRANSFER);
        }
    }

    if (report) {
        report_task_start(wc, node);
    }

    wc->send_task(node);
    cstate.activate_pending_node(node, wc);
}

void TaskManager::report_task_start(WorkerConnection *wc, const TaskNode &node)
{
    auto event = std::make_unique<loomcomm::Event>();
    event->set_type(loomcomm::Event_Type_TASK_START);
    event->set_time(uv_now(server.get_loop()) - cstate.get_base_time());
    event->set_id(node.get_client_id());
    event->set_worker_id(wc->get_worker_id());
    server.report_event(std::move(event));
}

void TaskManager::report_task_end(WorkerConnection *wc, const TaskNode &node)
{
    auto event = std::make_unique<loomcomm::Event>();
    event->set_type(loomcomm::Event_Type_TASK_END);
    event->set_time(uv_now(server.get_loop()) - cstate.get_base_time());
    event->set_id(node.get_client_id());
    event->set_worker_id(wc->get_worker_id());
    server.report_event(std::move(event));
}

void TaskManager::remove_node(TaskNode &node)
{
    logger->debug("Removing node id={}", node.get_id());
    assert(node.get_nexts().size() == 0);
    loom::base::Id id = node.get_id();

    node.foreach_worker([id](WorkerConnection *wc, TaskStatus status) {
        assert(status == TaskStatus::OWNER);
        wc->remove_data(id);
    });
    cstate.remove_node(node);
}

void TaskManager::on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    TaskNode *node = cstate.get_node_ptr(id);
    if (node == nullptr) {
        if (!check_trash(id, wc, true)) {
            logger->critical("Finished unknown node id={} on worker={}", id, wc->get_address());
            abort();
        }
        if (cstate.has_pending_nodes()) {
            server.need_task_distribution();
        }
        return;
    }

    node->set_as_finished(wc, size, length);

    if (report) {
        report_task_end(wc, *node);
    }

    if (cstate.is_result(id)) {
        logger->debug("Job id={} [RESULT] finished", id);
        WorkerConnection *owner = node->get_random_owner();
        assert(owner);
        owner->send_data(id, server.get_dummy_worker().get_address());
        if (cstate.is_finished()) {
            logger->debug("Plan is finished");
        }
    } else {
        assert(!node->get_nexts().empty());
        logger->debug("Job id={} finished (size={}, length={})", id, size, length);
    }

    // Remove duplicates
    std::vector<TaskNode*> inputs = node->get_inputs();
    std::sort(inputs.begin(), inputs.end());
    inputs.erase(std::unique(inputs.begin(), inputs.end()), inputs.end());

    for (TaskNode *input_node : inputs) {
        if (input_node->next_finished(*node)) {
            remove_node(*input_node);
        }
    }

    if (node->get_nexts().size() > 0) {
        cstate.add_ready_nexts(*node);
    } else {
        remove_node(*node);
    }

    if (cstate.has_pending_nodes()) {
        server.need_task_distribution();
    }
}

void TaskManager::on_data_transferred(Id id, WorkerConnection *wc)
{
    TaskNode *node = cstate.get_node_ptr(id);
    if (node == nullptr) {
        if (!check_trash(id, wc, true)) {
            logger->critical("Transferred unknown node id={} on worker={}", id, wc->get_address());
            abort();
        }
        return;
    }
    logger->debug("Data id={} transferred to {}", id, wc->get_address());
    node->set_as_transferred(wc);
}

void TaskManager::on_task_failed(Id id, WorkerConnection *wc, const std::string &error_msg)
{
    if (check_trash(id, wc, false)) {
        if (cstate.has_pending_nodes()) {
            server.need_task_distribution();
        }
        return;
    }

    TaskNode &node = cstate.get_node(id);
    if (report) {
        report_task_end(wc, node);
    }
    server.inform_about_task_error(id, *wc, error_msg);
    node.set_as_none(wc);
    trash_all_tasks();

    if (cstate.has_pending_nodes()) {
        server.need_task_distribution();
    }
}

void TaskManager::run_task_distribution()
{
    uv_loop_t *loop = server.get_loop();

    // Update & get time
    uv_update_time(loop);
    auto start_scheduler = uv_now(loop);

    // Schedule
    auto distribute = schedule(cstate);

    // Update & get time
    uv_update_time(loop);
    auto start_distribute = uv_now(loop);

    // Distribute
    distribute_work(distribute);

    // Update & get time
    uv_update_time(loop);
    auto end = uv_now(loop);

    logger->debug("Schuduling finished: sched_time={}, dist_time={})",
                  start_distribute - start_scheduler,
                  end - start_distribute);
}

void TaskManager::trash_all_tasks()
{
    std::vector<loom::base::Id> running;
    cstate.foreach_node([&running](std::unique_ptr<TaskNode> &task) {
        if (task->has_state()) {
            task->foreach_owner([&task](WorkerConnection *wc) {
                wc->remove_data(task->get_id());
            });
            if (task->is_active()) {
                task->reset_owners();
                running.push_back(task->get_id());
            }
        }
    });

    for (auto id : running) {
        logger->debug("Trashing id={}", id);
        trash[id] = cstate.pop_node(id);
    }

    cstate.clear_all();
}

bool TaskManager::check_trash(Id id, WorkerConnection *wc, bool success)
{
    auto it = trash.find(id);
    if (it == trash.end()) {
        return false;
    }
    logger->debug("Trashed node id={} from worker={}", id, wc->get_address());
    TaskNode &node = *it->second;
    node.set_as_none(wc);

    if (!node.is_active()) {
        logger->debug("Trashed node id={} cleaned", id);
        if (success) {
            wc->remove_data(id);
        }
        trash.erase(it);
    }
    return true;
}
