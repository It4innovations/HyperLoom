#ifndef LIBLOOMW_INIT_H
#define LIBLOOMW_INIT_H

#include <string>
#include <argp.h>

namespace loom {

class Config {

public:
    Config();

    void parse_args(int argc, char **argv);

    const std::string& get_server_address() const {
        return server_address;
    }

    const std::string& get_work_dir() const {
        return work_dir;
    }

    int get_port() const {
        return port;
    }

    int get_cpus() const {
        return cpus;
    }

    bool get_debug() const {
        return debug;
    }

protected:
    std::string server_address;
    std::string work_dir;
    int port;
    int cpus;
    bool debug;

private:
    static int parse_opt(int key, char *arg, struct argp_state *state);
};

}


#endif
