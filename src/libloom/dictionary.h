#ifndef LIBLOOM_DICTIONARY_H
#define LIBLOOM_DICTIONARY_H

#include "types.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace loom {

/** Container for symbols */
class Dictionary {

public:
    Dictionary();

    loom::Id find_symbol_or_fail(const std::string &symbol) const;
    loom::Id find_symbol(const std::string &symbol) const;
    loom::Id find_or_create(const std::string &symbol);
    const std::string& translate(loom::Id id);

    std::vector<std::string> get_all_symbols() const;

private:
    std::unordered_map<std::string, loom::Id> symbol_to_id;
};

}

#endif // LIBLOOM_DICTIONARY_H
