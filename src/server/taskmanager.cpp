#include "taskmanager.h"
#include "server.h"
#include "trace.h"

#include "libloom/compat.h"
#include "pb/comm.pb.h"
#include "libloom/log.h"

#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <memory>

using namespace loom;
using namespace loom::base;

TaskManager::TaskManager(Server &server)
    : server(server), cstate(server)
{
}

loom::base::Id TaskManager::add_plan(const loom::pb::comm::Plan &plan, bool load_checkpoints)
{
    std::vector<TaskNode*> to_load;
    loom::base::Id id_base = cstate.add_plan(plan, load_checkpoints, to_load);
    for (TaskNode *node : to_load) {
        WorkerConnection *wc = random_worker();
        node->set_as_loading(wc);
        wc->load_checkpoint(node->get_id(), node->get_task_def().checkpoint_path);
    }
    distribute_work(schedule(cstate));
    return id_base;
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

    wc->send_task(node);

    cstate.activate_pending_node(node, wc);
    //auto &trace = server.get_trace();
    /*if (trace) {
        trace->trace_task_start(node, wc);
    }*/
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
    node.set_not_needed();
    //cstate.remove_node(node);
    node.reset_owners();
    cstate.remove_node(node);
}

void TaskManager::on_task_finished(loom::base::Id id, size_t size, size_t length, WorkerConnection *wc, bool checkpointing)
{
   if (unlikely(wc->is_blocked())) {
      wc->residual_task_finished(id, true, checkpointing);
      return;
   }
   TaskNode &node = cstate.get_node(id);
   node.set_as_finished(wc, size, length);

   /*auto &trace = server.get_trace();
    if (trace) {
        trace->trace_task_end(*node, wc);
    }*/

   // Task with checkpoints is "finished" for worker only if
   // checkpoint is written
   if (!checkpointing && node.is_result()) {
      logger->debug("Job id={} [RESULT] finished", id);

      ClientConnection *cc = server.get_client_connection();
      if (cc) {
           cc->send_info_about_finished_result(node);
      }
   } else {
      assert(checkpointing || !node.get_nexts().empty());
      logger->debug("Job id={} finished (size={}, length={})", id, size, length);
   }

   if (checkpointing) {
        wc->change_checkpoint_writes(1);
   }

   for (TaskNode *input_node : node.get_inputs()) {
      if (input_node->next_finished(node) && !input_node->is_result()) {
         remove_node(*input_node);
      }
   }

   if (!node.get_nexts().empty()) {
      for (TaskNode *nn : node.get_nexts()) {
         if (nn->input_is_ready(&node)) {
            cstate.add_pending_node(*nn);
         }
      }
   } else if (!node.is_result()) {
        remove_node(node);
   }

   if (cstate.has_pending_nodes()) {
      server.need_task_distribution();
   }
}

void TaskManager::on_data_transferred(Id id, WorkerConnection *wc)
{
   if (unlikely(wc->is_blocked())) {
      wc->residual_task_finished(id, true, false);
      return;
   }
   TaskNode &node = cstate.get_node(id);
   logger->debug("Data id={} transferred to {}", id, wc->get_address());
   node.set_as_transferred(wc);
}

void TaskManager::on_task_failed(Id id, WorkerConnection *wc, const std::string &error_msg)
{
   if (unlikely(wc->is_blocked())) {
      wc->residual_task_finished(id, false, false);
      return;
   }
   logger->error("Task id={} failed on worker {}: {}",
                  id, wc->get_address(), error_msg);

    auto cc = server.get_client_connection();
    if (cc) {
        cc->send_task_failed(id, *wc, error_msg);
    }

    TaskNode &node = cstate.get_node(id);
    node.set_as_none(wc);
    trash_all_tasks();

    if (cstate.has_pending_nodes()) {
        server.need_task_distribution();
    }
}

void TaskManager::on_checkpoint_write_finished(Id id, WorkerConnection *wc)
{
    if (unlikely(wc->is_blocked())) {
       wc->residual_checkpoint_finished(id);
       return;
    }
    logger->debug("Checkpoint id={} finished on worker {}", id, wc->get_address());
    wc->change_checkpoint_writes(-1);
    TaskNode &node = cstate.get_node(id);
    assert(node.has_defined_checkpoint());
    node.set_checkpoint();

    if (node.is_result()) {
        ClientConnection *cc = server.get_client_connection();
        if (cc) {
             cc->send_info_about_finished_result(node);
        }
    }
}

void TaskManager::on_checkpoint_load_finished(Id id, WorkerConnection *wc, size_t size, size_t length)
{
    if (unlikely(wc->is_blocked())) {
       wc->residual_task_finished(id, false, false);
       return;
    }
    wc->change_checkpoint_loads(-1);

    TaskNode &node = cstate.get_node(id);
    node.set_as_loaded(wc, size, length);

    if (node.is_result()) {
       logger->debug("Task id={} [RESULT] checkpoint loaded", id);

       ClientConnection *cc = server.get_client_connection();
       if (cc) {
            cc->send_info_about_finished_result(node);
       }
    } else {
        logger->debug("Task id={} checkpoint loaded", id);
    }

    if (!node.get_nexts().empty()) {
       for (TaskNode *nn : node.get_nexts()) {
          if (nn->input_is_ready(&node)) {
             cstate.add_pending_node(*nn);
          }
       }
    } else if (!node.is_result()) {
         remove_node(node);
    }
    if (cstate.has_pending_nodes()) {
       server.need_task_distribution();
    }
}

void TaskManager::on_checkpoint_write_failed(Id id, WorkerConnection *wc, const std::string &error_msg)
{
    wc->change_checkpoint_writes(-1);
    if (unlikely(wc->is_blocked())) {
       return;
    }
    logger->error("Checkpoint id={} failed on worker {}: {}",
                   id, wc->get_address(), error_msg);
    auto cc = server.get_client_connection();
    if (cc) {
        cc->send_task_failed(id, *wc, error_msg);
    }
    trash_all_tasks();
}

void TaskManager::on_checkpoint_load_failed(Id id, WorkerConnection *wc, const std::string &error_msg)
{
    if (unlikely(wc->is_blocked())) {
        wc->residual_task_finished(id, false, false);
        return;
    }
    wc->change_checkpoint_loads(-1);
    logger->error("Checkpoint id={} load failed on worker {}: {}",
                   id, wc->get_address(), error_msg);
    auto cc = server.get_client_connection();
    if (cc) {
        cc->send_task_failed(id, *wc, error_msg);
    }
}

void TaskManager::run_task_distribution()
{
    uv_loop_t *loop = server.get_loop();

    // Update & get time
    uv_update_time(loop);
    auto start_scheduler = uv_now(loop);

    auto& trace = server.get_trace();
    if (trace) {
        trace->trace_scheduler_start();
    }

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
    if (trace) {
        trace->trace_scheduler_end();
    }
}

void TaskManager::trash_all_tasks()
{
    for (auto &wc : server.get_connections()) {
        wc->change_residual_tasks(wc->get_checkpoint_loads());
        wc->change_checkpoint_loads(-wc->get_checkpoint_loads());
    }
    cstate.foreach_node([](std::unique_ptr<TaskNode> &task) {
        task->foreach_worker([&task](WorkerConnection *wc, TaskStatus status) {
            if (status == TaskStatus::OWNER) {
              wc->remove_data(task->get_id());
            } else if (status == TaskStatus::RUNNING) {
              wc->change_residual_tasks(1);
              wc->free_resources(*task);
              logger->debug("Residual task id={} on worker={}", task->get_id(), wc->get_worker_id());
            } else {
               assert(status == TaskStatus::TRANSFER);
               wc->change_residual_tasks(1);
               logger->debug("Residual transfer id={} on worker={}", task->get_id(), wc->get_worker_id());
            }
            status = TaskStatus::NONE;
        });
    });
    cstate.clear_all();
}

void TaskManager::release_node(TaskNode *node)
{
   if (node->get_nexts().empty()) {
      remove_node(*node);
      return;
   } else {
      logger->debug("Removing result flag id={}", node->get_id());
      node->reset_result_flag();
   }
}

void TaskManager::worker_fail(WorkerConnection &conn)
{
    auto cc = server.get_client_connection();
    if (cc) {
        cc->send_task_failed(-1, conn, "Worker lost");
    }
    trash_all_tasks();
}

WorkerConnection *TaskManager::random_worker()
{
    auto &connections = server.get_connections();
    assert(!connections.empty());
    int index = rand() % connections.size();
    return connections[index].get();
}
