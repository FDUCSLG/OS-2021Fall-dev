#pragma once

#include <csignal>
#include <cstdio>

#include <atomic>
#include <unistd.h>

#define PAUSE                                                                                      \
    { Pause pause; }

class Pause {
public:
    Pause() {
        int pid = getpid();
        printf("(debug) process %d paused.\n", pid);
        raise(SIGSTOP);
    }
};
