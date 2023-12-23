#ifndef SHARED_STRING_EXT_H_
#define SHARED_STRING_EXT_H_ 1

#include <string>

std::string trim_spaces(std::string const& target);

bool string_starts_with_ignore_case(
    std::string const& haystack,
    std::string const& needle
);

#endif
