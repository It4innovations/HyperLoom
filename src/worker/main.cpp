
#include "runtask.h"
#include "basictasks.h"
#include "rawdatatasks.h"
#include "arraytasks.h"

#include "libloom/worker.h"
#include "libloom/log.h"


#include <memory>
#include <argp.h>

using namespace loom;


struct Config {
    Config() : work_dir("/tmp"),
               cpus(0),
               debug(false) {
    }

    std::string server_address;
    std::string work_dir;
    int port;
    int cpus;
    bool debug;

};


static int
parse_opt (int key, char *arg,
           struct argp_state *state)
{
    Config *config = (Config*) state->input;

    switch (key)
    {
    case 300:
        config->debug = true;
        break;
    case 301:
        config->cpus = atoi(arg);
        if (config->cpus <= 0) {
            fprintf(stderr, "Invalid number of cpus\n");
            exit(1);
        }
        break;
    case 302:
        config->work_dir = arg;
        break;
    case ARGP_KEY_ARG:
        switch(state->arg_num) {
            case 0:
                config->server_address = arg;
                break;
            case 1:
                config->port = atoi(arg);
                if (config->port <= 0 || config->port > 65535) {
                    fprintf(stderr, "Invalid port number\n");
                    exit(1);
                }
                break;
             default:
                argp_usage(state); /* no return */
        }
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 2)
        {
            argp_usage (state);
            /* no return */
        }
        break;
    }
    return 0;
}

static int parse_args(int argc, char **argv, Config *config)
{
    struct argp_option options[] = {
        { "debug", 300, 0, 0, "Debug mode"},
        { "cpus", 301, "NUMBER", 0, "Number of cpus (default: autodetect)"},
        { "wdir", 302, "DIRECTORY", 0, "Working directory (default: /tmp)"},
        { 0 }
    };
    struct argp argp = { options, parse_opt, "SERVER-ADDRESS PORT" };
    return argp_parse (&argp, argc, argv, 0, 0, config);
}


int main(int argc, char **argv)
{
    Config config;
    int r = parse_args(argc, argv, &config);
    if (r) {
        return r;
    }

    spdlog::set_pattern("%H:%M:%S [%l] %v");
    if (config.debug) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    uv_loop_t loop;
    uv_loop_init(&loop);
    loom::Worker worker(&loop,
                        config.server_address,
                        config.port,
                        config.work_dir);
    loom::llog->info("Worker started; listening on port {}", worker.get_listen_port());

    // Base
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<GetTask>>("base/get"));
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<SliceTask>>("base/slice"));

    // RawData
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<ConstTask>>("data/const"));
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<MergeTask>>("data/merge"));
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<OpenTask>>("data/open"));
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<SplitTask>>("data/split"));

    // Arrays
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<ArrayMakeTask>>("array/make"));

    // Run
    worker.add_task_factory(
                std::make_unique<SimpleTaskFactory<RunTask>>("run/run"));

    worker.set_cpus(config.cpus);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    return 0;
}
