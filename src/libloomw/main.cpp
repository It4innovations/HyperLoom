

#include "worker.h"
#include <uv.h>

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;    
  uv_loop_t loop;
  uv_loop_init(&loop);
  Worker client(&loop, "127.0.0.1", 9010);
  uv_run(&loop, UV_RUN_DEFAULT);
  uv_loop_close(&loop);  
  return 0;
}
