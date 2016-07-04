#ifndef LIBLOOM_UNPACKING_H
#define LIBLOOM_UNPACKING_H

#include "data.h"
#include "compat.h"

#include <memory>

namespace loom {

class UnpackFactory
{
public:
    virtual ~UnpackFactory();
    virtual std::unique_ptr<DataUnpacker> make_unpacker() = 0;
};

template<typename T> class SimpleUnpackFactory : public UnpackFactory
{
public:
    std::unique_ptr<DataUnpacker> make_unpacker() {
        return std::make_unique<T>();
    }
};

}

#endif // LIBLOOM_UNPACKING_H
