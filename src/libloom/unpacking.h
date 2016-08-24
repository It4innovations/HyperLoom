#ifndef LIBLOOM_UNPACKING_H
#define LIBLOOM_UNPACKING_H

#include "data.h"
#include "compat.h"

#include <memory>

namespace loom {

/** Base class for factory producing deserializers for data types */
class UnpackFactory
{
public:
    virtual ~UnpackFactory();
    virtual std::unique_ptr<DataUnpacker> make_unpacker() = 0;
    virtual const char* get_type_name() const = 0;
};

template<typename T> class SimpleUnpackFactory : public UnpackFactory
{
public:
    std::unique_ptr<DataUnpacker> make_unpacker() {
        return std::make_unique<T>();
    }

    virtual const char* get_type_name() const {
        return T::get_type_name();
    }
};

}

#endif // LIBLOOM_UNPACKING_H
