#include "resendtask.h"
#include "server.h"

#include "libloom/log.h"

using namespace loom;

ResendTask::ResendTask(Server &server, std::unique_ptr<Task> task)
    : TaskInstance(server.get_dummy_worker(), std::move(task)), server(server)
{
    request.data = this;
}

void ResendTask::start(DataVector &input_data)
{
    assert(input_data.size() == 1);
    data = *input_data[0];


    loomcomm::Data msg;

    msg.set_id(data->get_id());
    msg.set_size(data->get_size());

    llog->debug("Resending data id={} to client", data->get_id());

    auto size = msg.ByteSize();

    message_data = std::make_unique<char[]>(size + sizeof(uint32_t));
    uint32_t *size_ptr = (uint32_t*) message_data.get();
    *size_ptr = size;
    msg.SerializePartialToArray(message_data.get() + sizeof(uint32_t), size);
    message_size = size;

    uv_buf_t bufs[2];
    bufs[0].base = message_data.get();
    bufs[0].len = message_size + sizeof(uint32_t);
    bufs[1] = data->get_uv_buf(worker);
    auto &connection = server.get_client_connection();
    connection.send(&request, bufs, 2, _on_write);
}

void ResendTask::_on_write(uv_write_t *write_req, int status)
{    
    UV_CHECK(status);
    ResendTask *task = static_cast<ResendTask*>(write_req->data);
    llog->debug("Resend task id={} finished", task->get_id());
    task->finish_without_data();    
}

ResendTaskFactory::ResendTaskFactory(Server &server)
    : TaskFactory("resend"), server(server)
{

}

std::unique_ptr<TaskInstance> ResendTaskFactory::make_instance(Worker &worker, std::unique_ptr<Task> task)
{
    return std::make_unique<ResendTask>(server, std::move(task));
}
