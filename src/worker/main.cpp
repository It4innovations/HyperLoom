
#include "libloomw/worker.h"
#include "libloom/log.h"
#include "libloomw/config.h"


#include <memory>


using namespace loom;


int main(int argc, char **argv)
{
    /* Create a configuration and parse args */
    Config config;
    config.parse_args(argc, argv);

    /* Init libuv */
    uv_loop_t loop;
    uv_loop_init(&loop);

    /* Create worker */
    loom::Worker worker(&loop, config);
    worker.register_basic_tasks();

    /* Start loop */
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    return 0;
}
