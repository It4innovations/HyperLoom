#include "worker.h"
#include "loomcomm.pb.h"
#include "utils.h"
#include "log.h"
#include "types.h"
#include "config.h"

#include "data/rawdata.h"
#include "data/array.h"
#include "data/index.h"

#include "tasks/basetasks.h"
#include "tasks/rawdatatasks.h"
#include "tasks/arraytasks.h"
#include "tasks/runtask.h"
#include "tasks/python.h"

#include <stdlib.h>
#include <sstream>
#include <unistd.h>

using namespace loom;


Worker::Worker(uv_loop_t *loop,
               const Config &config)
    : loop(loop),
      server_conn(*this),
      server_port(config.get_port())
{
    spdlog::set_pattern("%H:%M:%S [%l] %v");
    if (config.get_debug()) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
    loom::llog->info("New worker; listening on port {}", config.get_port());

    GOOGLE_PROTOBUF_VERIFY_VERSION;
    UV_CHECK(uv_tcp_init(loop, &listen_socket));
    listen_socket.data = this;
    start_listen();

    auto &server_address = config.get_server_address();
    if (!server_address.empty()) {
        llog->info("Connecting to server {}:{}", server_address, server_port);
        uv_getaddrinfo_t* handle = new uv_getaddrinfo_t;
        handle->data = this;
        struct addrinfo hints;
        memset(&hints,0,sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        UV_CHECK(uv_getaddrinfo(
            loop, handle, _on_getaddrinfo,
            server_address.c_str(), "80", &hints));
    }

    auto &work_dir_root = config.get_work_dir();
    if (!work_dir_root.empty()) {
        std::stringstream s;
        s << work_dir_root;
        if (work_dir_root.back() != '/') {
            s << "/";
        }
        char tmp[100];
        if (gethostname(tmp, 100)) {
            llog->error("Cannot get hostname, using 'nohostname'");
            strcpy(tmp, "nohostname");
        }
        s << "worker-" << tmp << '-' << listen_port << '/';
        work_dir = s.str();

        if (make_path(work_dir.c_str(), S_IRWXU)) {
            llog->critical("Cannot create working directory '{}'", work_dir);
            exit(1);
        }

        if (mkdir((work_dir + "data").c_str(), S_IRWXU)) {
            llog->critical("Cannot create 'data' in working directory");
            exit(1);
        }

        llog->info("Using '{}' as working directory", work_dir);
    }

    add_unpacker<RawDataUnpacker>();
    add_unpacker<ArrayUnpacker>();
    add_unpacker<IndexUnpacker>();
    resource_cpus = config.get_cpus();
}

void Worker::register_basic_tasks()
{
    // Base
    add_task_factory<GetTask>("loom/base/get");
    add_task_factory<SliceTask>("loom/base/slice");
    add_task_factory<SizeTask>("loom/base/size");
    add_task_factory<LengthTask>("loom/base/length");

    // RawData
    add_task_factory<ConstTask>("loom/data/const");
    add_task_factory<ThreadTaskInstance<MergeJob>>("loom/data/merge");
    add_task_factory<OpenTask>("loom/data/open");
    add_task_factory<SplitTask>("loom/data/split");

    // Arrays
    add_task_factory<ArrayMakeTask>("loom/array/make");

    // Run
    add_task_factory<RunTask>("loom/run/run");

    // Python
    add_task_factory<ThreadTaskInstance<PyCallJob>>("loom/py/call");
}


void Worker::_on_getaddrinfo(uv_getaddrinfo_t* handle, int status,
        struct addrinfo* response) {
    Worker *worker = static_cast<Worker*>(handle->data);
    if (status) {
        llog->critical("Cannot resolve server name");
        uv_freeaddrinfo(response);
        delete handle;
        worker->close_all();
        exit(1);
    }
    assert(response->ai_family == AF_INET);
    char tmp[60];
    UV_CHECK(uv_ip4_name((struct sockaddr_in*) response->ai_addr, tmp, 60));
    worker->server_address = tmp;

    uv_freeaddrinfo(response);
    delete handle;

    llog->debug("Server address resolved to {}", worker->server_address);
    worker->server_conn.connect(worker->server_address, worker->server_port);
}

void Worker::_on_new_connection(uv_stream_t *stream, int status)
{
    UV_CHECK(status);
    Worker *worker = static_cast<Worker*>(stream->data);
    auto connection = std::make_unique<InterConnection>(*worker);
    connection->accept(&worker->listen_socket);
    llog->debug("Worker connection from {}", connection->get_peername());
    worker->add_connection(std::move(connection));
}

void Worker::start_listen()
{
    struct sockaddr_in addr;
    UV_CHECK(uv_ip4_addr("0.0.0.0", 0, &addr));

    UV_CHECK(uv_tcp_bind(&listen_socket, (const struct sockaddr *) &addr, 0));
    UV_CHECK(uv_listen((uv_stream_t *) &listen_socket, 10, _on_new_connection));

    struct sockaddr_in sockname;
    int namelen = sizeof(sockname);
    uv_tcp_getsockname(&listen_socket, (sockaddr*) &sockname, &namelen);
    listen_port = ntohs(sockname.sin_port);
}

/*int Worker::get_listen_port()
{
    struct sockaddr_in sa;
    int name_len = sizeof(sa);
    UV_CHECK(uv_tcp_getsockname(&listen_socket, (sockaddr *) &sa, &name_len));
    return ntohs(sa.sin_port);
}*/

void Worker::register_worker()
{
    loomcomm::Register msg;
    msg.set_type(loomcomm::Register_Type_REGISTER_WORKER);
    msg.set_protocol_version(PROTOCOL_VERSION);

    msg.set_port(get_listen_port());
    msg.set_cpus(resource_cpus);

    for (auto& factory : unregistered_task_factories) {
        msg.add_task_types(factory->get_name());
    }

    for (auto& factory : unregistered_unpack_factories) {
        msg.add_data_types(factory->get_type_name());
    }

    server_conn.send_message(msg);
}

void Worker::new_task(std::unique_ptr<Task> task)
{
    if (task->is_ready(*this)) {
        ready_tasks.push_back(std::move(task));
        check_ready_tasks();
        return;
    }
    waiting_tasks.push_back(std::move(task));
}

void Worker::start_task(std::unique_ptr<Task> task)
{
    llog->debug("Starting task id={} task_type={} n_inputs={}",
                task->get_id(), task->get_task_type(), task->get_inputs().size());
    auto i = task_factories.find(task->get_task_type());
    if (unlikely(i == task_factories.end())) {
        llog->critical("Task with unknown type {} received", task->get_task_type());
        exit(1);
    }
    auto task_instance = i->second->make_instance(*this, std::move(task));
    TaskInstance *t = task_instance.get();
    active_tasks.push_back(std::move(task_instance));

    DataVector input_data;
    for (Id id : t->get_inputs()) {
        input_data.push_back(get_data(id));
    }

    t->start(input_data);
    resource_cpus -= 1;
}

/*void Worker::new_task(const Task &task)
{
    auto task_type = task.get_task_type();
    assert(task_type >= 0 && task_type < (int) task_factories.size());
    auto task_instance = task_factories[task_type]->make_instance(*this, task);
    TaskInstance *t = task_instance.get();
    active_tasks.push_back(std::move(task_instance));
    t->start();
}*/

void Worker::publish_data(Id id, const std::shared_ptr<Data> &data)
{
    llog->debug("Publishing data id={} size={} info={}", id, data->get_size(), data->get_info());
    public_data[id] = data;
    check_waiting_tasks();
}

void Worker::remove_data(Id id)
{
    llog->debug("Removing data id={}", id);
    auto i = public_data.find(id);
    assert(i != public_data.end());
    public_data.erase(i);
}

InterConnection& Worker::get_connection(const std::string &address)
{
    auto &connection = connections[address];
    if (connection.get() == nullptr) {
        llog->info("Connecting to {}", address);
        connection = std::make_unique<InterConnection>(*this);

        std::stringstream ss(address);
        std::string base_address;
        std::getline(ss, base_address, ':');
        int port;
        ss >> port;

        if (base_address == "!server") {
            connection->connect(server_address, port);
        } else {
            connection->connect(base_address, port);
        }
    }
    return *connection;
}

void Worker::close_all()
{
    uv_close((uv_handle_t*) &listen_socket, NULL);
    server_conn.close();
    for (auto& pair : connections) {
        pair.second->close();
    }
    for (auto& c : nonregistered_connections) {
        c->close();
    }
}

void Worker::register_connection(InterConnection &connection)
{    
    auto &c = connections[connection.get_address()];
    if (unlikely(c.get() != nullptr)) {
        // This can happen when two workers connect each other in the same time
        llog->debug("Registration collision, leaving unregisted");
        // It is ok to leave it as it be, we will just hold the redundant connection
        // in unregistered connections
        return;
    }
    auto i = std::find_if(
                nonregistered_connections.begin(),
                nonregistered_connections.end(),
                [&](std::unique_ptr<InterConnection>& p) { return p.get() == &connection; });
    assert(i != nonregistered_connections.end());
    c = std::move(*i);
    nonregistered_connections.erase(i);
}

void Worker::unregister_connection(InterConnection &connection)
{
    const auto &i = connections.find(connection.get_address());
    if (unlikely(i == connections.end())) {
        auto i = std::find_if(
                    nonregistered_connections.begin(),
                    nonregistered_connections.end(),
                    [&](std::unique_ptr<InterConnection>& p) { return p.get() == &connection; });
        assert(i != nonregistered_connections.end());
        nonregistered_connections.erase(i);
        return;
    }
    connections.erase(i);
}

std::string Worker::get_run_dir(Id id)
{
    std::stringstream s;
    s << work_dir << "run/" << id << "/";
    return s.str();
}

void Worker::check_ready_tasks()
{
    while (resource_cpus > 0 && ready_tasks.size()) {
        auto task = std::move(ready_tasks[0]);
        ready_tasks.erase(ready_tasks.begin());
        start_task(std::move(task));
    }
}

void Worker::set_cpus(int value)
{
    if (value == 0) {
        value = sysconf(_SC_NPROCESSORS_ONLN);
        llog->debug("Autodetection of CPUs: {}", value);
    }
    if (value <= 0) {
        value = 1;
    }

    resource_cpus = value;
    llog->info("Number of CPUs for worker: {}", value);
}

void Worker::add_unpacker(std::unique_ptr<UnpackFactory> factory)
{
    unregistered_unpack_factories.push_back(std::move(factory));
}

std::unique_ptr<DataUnpacker> Worker::unpack(DataTypeId id)
{
    auto i = unpack_factories.find(id);
    assert(i != unpack_factories.end());
    return i->second->make_unpacker();
}

void Worker::on_dictionary_updated()
{
    for (auto &f : unregistered_task_factories) {
        loom::Id id = dictionary.find_symbol_or_fail(f->get_name());
        llog->debug("Registering task_factory: {} = {}", f->get_name(), id);
        task_factories[id] = std::move(f);
    }
    unregistered_task_factories.clear();

    for (auto &f : unregistered_unpack_factories) {
        loom::Id id = dictionary.find_symbol_or_fail(f->get_type_name());
        llog->debug("Registering unpack_factory: {} = {}", f->get_type_name(), id);
        unpack_factories[id] = std::move(f);
    }
    unregistered_unpack_factories.clear();
}

void Worker::check_waiting_tasks()
{
    bool something_new = false;
    auto i = waiting_tasks.begin();
    while (i != waiting_tasks.end()) {
        auto& task_ptr = *i;
        if (task_ptr->is_ready(*this)) {
            ready_tasks.push_back(std::move(task_ptr));
            i = waiting_tasks.erase(i);
            something_new = true;
        } else {
            ++i;
        }
    }
    if (something_new) {
        check_ready_tasks();
    }
}

void Worker::remove_task(TaskInstance &task, bool free_resources)
{
    if (free_resources) {
        resource_cpus += 1;
    }
    for (auto i = active_tasks.begin(); i != active_tasks.end(); i++) {
        if ((*i)->get_id() == task.get_id()) {
            active_tasks.erase(i);
            return;
        }
    }
    assert(0);
}

void Worker::task_failed(TaskInstance &task, const std::string &error_msg)
{
    llog->error("Task id={} failed: {}", task.get_id(), error_msg);
    if (server_conn.is_connected()) {
        loomcomm::WorkerResponse msg;
        msg.set_type(loomcomm::WorkerResponse_Type_FAILED);
        msg.set_id(task.get_id());
        msg.set_error_msg(error_msg);
        server_conn.send_message(msg);
    }
    remove_task(task);
}

void Worker::task_redirect(TaskInstance &task,
                           std::unique_ptr<TaskDescription> new_task_desc)
{
    loom::Id id = task.get_id();
    llog->debug("Redirecting task id={} task_type={} n_inputs={}",
                id, new_task_desc->task_type, new_task_desc->inputs.size());
    remove_task(task, false);

    Id task_type_id = dictionary.find_symbol_or_fail(new_task_desc->task_type);
    auto new_task = std::make_unique<Task>(id, task_type_id,
                                           std::move(new_task_desc->config));
    auto i = task_factories.find(task_type_id);
    if (unlikely(i == task_factories.end())) {
        llog->critical("Task with unknown type {} received", new_task->get_task_type());
        assert(0);
    }
    auto task_instance = i->second->make_instance(*this, std::move(new_task));
    TaskInstance *t = task_instance.get();
    active_tasks.push_back(std::move(task_instance));
    t->start(new_task_desc->inputs);
}

void Worker::task_finished(TaskInstance &task, Data &data)
{
    if (server_conn.is_connected()) {
        loomcomm::WorkerResponse msg;
        msg.set_type(loomcomm::WorkerResponse_Type_FINISH);
        msg.set_id(task.get_id());
        msg.set_size(data.get_size());
        msg.set_length(data.get_length());
        server_conn.send_message(msg);
    }
    remove_task(task);
    check_ready_tasks();
}

void Worker::send_data(const std::string &address, Id id, std::shared_ptr<Data> &data, bool with_size)
{
    auto &connection = get_connection(address);;
    connection.send(id, data, with_size);
}

ServerConnection::ServerConnection(Worker &worker)
    : SimpleConnectionCallback(worker.get_loop()),
      worker(worker)
{

}

void ServerConnection::on_connection()
{
    connection.start_read();
    worker.register_worker();
}

void ServerConnection::on_close()
{
    llog->critical("Connection to server is closed. Terminating ...");
    worker.close_all();
}

void ServerConnection::on_error(int error_code)
{
    llog->critical("Server connection error: {}", uv_strerror(error_code));
    connection.close();
}

void ServerConnection::on_message(const char *data, size_t size)
{
    loomcomm::WorkerCommand msg;
    assert(msg.ParseFromArray(data, size));
    auto type = msg.type();

    switch (type) {
    case loomcomm::WorkerCommand_Type_TASK: {
        llog->debug("Task id={} received", msg.id());
        auto task = std::make_unique<Task>(msg.id(),
                                           msg.task_type(),
                                           msg.task_config());
        for (int i = 0; i < msg.task_inputs_size(); i++) {
            task->add_input(msg.task_inputs(i));
        }
        worker.new_task(std::move(task));
        break;
    }
    case loomcomm::WorkerCommand_Type_REMOVE: {
        worker.remove_data(msg.id());
        break;
    }
    case loomcomm::WorkerCommand_Type_SEND: {
        auto& address = msg.address();
        /* "!" means address to server, so we replace the sign to proper address */
        if (address.size() > 2 && address[0] == '!' && address[1] == ':') {
            msg.set_address(worker.get_server_address() + ":" + address.substr(2, std::string::npos));
        }
        llog->debug("Sending data id={} to {}", msg.id(), msg.address());
        bool with_size = msg.has_with_size() && msg.with_size();
        assert(worker.send_data(msg.address(), msg.id(), with_size));
        break;
    }
    case loomcomm::WorkerCommand_Type_DICTIONARY: {
        auto count = msg.symbols_size();
        llog->debug("New dictionary ({} symbols)", count);
        Dictionary &dictionary = worker.get_dictionary();
        for (int i = 0; i < count; i++) {
            dictionary.find_or_create(msg.symbols(i));
        }
        worker.on_dictionary_updated();

    } break;
    default:
        llog->critical("Invalid message");
        exit(1);
    }
}
