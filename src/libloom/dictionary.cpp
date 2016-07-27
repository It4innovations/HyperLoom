
#include "dictionary.h"

#include <assert.h>

using namespace loom;

Dictionary::Dictionary()
{

}

Id Dictionary::lookup_symbol(const std::string &symbol)
{
    auto i = symbol_to_id.find(symbol);
    assert(i != symbol_to_id.end());
    assert(i->second != -1);
    return i->second;
}

Id Dictionary::find_or_create(const std::string &symbol)
{
    auto i = symbol_to_id.find(symbol);
    if (i == symbol_to_id.end()) {
        int new_id = symbol_to_id.size();
        symbol_to_id[symbol] = new_id;
        return new_id;
    } else {
        return i->second;
    }
}

const std::string& Dictionary::translate(Id id)
{
    for (auto &i : symbol_to_id) {
        if (i.second == id) {
            return i.first;
        }
    }
    assert(0);
}

std::vector<std::string> Dictionary::get_all_symbols() const
{
    std::vector<std::string> symbols;
    int size = symbol_to_id.size();
    symbols.resize(size);
    for (auto &i : symbol_to_id) {
        assert(i.second >= 0 && i.second < size);
        symbols[i.second] = i.first;
    }
    return symbols;
}
