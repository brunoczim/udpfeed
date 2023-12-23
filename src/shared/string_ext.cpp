#include <cctype>
#include <algorithm>
#include "string_ext.h"

std::string trim_spaces(std::string const& target)
{
    size_t end = target.size();
    while (end > 0 && std::isspace(target[end - 1])) {
        end--;
    }

    size_t start = 0;
    while (start < end && std::isspace(target[start])) {
        start++;
    }

    return target.substr(start, end - start);
}

bool string_starts_with_ignore_case(
    std::string const& haystack,
    std::string const& needle
)
{
    if (haystack.size() < needle.size()) {
        return false;
    }

    for (size_t i = 0; i < needle.size(); i++) {
        if (toupper(haystack[i]) != toupper(needle[i])) {
            return false;
        }
    }

    return true;
}
