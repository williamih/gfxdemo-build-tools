#ifndef OS_PROCESS_H
#define OS_PROCESS_H

#include <vector>
#include <string>

enum ProcessCreationResult {
    PROCESS_SUCCESS,
    PROCESS_NOT_FOUND
};

struct Process {
    Process(const char* path, const std::vector<const char*>& args);

    ProcessCreationResult result;
    int status;
    std::string stdoutStr;
    std::string stderrStr;
};

#endif // OS_PROCESS_H
