#pragma once

#include <sstream>

#include "exception.hpp"

template <typename T>
void _assert_eq(const T &actual,
                const T &expect,
                const char *expr,
                size_t line,
                const char *file,
                const char *func) {
    if (actual != expect) {
        std::stringstream buf;
        buf << "[" << file << ":" << func << " @" << line << "] ";
        buf << "assert_eq failed: '" << expr << "': expect '" << expect << "', got '" << actual
            << "'";
        throw Panic(buf.str());
    }
}

#define assert_eq(actual, expect)                                                                  \
    _assert_eq((actual), (expect), #actual, __LINE__, __FILE__, __FUNCTION__);
