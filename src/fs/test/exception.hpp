#pragma once

#include <cstdio>

#include <string>

struct Exception {
    std::string message;

    Exception(const std::string &_message) : message(_message) {}

    virtual void print() {
        print_with_prefix("exception");
    }

protected:
    void print_with_prefix(const std::string &prefix) {
        fprintf(stderr, "%s: %s\n", prefix.data(), message.data());
    }
};

struct Internal final : Exception {
    using Exception::Exception;

    void print() override {
        print_with_prefix("internal");
    }
};

struct Panic final : Exception {
    using Exception::Exception;

    void print() override {
        print_with_prefix("PANIC");
    }
};

struct Offline final : Exception {
    using Exception::Exception;

    void print() override {
        print_with_prefix("offline");
    }
};
