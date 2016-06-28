/*#ifndef LOOM_SERVER_RESENDJOB_H
#define LOOM_SERVER_RESENDJOB_H

#include "libloom/taskinstance.h"
#include "libloom/taskfactory.h"
#include "libloom/sendbuffer.h"

namespace loom {
    class Data;
}

class Server;

class ResendTask : public loom::TaskInstance
{
public:
    class _SendBuffer : public loom::SendBuffer {
    public:
        _SendBuffer(ResendTask &task);
        ~_SendBuffer() {}
        void on_finish(int status);
    protected:
        ResendTask &task;
    };

    ResendTask(Server &server, std::unique_ptr<loom::Task> task);
    void start(loom::DataVector &input_data);

protected:
    Server &server;    
    _SendBuffer buffer;
    static void _on_write(uv_write_t *write_req, int status);
};


class ResendTaskFactory : public loom::TaskFactory
{
public:
    ResendTaskFactory(Server &server);
    std::unique_ptr<loom::TaskInstance> make_instance(loom::Worker &worker, std::unique_ptr<loom::Task> task);

private:
    Server &server;
};


#endif // LOOM_SERVER_RESENDJOB_H
*/
