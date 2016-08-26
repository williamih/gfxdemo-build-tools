#ifndef OS_FILE_H
#define OS_FILE_H

#include <vector>
#include <Core/Types.h>

void FileReadAllBytes(const char* path, std::vector<u8>* output);
void FileDelete(const char* path);

#endif // OS_FILE_H
