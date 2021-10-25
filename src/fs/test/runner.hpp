#pragma once

#include <cstdio>

#include <functional>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

using TestFunc = std::function<int()>;

struct Testcase {
    std::string name;
    TestFunc func;
};

class Runner {
public:
    Runner(const std::vector<Testcase> &_testcases) : testcases(_testcases) {}

    void run() {
        for (const auto &testcase : testcases) {
            run(testcase);
        }
    }

    static bool run(const Testcase &testcase) {
        int pid;
        if ((pid = fork()) == 0)
            exit(testcase.func());

        int ws;
        waitpid(pid, &ws, 0);

        if (!WIFEXITED(ws)) {
            fprintf(stderr, "(error) \"%s\" [%d] exited abnormally.\n", testcase.name.data(), pid);
            return false;
        } else {
            int status = WEXITSTATUS(ws);
            if (status != 0) {
                fprintf(stderr,
                        "(error) \"%s\" [%d] exited with status %d.\n",
                        testcase.name.data(),
                        pid,
                        status);
                return false;
            } else
                printf("(info) \"%s\" passed.\n", testcase.name.data());
        }

        return true;
    }

private:
    std::vector<Testcase> testcases;
};
