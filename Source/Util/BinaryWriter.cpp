#include "BinaryWriter.h"
#include <Core/Endian.h>
#include <Core/Str.h>

// default alignment in bytes
const u32 ALIGNMENT = 4;

BinaryWriter::BinaryWriter(FILE* fp)
    : m_file(fp)
{}

void BinaryWriter::Write8(u8 n)
{
    fwrite(&n, 1, 1, m_file);
}

void BinaryWriter::Write16(u16 n)
{
    CheckAlign(2);
    n = EndianSwapLE16(n);
    fwrite(&n, 2, 1, m_file);
}

void BinaryWriter::Write32(u32 n)
{
    CheckAlign(4);
    n = EndianSwapLE32(n);
    fwrite(&n, 4, 1, m_file);
}

void BinaryWriter::WriteF32(f32 n)
{
    CheckAlign(4);
    n = EndianSwapLEFloat32(n);
    fwrite(&n, 4, 1, m_file);
}

void BinaryWriter::Write64(u64 n)
{
    CheckAlign(8);
    n = EndianSwapLE64(n);
    fwrite(&n, 8, 1, m_file);
}

void BinaryWriter::WriteRawData(const void* data, size_t len)
{
    CheckAlign(ALIGNMENT);
    fwrite(data, 1, len, m_file);
}

long BinaryWriter::WriteTemp32()
{
    CheckAlign(4);
    long pos = ftell(m_file);
    u32 temp = 0xFFFFFFFFU;
    fwrite(&temp, 4, 1, m_file);
    return pos;
}

void BinaryWriter::WriteStr(const char* str)
{
    size_t length = StrLen(str);
    fwrite(str, 1, length, m_file);
    fputc(0, m_file); // null terminator
    long pos = ftell(m_file);
    long remainder = pos % ALIGNMENT;
    if (remainder != 0) {
        u32 padding = ALIGNMENT - (u32)remainder;
        for (u32 i = 0; i < padding; ++i) {
            fputc(0, m_file);
        }
    }
}

void BinaryWriter::OverwriteTemp32(long pos, u32 n)
{
    long prevFilePointer = ftell(m_file);
    fseek(m_file, pos, SEEK_SET);
    Write32(n);
    fseek(m_file, prevFilePointer, SEEK_SET);
}

u32 BinaryWriter::RelativeOffset(long pos)
{
    long filePointer = AlignAndTell();
    return (u32)(filePointer - pos);
}

long BinaryWriter::AlignAndTell()
{
    CheckAlign(ALIGNMENT);
    return ftell(m_file);
}

void BinaryWriter::CheckAlign(u32 alignment)
{
    long pos = ftell(m_file);
    long remainder = pos % alignment;
    if (remainder != 0) {
        u32 padding = alignment - (u32)remainder;
        for (u32 i = 0; i < padding; ++i) {
            fputc(0xAA, m_file);
        }
    }
}
