#include "resendtask.h"
#include "server.h"

#include "libloom/loomcomm.pb.h"
#include "libloom/log.h"

using namespace loom;

ResendTask::ResendTask(Server &server, std::unique_ptr<Task> task)
    : TaskInstance(server.get_dummy_worker(),std::move(task)),
      server(server), buffer(*this)
{

}

void ResendTask::start(DataVector &input_data)
{
    assert(input_data.size() == 1);
    auto data = *input_data[0];
    llog->debug("Resending data id={} to client", data->get_id());

    loomcomm::Data msg;
    msg.set_id(data->get_id());
    msg.set_size(data->get_size());

    buffer.add(msg);
    buffer.add(data, data->get_data(worker), data->get_size());

    auto &connection = server.get_client_connection();
    connection.send_buffer(&buffer);
}

ResendTaskFactory::ResendTaskFactory(Server &server)
    : TaskFactory("resend"), server(server)
{

}

std::unique_ptr<TaskInstance> ResendTaskFactory::make_instance(Worker &worker, std::unique_ptr<Task> task)
{
    return std::make_unique<ResendTask>(server, std::move(task));
}

ResendTask::_SendBuffer::_SendBuffer(ResendTask &task) : task(task)
{

}

void ResendTask::_SendBuffer::on_finish(int status)
{
    UV_CHECK(status);
    llog->debug("Resend task id={} finished", task.get_id());
    task.finish_without_data();
}
