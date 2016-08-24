#ifndef LOOM_SERVER_WORKERCONN
#define LOOM_SERVER_WORKERCONN

#include "libloom/connection.h"
#include "libloom/types.h"

#include <unordered_map>

class Server;
class TaskNode;

/** Connection to worker */
class WorkerConnection : public loom::ConnectionCallback {

public:
    WorkerConnection(Server &server,
                     std::unique_ptr<loom::Connection> connection,
                     const std::string& address,
                     const std::vector<loom::Id> &task_types,
                     const std::vector<loom::Id> &data_types,
                     int resource_cpus);
    void on_message(const char *buffer, size_t size);
    void on_close();


    void send_task(TaskNode *task);
    void send_data(loom::Id id, const std::string &address, bool with_size);
    void remove_data(loom::Id id);

    std::unordered_map<loom::Id, TaskNode*>& get_tasks() {
        return tasks;
    }

    const std::string &get_address() {
        return address;
    }

    int get_resource_cpus() const {
        return resource_cpus;
    }

private:
    Server &server;
    std::unique_ptr<loom::Connection> connection;
    std::unordered_map<loom::Id, TaskNode*> tasks;
    int resource_cpus;
    std::string address;

    std::vector<int> task_types;
    std::vector<int> data_types;
};

#endif // LOOM_SERVER_WORKERCONN
