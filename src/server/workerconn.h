#ifndef LOOM_SERVER_WORKERCONN
#define LOOM_SERVER_WORKERCONN

#include "libloomnet/socket.h"
#include "libloomw/types.h"

class Server;
class PlanNode;


/** Connection to worker */
class WorkerConnection {

public:
    WorkerConnection(Server &server,
                     std::unique_ptr<loom::net::Socket> socket,
                     const std::string& address,
                     const std::vector<loom::Id> &task_types,
                     const std::vector<loom::Id> &data_types,
                     int resource_cpus,
                     int worker_id);
    void on_message(const char *buffer, size_t size);

    void send_task(const PlanNode &task);
    void send_data(loom::Id id, const std::string &address);
    void remove_data(loom::Id id);

    const std::string &get_address() {
        return address;
    }

    int get_resource_cpus() const {
        return resource_cpus;
    }

    int get_worker_id() const {
        return worker_id;
    }

private:
    Server &server;
    std::unique_ptr<loom::net::Socket> socket;
    int resource_cpus;
    std::string address;

    std::vector<int> task_types;
    std::vector<int> data_types;

    int worker_id;
};

#endif // LOOM_SERVER_WORKERCONN
