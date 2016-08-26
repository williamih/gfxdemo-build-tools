#ifndef OS_FILE_H
#define OS_FILE_H

#include <vector>
#include <Core/Types.h>

void FileReadAllBytes(const char* path, std::vector<u8>* output);
void FileDelete(const char* path);
void FileMove(const char* currPath, const char* newPath);

#endif // OS_FILE_H
