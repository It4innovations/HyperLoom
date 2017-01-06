#include "worker.h"
#include "utils.h"

#include "config.h"

#include "data/rawdata.h"
#include "data/array.h"
#include "data/index.h"

#include "tasks/basetasks.h"
#include "tasks/rawdatatasks.h"
#include "tasks/arraytasks.h"
#include "tasks/runtask.h"
#include "tasks/python.h"

#include "libloom/loomcomm.pb.h"
#include "libloom/types.h"
#include "libloom/log.h"
#include "libloom/sendbuffer.h"
#include "libloom/pbutils.h"

#include <stdlib.h>
#include <sstream>
#include <unistd.h>

using namespace loom;
using namespace loom::base;


Worker::Worker(uv_loop_t *loop,
               const Config &config)
    : loop(loop),
      server_conn(loop),
      server_port(config.get_port())
{
    spdlog::set_pattern("%H:%M:%S [%l] %v");
    if (config.get_debug()) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
    logger->info("New worker; listening on port {}", config.get_port());

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    listener.start(loop, 0, [this]() {
        auto connection = std::make_unique<InterConnection>(*this);
        connection->accept(listener);
        logger->debug("Worker connection from {}", connection->get_peername());
        add_connection(std::move(connection));
    });

    auto &server_address = config.get_server_address();
    if (!server_address.empty()) {
        logger->info("Connecting to server {}:{}", server_address, server_port);
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
            logger->error("Cannot get hostname, using 'nohostname'");
            strcpy(tmp, "nohostname");
        }
        s << "worker-" << tmp << '-' << listener.get_port() << '/';
        work_dir = s.str();

        if (make_path(work_dir.c_str(), S_IRWXU)) {
            logger->critical("Cannot create working directory '{}'", work_dir);
            exit(1);
        }

        if (mkdir((work_dir + "data").c_str(), S_IRWXU)) {
            logger->critical("Cannot create 'data' in working directory");
            exit(1);
        }

        logger->info("Using '{}' as working directory", work_dir);
    }
    set_cpus(config.get_cpus());

    server_conn.set_on_error([this](int error_code) {
       logger->critical("Server connection error: {}", uv_strerror(error_code));
       close_all();
    });

    server_conn.set_on_close([this]() {
       logger->critical("Connection to server is closed. Terminating ...");
       close_all();
    });

    server_conn.set_on_connect([this]() {
        register_worker();
    });

    server_conn.set_on_message([this](const char *data, size_t size) {
        on_message(data, size);
    });

    add_unpacker("loom/data", [this]() {
       return std::make_unique<RawDataUnpacker>(*this);
    });
    add_unpacker("loom/array", [this]() {
       return std::make_unique<ArrayUnpacker>(*this);
    });
    add_unpacker("loom/index", std::make_unique<IndexUnpacker>);
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
        logger->critical("Cannot resolve server name");
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

    logger->debug("Server address resolved to {}", worker->server_address);
    worker->server_conn.connect(worker->server_address, worker->server_port);
}

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

    for (auto& pair : unregistered_unpack_ffs) {
        msg.add_data_types(pair.first);
    }

    loom::base::send_message(server_conn, msg);
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
    logger->debug("Starting task id={} task_type={} n_inputs={}",
                task->get_id(), task->get_task_type(), task->get_inputs().size());
    auto i = task_factories.find(task->get_task_type());
    if (unlikely(i == task_factories.end())) {
        logger->critical("Task with unknown type {} received", task->get_task_type());
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

void Worker:: publish_data(Id id, const DataPtr &data)
{
    logger->debug("Publishing data id={} size={} info={}", id, data->get_size(), data->get_info());
    public_data[id] = data;
    check_waiting_tasks();
}

void Worker::remove_data(Id id)
{
    logger->debug("Removing data id={}", id);
    auto i = public_data.find(id);
    assert(i != public_data.end());
    public_data.erase(i);
}

InterConnection& Worker::get_connection(const std::string &address)
{
    auto &connection = connections[address];
    if (connection.get() == nullptr) {
        logger->info("Connecting to {}", address);
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
    listener.close();
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
        logger->debug("Registration collision, leaving unregisted");
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
        logger->debug("Autodetection of CPUs: {}", value);
    }
    if (value <= 0) {
        logger->critical("Cannot detect number of CPUs");
        exit(1);
    }

    resource_cpus = value;
    logger->info("Number of CPUs for worker: {}", value);
}

void Worker::add_unpacker(const std::string &symbol, const UnpackFactoryFn &unpacker)
{
    unregistered_unpack_ffs[symbol] = unpacker;
}

std::unique_ptr<DataUnpacker> Worker::get_unpacker(base::Id id)
{
    auto i = unpack_ffs.find(id);
    assert(i != unpack_ffs.end());
    return i->second();
}

void Worker::on_dictionary_updated()
{
    for (auto &f : unregistered_task_factories) {
        loom::base::Id id = dictionary.find_symbol_or_fail(f->get_name());
        logger->debug("Registering task_factory: {} = {}", f->get_name(), id);
        task_factories[id] = std::move(f);
    }
    unregistered_task_factories.clear();

    for (auto &pair : unregistered_unpack_ffs) {
        loom::base::Id id = dictionary.find_symbol_or_fail(pair.first);
        logger->debug("Registering unpack_factory: {} = {}", pair.first, id);
        unpack_ffs[id] = pair.second;
    }
    unregistered_unpack_ffs.clear();
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
    logger->error("Task id={} failed: {}", task.get_id(), error_msg);
    if (server_conn.is_connected()) {
        loomcomm::WorkerResponse msg;
        msg.set_type(loomcomm::WorkerResponse_Type_FAILED);
        msg.set_id(task.get_id());
        msg.set_error_msg(error_msg);
        send_message(server_conn, msg);
    }
    remove_task(task);
}

void Worker::task_redirect(TaskInstance &task,
                           std::unique_ptr<TaskDescription> new_task_desc)
{
    loom::base::Id id = task.get_id();
    logger->debug("Redirecting task id={} task_type={} n_inputs={}",
                id, new_task_desc->task_type, new_task_desc->inputs.size());
    remove_task(task, false);

    Id task_type_id = dictionary.find_symbol_or_fail(new_task_desc->task_type);
    auto new_task = std::make_unique<Task>(id, task_type_id,
                                           std::move(new_task_desc->config));
    auto i = task_factories.find(task_type_id);
    if (unlikely(i == task_factories.end())) {
        logger->critical("Task with unknown type {} received", new_task->get_task_type());
        assert(0);
    }
    auto task_instance = i->second->make_instance(*this, std::move(new_task));
    TaskInstance *t = task_instance.get();
    active_tasks.push_back(std::move(task_instance));
    t->start(new_task_desc->inputs);
}

void Worker::task_finished(TaskInstance &task, const DataPtr &data)
{
    if (server_conn.is_connected()) {
        loomcomm::WorkerResponse msg;
        msg.set_type(loomcomm::WorkerResponse_Type_FINISHED);
        msg.set_id(task.get_id());
        msg.set_size(data->get_size());
        msg.set_length(data->get_length());
        send_message(server_conn, msg);
    }
    remove_task(task);
    check_ready_tasks();
}

void Worker::data_transfered(base::Id task_id)
{
    if (server_conn.is_connected()) {
        loomcomm::WorkerResponse msg;
        msg.set_type(loomcomm::WorkerResponse_Type_TRANSFERED);
        msg.set_id(task_id);
        send_message(server_conn, msg);
    }
}

void Worker::send_data(const std::string &address, Id id, DataPtr &data)
{
    auto &connection = get_connection(address);;
    connection.send(id, data);
}

void Worker::on_message(const char *data, size_t size)
{
    loomcomm::WorkerCommand msg;
    assert(msg.ParseFromArray(data, size));
    auto type = msg.type();

    switch (type) {
    case loomcomm::WorkerCommand_Type_TASK: {
        logger->debug("Task id={} received", msg.id());
        auto task = std::make_unique<Task>(msg.id(),
                                           msg.task_type(),
                                           msg.task_config());
        for (int i = 0; i < msg.task_inputs_size(); i++) {
            task->add_input(msg.task_inputs(i));
        }
        new_task(std::move(task));
        break;
    }
    case loomcomm::WorkerCommand_Type_REMOVE: {
        remove_data(msg.id());
        break;
    }
    case loomcomm::WorkerCommand_Type_SEND: {
        auto& address = msg.address();
        /* "!" means address to server, so we replace the sign to proper address */
        if (address.size() > 2 && address[0] == '!' && address[1] == ':') {
            msg.set_address(get_server_address() + ":" + address.substr(2, std::string::npos));
        }
        logger->debug("Sending data id={} to {}", msg.id(), msg.address());
        assert(send_data(msg.address(), msg.id()));
        break;
    }
    case loomcomm::WorkerCommand_Type_DICTIONARY: {
        auto count = msg.symbols_size();
        logger->debug("New dictionary ({} symbols)", count);
        Dictionary &dictionary = get_dictionary();
        for (int i = 0; i < count; i++) {
            dictionary.find_or_create(msg.symbols(i));
        }
        on_dictionary_updated();
    } break;
    default:
        logger->critical("Invalid message");
        exit(1);
    }
}
