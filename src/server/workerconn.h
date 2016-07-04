#ifndef LOOM_SERVER_WORKERCONN
#define LOOM_SERVER_WORKERCONN

#include "libloom/connection.h"
#include "libloom/types.h"

#include <unordered_map>

class Server;
class TaskNode;

class WorkerConnection : public loom::ConnectionCallback {

public:
    WorkerConnection(Server &server,
                     std::unique_ptr<loom::Connection> connection,
                     const std::string& address,
                     const std::vector<std::string> &task_types,
                     int resource_cpus);
    void on_message(const char *buffer, size_t size);
    void on_close();


    void send_task(TaskNode *task);
    void send_data(loom::Id id, const std::string &address, bool with_size);

    auto& get_tasks() {
        return tasks;
    }

    const std::string &get_address() {
        return address;
    }

    loom::TaskId translate_task_type_id(loom::TaskId task_type_id) {
        loom::TaskId size = task_type_translates.size();
        for (loom::TaskId i = 0; i < size; i++) {
            if (task_type_translates[i] == task_type_id) {
                return i;
            }
        }
        assert(0);
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

    std::vector<int> task_type_translates;
};

#endif // LOOM_SERVER_WORKERCONN
