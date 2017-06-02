#include "config.h"
#include <stdlib.h>

loom::Config::Config()
    : work_dir("/tmp"), cpus(0), debug(false), pinning(true)
{

}

void loom::Config::parse_args(int argc, char **argv)
{
    struct argp_option options[] = {
        { "debug", 300, 0, 0, "Debug mode"},
        { "cpus", 301, "NUMBER", 0, "Number of cpus (default: autodetect)"},
        { "wdir", 302, "DIRECTORY", 0, "Working directory (default: /tmp)"},
        { "nopin", 303, 0, 0, "Disable pinning of processes"},
        { 0 }
    };
    struct argp argp = { options, parse_opt, "SERVER-ADDRESS PORT" };
    int r = argp_parse (&argp, argc, argv, 0, 0, this);
    if (r) {
        exit(r);
    }
}

int
loom::Config::parse_opt (int key, char *arg,
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
    case 303:
        config->pinning = false;
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
