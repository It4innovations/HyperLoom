#include "taskmanager.h"
#include "server.h"

#include "libloom/compat.h"
#include "libloom/loomplan.pb.h"
#include "libloom/log.h"

#include <algorithm>
#include <limits.h>

using namespace loom;

TaskManager::TaskManager(Server &server)
    : server(server), cstate(server)
{
}

void TaskManager::add_plan(Plan &&plan)
{
    cstate.set_plan(std::move(plan));
    TaskDistribution distribution(cstate.compute_initial_distribution());
    distribute_work(distribution);
}

void TaskManager::distribute_work(TaskDistribution &distribution)
{
    if (distribution.size() == 0) {
        return;
    }

    if (llog->level() >= spdlog::level::debug) {
        size_t count = 0;
        for (auto &p : distribution) {
            count += p.second.size();
        }
        llog->debug("Distributing {} task(s)", count);
    }

    for (auto& p : distribution) {
        for (loom::Id id : p.second) {
            start_task(p.first, id);
        }
    }
}

void TaskManager::start_task(WorkerConnection *wc, Id task_id)
{
    const TaskNode &node = cstate.get_node(task_id);
    for (loom::Id id : node.get_inputs()) {
        TaskState state = cstate.get_state_or_create(id);
        TaskState::WStatus st = state.get_worker_status(wc);
        if (st == TaskState::NONE) {
            WorkerConnection *owner = state.get_first_owner();
            assert(owner);
            owner->send_data(id, wc->get_address(), false);
            state.set_worker_status(wc, TaskState::OWNER);
        }
    }
    wc->send_task(node);
    cstate.set_running_task(wc, task_id);
}

void TaskManager::remove_state(TaskState &state)
{
    assert(state.get_ref_counter() == 0);
    loom::Id id = state.get_id();
    state.foreach_owner([id](WorkerConnection *wc) {
        wc->remove_data(id);
    });
    cstate.remove_state(id);
}

void TaskManager::on_task_finished(loom::Id id, size_t size, size_t length, WorkerConnection *wc)
{
    cstate.set_task_finished(id, size, length, wc);
    const TaskNode &node = cstate.get_node(id);

    if (node.is_result()) {
        llog->debug("Job id={} [RESULT] finished", id);
        TaskState &state = cstate.get_state(id);
        WorkerConnection *owner = state.get_first_owner();
        assert(owner);
        owner->send_data(id, server.get_dummy_worker().get_address(), true);

        if (state.dec_ref_counter()) {
            remove_state(state);
        }
    } else {
        llog->debug("Job id={} finished (size={}, length={})", id, size, length);
    }

    for (loom::Id id : node.get_inputs()) {
        TaskState *state = cstate.get_state_ptr(id);
        if (state && state->dec_ref_counter()) {
            remove_state(*state);
        }
    }

    cstate.add_ready_nexts(node);
    TaskDistribution distribution(cstate.compute_distribution());
    distribute_work(distribution);
}

void TaskManager::register_worker(WorkerConnection *wc)
{
    cstate.add_worker(wc);
}
