#include "File.h"

#include <stdio.h>

#include <Core/Macros.h>

void FileReadAllBytes(const char* path, std::vector<u8>* output)
{
    ASSERT(output);

    FILE* file;
    long length;
    bool result = false;

    for (;;) {
        if (!(file = fopen(path, "rb")))
            break;

        if (fseek(file, 0, SEEK_END))
            break;

        if ((length = ftell(file)) == -1)
            break;

        if (fseek(file, 0, SEEK_SET))
            break;

        output->resize((size_t)length);

        if (fread(&(*output)[0], 1, (size_t)length, file) != (size_t)length)
            break;

        result = true;
        break;
    }

    // Clean up
    if (file) fclose(file);
    if (!result)
        FATAL("Failed to read file %s", path);
}

void FileDelete(const char* path)
{
    if (remove(path) != 0)
        FATAL("Failed to delete file %s", path);
}

void FileMove(const char* currPath, const char* newPath)
{
    if (rename(currPath, newPath) != 0)
        FATAL("Failed to rename file %s to %s", currPath, newPath);
}
