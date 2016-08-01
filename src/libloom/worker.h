#ifndef LOOM_WORKER_H
#define LOOM_WORKER_H

#include "interconnect.h"
#include "taskinstance.h"
#include "unpacking.h"
#include "taskfactory.h"
#include "dictionary.h"

#include <uv.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace loom {

class Worker;
class DataUnpacker;

class ServerConnection : public SimpleConnectionCallback {
    
public:
    ServerConnection(Worker &worker);

    bool is_connected() {
        return connection.get_state() == Connection::ConnectionOpen;
    }

protected:
    Worker &worker;

    void on_connection();
    void on_close();
    void on_message(const char *data, size_t size);
    void on_error(int error_code);
};


class Worker {    
    friend class ServerConnection;

public:
    Worker(uv_loop_t* loop,
           const std::string& server_address,
           int server_port,
           const std::string& work_dir_root);

    uv_loop_t *get_loop() {
        return loop;
    }
    
    void new_task(std::unique_ptr<Task> task);
    void send_data(const std::string &address, Id id, std::shared_ptr<Data> &data, bool with_size);
    bool send_data(const std::string &address, Id id, bool with_size) {
        auto& data = public_data[id];
        if (data.get() == nullptr) {
            return false;
        }
        send_data(address, id, data, with_size);
        return true;
    }

    void task_finished(TaskInstance &task_instance, Data &data);
    void task_failed(TaskInstance &task_instance, const std::string &error_msg);
    void publish_data(Id id, std::shared_ptr<Data> &data);
    void remove_data(Id id);

    bool has_data(Id id) const
    {
        return public_data.find(id) != public_data.end();
    }

    std::shared_ptr<Data>& get_data(Id id)
    {
        auto it = public_data.find(id);
        assert(it != public_data.end());
        return it->second;
    }

    void add_task_factory(std::unique_ptr<TaskFactory> factory)
    {        
        unregistered_task_factories.push_back(std::move(factory));
    }

    template<typename T> void add_task_factory(const std::string &name)
    {
        add_task_factory(std::make_unique<SimpleTaskFactory<T>>(name));
    }

    InterConnection &get_connection(const std::string &address);

    void close_all();

    int get_listen_port() const
    {
        return listen_port;
    }

    void add_connection(std::unique_ptr<InterConnection> connection)
    {
        nonregistered_connections.push_back(std::move(connection));
    }

    void register_connection(InterConnection &connection);
    void unregister_connection(InterConnection &connection);

    const std::string& get_server_address() {
        return server_address;
    }

    const std::string& get_work_dir() {
        return work_dir;
    }

    std::string get_run_dir(Id id);

    void check_waiting_tasks();
    void check_ready_tasks();

    void set_cpus(int value);
    void add_unpacker(DataTypeId type_id, std::unique_ptr<UnpackFactory> factory);
    std::unique_ptr<DataUnpacker> unpack(DataTypeId id);

    Dictionary& get_dictionary() {
        return dictionary;
    }

    void on_dictionary_updated();

private:
    void register_worker();
    void start_listen();

    void remove_task(TaskInstance &task);
    void start_task(std::unique_ptr<Task> task);
    //int get_listen_port();

    
    uv_loop_t *loop;

    int resource_cpus;
    std::vector<std::unique_ptr<TaskInstance>> active_tasks;
    std::vector<std::unique_ptr<Task>> ready_tasks;
    std::vector<std::unique_ptr<Task>> waiting_tasks;
    std::unordered_map<Id, std::unique_ptr<TaskFactory>> task_factories;

    std::unordered_map<int, std::shared_ptr<Data>> public_data;
    std::string work_dir;

    std::unordered_map<DataTypeId, std::unique_ptr<UnpackFactory>> unpack_factories;

    ServerConnection server_conn;
    std::unordered_map<std::string, std::unique_ptr<InterConnection>> connections;
    std::vector<std::unique_ptr<InterConnection>> nonregistered_connections;

    Dictionary dictionary;

    std::string server_address;
    int server_port;

    uv_tcp_t listen_socket;
    int listen_port;

    std::vector<std::unique_ptr<TaskFactory>> unregistered_task_factories;

    static void _on_new_connection(uv_stream_t *stream, int status);
    static void _on_getaddrinfo(uv_getaddrinfo_t* handle, int status, struct addrinfo* response);
};

}

#endif // LOOM_WORKER_H
