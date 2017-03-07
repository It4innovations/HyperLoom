
#include "globals.h"
#include <sstream>

loom::Globals::Globals() : file_id_counter(1)
{

}

std::string loom::Globals::create_data_filename()
{
    int file_id = file_id_counter++;
    std::stringstream s;
    s << work_dir << "data/" << file_id;
    return s.str();
}

void loom::Globals::init(const std::string &work_dir)
{
    this->work_dir = work_dir;
}
