#include "server.h"
#include "spdlog/spdlog.h"

#include <argp.h>

struct Config {
    Config() : port(9010), debug(false) {}

    int port;
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

    case 'p':
        config->port = atoi(arg);
        if (config->port <= 0 || config->port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            exit(1);
        }
    }
    return 0;
}

int parse_args(int argc, char **argv, Config *config)
{
    struct argp_option options[] = {
        { "debug", 300, 0, 0, "Debug mode"},
        { "port", 'p', "NUMBER", 0, "Listen port for server (default: 9010)"},
        { 0 }
    };
    struct argp argp = { options, parse_opt };
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
    Server server(&loop, config.port);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    return 0;
}
