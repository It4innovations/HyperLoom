#ifndef LOOM_SERVER_WORKERCONN
#define LOOM_SERVER_WORKERCONN

#include "libloom/socket.h"
#include "libloom/types.h"

#include <assert.h>

class Server;
class TaskNode;


/** Connection to worker */
class WorkerConnection {

public:
    WorkerConnection(Server &server,
                     std::unique_ptr<loom::base::Socket> socket,
                     const std::string& address,
                     const std::vector<loom::base::Id> &task_types,
                     const std::vector<loom::base::Id> &data_types,
                     int resource_cpus,
                     int worker_id);
    void on_message(const char *buffer, size_t size);

    void send_task(const TaskNode &task);
    void send_data(loom::base::Id id, const std::string &address);
    void remove_data(loom::base::Id id);

    const std::string &get_address() {
        return address;
    }

    int get_resource_cpus() const {
        return resource_cpus;
    }

    int get_worker_id() const {
        return worker_id;
    }

    int get_free_cpus() const {
        return free_cpus;
    }

    void add_free_cpus(int value) {
        free_cpus += value;
        assert(free_cpus >= 0);
    }

    void remove_free_cpus(int value) {
        free_cpus -= value;
        assert(free_cpus >= 0);
    }

private:
    Server &server;
    std::unique_ptr<loom::base::Socket> socket;
    int free_cpus;
    int resource_cpus;
    std::string address;

    std::vector<int> task_types;
    std::vector<int> data_types;

    int worker_id;
};

#endif // LOOM_SERVER_WORKERCONN
