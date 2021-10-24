#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include <sstream>

using std::size_t;

extern "C" {
void _panic(const char *file, size_t line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    std::stringstream buf;
    buf << "PANIC: " << file << " @" << line;
    throw std::runtime_error(buf.str());
}
}
