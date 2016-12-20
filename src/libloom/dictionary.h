#ifndef LIBLOOM_DICTIONARY_H
#define LIBLOOM_DICTIONARY_H

#include "types.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace loom {
namespace base {

/** Container for symbols */
class Dictionary {

public:
    Dictionary();

    loom::base::Id find_symbol_or_fail(const std::string &symbol) const;
    loom::base::Id find_symbol(const std::string &symbol) const;
    loom::base::Id find_or_create(const std::string &symbol);
    const std::string& translate(loom::base::Id id);

    std::vector<std::string> get_all_symbols() const;

private:
    std::unordered_map<std::string, loom::base::Id> symbol_to_id;
};

}
}

#endif // LIBLOOM_DICTIONARY_H
