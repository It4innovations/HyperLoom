#ifndef LIBLOOMW_WORKER_H
#define LIBLOOMW_WORKER_H

#include "interconnect.h"
#include "taskinstance.h"
#include "unpacking.h"
#include "taskfactory.h"

#include "libloom/dictionary.h"
#include "libloom/listener.h"

#include <uv.h>

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <memory>

namespace loom {

class Worker;
class DataUnpacker;
class Config;

/** Main class of the libloom that represents whole worker */
class Worker {    
    friend class ServerConnection;

public:
    Worker(uv_loop_t* loop, const Config &config);

    uv_loop_t *get_loop() {
        return loop;
    }

    void register_basic_tasks();
    
    void new_task(std::unique_ptr<Task> task);
    void send_data(const std::string &address, base::Id id, DataPtr &data);
    bool send_data(const std::string &address, base::Id id) {
        auto& data = public_data[id];
        if (data.get() == nullptr) {
            return false;
        }
        send_data(address, id, data);
        return true;
    }

    void task_finished(TaskInstance &task_instance, const DataPtr &data);
    void task_failed(TaskInstance &task_instance, const std::string &error_msg);
    void data_transferred(base::Id task_id);

    void task_redirect(TaskInstance &task, std::unique_ptr<TaskDescription> new_task_desc);
    void publish_data(base::Id id, const DataPtr &data);
    void remove_data(base::Id id);

    bool has_data(base::Id id) const
    {
        return public_data.find(id) != public_data.end();
    }

    DataPtr& get_data(base::Id id)
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
        return listener.get_port();
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

    std::string get_run_dir(base::Id id);

    void check_waiting_tasks();
    void check_ready_tasks();

    void set_cpus(int value);
    void add_unpacker(const std::string &symbol, const UnpackFactoryFn &unpacker);


    std::unique_ptr<DataUnpacker> get_unpacker(base::Id id);

    base::Dictionary& get_dictionary() {
        return dictionary;
    }

    void on_dictionary_updated();

private:
    void register_worker();

    void remove_task(TaskInstance &task, bool free_resources=true);
    void start_task(std::unique_ptr<Task> task);
    //int get_listen_port();

    void on_message(const char *data, size_t size);
    
    uv_loop_t *loop;

    int resource_cpus;
    int task_slots;
    std::deque<std::unique_ptr<TaskInstance>> active_tasks;
    std::deque<std::unique_ptr<Task>> ready_tasks;
    std::deque<std::unique_ptr<Task>> waiting_tasks;
    std::unordered_map<base::Id, std::unique_ptr<TaskFactory>> task_factories;

    std::unordered_map<int, DataPtr> public_data;
    std::string work_dir;

    std::unordered_map<base::Id, UnpackFactoryFn> unpack_ffs;

    base::Socket server_conn;
    std::unordered_map<std::string, std::unique_ptr<InterConnection>> connections;
    std::vector<std::unique_ptr<InterConnection>> nonregistered_connections;

    base::Dictionary dictionary;

    std::string server_address;
    int server_port;

    base::Listener listener;

    std::vector<std::unique_ptr<TaskFactory>> unregistered_task_factories;
    std::unordered_map<std::string, UnpackFactoryFn> unregistered_unpack_ffs;

    bool start_tasks;
    uv_idle_t start_tasks_idle;

    static void _on_getaddrinfo(uv_getaddrinfo_t* handle, int status, struct addrinfo* response);
    static void _start_tasks_callback(uv_idle_t *idle);
};

}

#endif // LIBLOOMW_WORKER_H
