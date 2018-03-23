#include "workerconn.h"
#include "server.h"

#include "libloom/log.h"
#include "pb/comm.pb.h"
#include "taskmanager.h"


using namespace loom;
using namespace loom::base;

WorkerConnection::WorkerConnection(Server &server,
                                   std::unique_ptr<loom::base::Socket> socket,
                                   const std::string& address,
                                   const std::vector<loom::base::Id> &task_types,
                                   const std::vector<loom::base::Id> &data_types,
                                   int resource_cpus, int worker_id)
    : server(server),
      socket(std::move(socket)),
      free_cpus(resource_cpus),
      resource_cpus(resource_cpus),
      address(address),
      task_types(task_types),
      data_types(data_types),
      worker_id(worker_id),
      n_residual_tasks(0),
      checkpoint_writes(0),
      checkpoint_loads(0)
{
    logger->info("Worker {} connected (cpus={})", address, resource_cpus);
    if (this->socket) {
        this->socket->set_on_message([this](const char *buffer, size_t size) {
            on_message(buffer, size);
        });
        this->socket->set_on_close([this](){
            logger->info("Worker {} disconnected.", this->address);
            this->server.remove_worker_connection(*this);
        });
        server.send_dictionary(*this->socket);
    }

    if (task_types.size() == 0) {
        logger->warn("No task_type has been registered by worker");
    }
}

void WorkerConnection::on_message(const char *buffer, size_t size)
{
    using namespace loom::pb::comm;
    WorkerResponse msg;
    msg.ParseFromArray(buffer, size);

    auto type = msg.type();
    if (type == WorkerResponse_Type_FINISHED) {
        server.on_task_finished(msg.id(), msg.size(), msg.length(), this, false);
        return;
    }

    if (type == WorkerResponse_Type_FINISHED_AND_CHECKPOINTING) {
        server.on_task_finished(msg.id(), msg.size(), msg.length(), this, true);
        return;
    }

    if (type == WorkerResponse_Type_TRANSFERED) {
        server.on_data_transferred(msg.id(), this);
        return;
    }

    if (type == WorkerResponse_Type_FAILED) {
        assert(msg.has_error_msg());
        server.on_task_failed(msg.id(), this, msg.error_msg());
        return;
    }

    if (type == WorkerResponse_Type_CHECKPOINT_WRITTEN) {
        server.on_checkpoint_write_finished(msg.id(), this);
        return;
    }

    if (type == WorkerResponse_Type_CHECKPOINT_WRITE_FAILED) {
        server.on_checkpoint_write_failed(msg.id(), this, msg.error_msg());
        return;
    }

    if (type == WorkerResponse_Type_CHECKPOINT_LOADED) {
        server.on_checkpoint_load_finished(msg.id(), this, msg.size(), msg.length());
        return;
    }

    if (type == WorkerResponse_Type_CHECKPOINT_LOAD_FAILED) {
        server.on_checkpoint_load_failed(msg.id(), this, msg.error_msg());
        return;
    }
}

void WorkerConnection::send_task(const TaskNode &task)
{
    using namespace loom::pb::comm;
    auto id = task.get_id();
    logger->debug("Assigning task id={} to address={} cpus={}",
                  id, address, task.get_n_cpus());

    WorkerCommand msg;
    msg.set_type(WorkerCommand_Type_TASK);
    msg.set_id(id);
    const TaskDef& def = task.get_task_def();
    msg.set_task_type(def.task_type);
    msg.set_task_config(def.config);
    msg.set_n_cpus(def.n_cpus);
    msg.set_checkpoint_path(def.checkpoint_path);

    for (TaskNode *input_node : task.get_inputs()) {
        msg.add_task_inputs(input_node->get_id());
    }
    send_message(*socket, msg);
}

void WorkerConnection::send_data(Id id, const std::string &address)
{
    using namespace loom::pb::comm;
    logger->debug("Command for {}: SEND id={} address={}", this->address, id, address);

    WorkerCommand msg;
    msg.set_type(WorkerCommand_Type_SEND);
    msg.set_id(id);
    msg.set_address(address);
    send_message(*socket, msg);
}

void WorkerConnection::load_checkpoint(Id id, const std::string &checkpoint_path)
{
    checkpoint_loads += 1;
    using namespace loom::pb::comm;
    logger->debug("Command for {}: LOAD_CHECKPOINT id={} path={}", this->address, id, checkpoint_path);

    WorkerCommand msg;
    msg.set_type(WorkerCommand_Type_LOAD_CHECKPOINT);
    msg.set_id(id);
    msg.set_checkpoint_path(checkpoint_path);
    send_message(*socket, msg);
}

void WorkerConnection::remove_data(Id id)
{
    using namespace loom::pb::comm;
    logger->debug("Command for {}: REMOVE id={}", this->address, id);
    WorkerCommand msg;
    msg.set_type(WorkerCommand_Type_REMOVE);
    msg.set_id(id);
    send_message(*socket, msg);
}

void WorkerConnection::free_resources(TaskNode &node)
{
   add_free_cpus(node.get_n_cpus());
}

void WorkerConnection::residual_task_finished(Id id, bool success, bool checkpointing)
{
   logger->debug("Residual tasks id={} finished on {}", id, address);
   change_residual_tasks(-1);

   if (checkpointing) {
       checkpoint_writes += 1;
   }

   if (success) {
      remove_data(id);
   }
   if (!is_blocked()) {
      server.need_task_distribution();
   }
}

void WorkerConnection::residual_checkpoint_finished(Id id)
{
   logger->debug("Residual checkpoint id={} finished on {}", id, address);
   checkpoint_writes -= 1;
   if (!is_blocked()) {
      server.need_task_distribution();
   }
}

void WorkerConnection::create_trace(const std::string &trace_path)
{
    using namespace loom::pb::comm;
    WorkerCommand msg;
    msg.set_type(WorkerCommand_Type_UPDATE);
    msg.set_trace_path(trace_path);
    msg.set_worker_id(worker_id);
    send_message(*socket, msg);
}
