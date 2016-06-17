//#include "fileservice.h"

//#include "worker.h"
//#include "utils.h"
//#include "log.h"

//#include <sys/stat.h>

//using namespace loom;

//FileService::FileService(FileServiceCallback &callback,
//                         const std::string &filename)
//    : callback(callback), filename(filename)
//{
//    request1.data = this;
//    request2.data = this;
//}

//void FileService::start()
//{
//    llog->debug("Opening file service: {}", filename);
//    uv_loop_t *loop = callback.get_worker().get_loop();
//    UV_CHECK(uv_fs_open(
//        loop, &request1, filename.c_str(), O_WRONLY, 0, _on_open));
//}

//void FileService::_on_open(uv_fs_t *request)
//{
//    FileService *service = static_cast<FileService*>(request->data);
//    uv_buf_t buf = service->input_data->get_uv_buf(worker);
//    uv_loop_t *loop = service->callback.get_worker().get_loop();
//    UV_CHECK(uv_fs_write(loop,
//                         &service->request1,
//                         request->file,
//                         &buf, 1, 0, _on_write));
//}

//void FileService::_on_write(uv_fs_t *request)
//{
//    FileService *service = static_cast<FileService*>(request->data);
//    service->input_data.reset();
//    service->callback.on_finish();
//}
