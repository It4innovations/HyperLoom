//#ifndef LIBLOOM_FILESERVICE_H
//#define LIBLOOM_FILESERVICE_H

//#include <uv.h>

//#include <string>
//#include <memory>

//namespace loom {

//class Worker;
//class Data;

//class FileServiceCallback
//{
//public:
//    virtual ~FileServiceCallback() {}
//    virtual void on_finish() = 0;
//    virtual Worker& get_worker() = 0;
//};

//class FileService
//{
//public:
//    FileService(FileServiceCallback &callback, const std::string &filename);

//    void set_input_data(std::shared_ptr<Data> &data) {
//        input_data = data;
//    }

//    void start();
//protected:
//    FileServiceCallback &callback;
//    std::string filename;

//    std::shared_ptr<Data> input_data;

//    uv_fs_t request1;
//    uv_fs_t request2;

//    static void _on_open(uv_fs_t *request);
//    static void _on_write(uv_fs_t *request);
//};

//}


//#endif
