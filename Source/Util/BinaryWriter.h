#ifndef UTIL_BINARYWRITER_H
#define UTIL_BINARYWRITER_H

#include <stdio.h>
#include <Core/Types.h>

class BinaryWriter {
public:
    explicit BinaryWriter(FILE* fp);

    void Write8(u8 n);
    void Write16(u16 n);
    void Write32(u32 n);
    void Write64(u64 n);
    void WriteF32(f32 n);
    void WriteRawData(const void* data, size_t len);

    void WriteStr(const char* str);

    long WriteTemp32();
    void OverwriteTemp32(long pos, u32 n);
    u32 RelativeOffset(long pos);

    long AlignAndTell();
private:
    BinaryWriter(const BinaryWriter&);
    BinaryWriter& operator=(const BinaryWriter&);

    void CheckAlign(u32 alignment);

    FILE* m_file;
};

#endif // UTIL_BINARYWRITER_H
