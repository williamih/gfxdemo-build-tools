#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <ctype.h> // for isspace
#include <assert.h>
#include <Core/Macros.h>
#include <Os/Process.h>
#include <Os/File.h>
#include <Util/BinaryWriter.h>
#include "TempDir.h"

struct FILEWrapper {
    FILEWrapper(FILE* fp) : fp(fp) {}
    ~FILEWrapper() { fclose(fp); }

    operator FILE*() { return fp; }

    FILE* fp;
};

struct FileDeletionAssurance {
    explicit FileDeletionAssurance(const std::string& path) : path(path) {}

    ~FileDeletionAssurance() { if (!path.empty()) FileDelete(path.c_str()); }

    void Stop() { path.clear(); }

    std::string path;
};

const int SHADER_FORMAT_VERSION = 1;

const char* const TOOL_METAL =
"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/usr/bin/metal";

const char* const TOOL_METAL_AR =
"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/usr/bin/metal-ar";

const char* const TOOL_METALLIB =
"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/usr/bin/metallib";

const char* const SYSROOT =
"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk";

const char* const AIR_FILE = "out.air";
const char* const DIAG_FILE = "diag.dia";
const char* const METAL_AR_FILE = "out.metal-ar";
const char* const METAL_LIBRARY_FILE = "library.metallib";

const char* const TEMP_SHADER_FILE = "result.shd";

// N.B. The iteration behavior of std::map (in ascending order of the keys)
// is important to the algorithm. Do not change this to an unordered_map!
typedef std::map<int, std::string> IfdefMap;

static void FindOptionIfDefs(const char* path, IfdefMap* map);
static u32 NumberOfSetBits(u32 i);
static std::string JoinPaths(const char* first, const char* second);
static bool InternalCompileShader(const char* inputPath, const char* tempDir,
                                  const std::vector<std::string>& macros,
                                  std::vector<u8>* outputBytes,
                                  std::string* errorOutput);

static bool Compile(const char* inputPath, const char* outputPath,
                    std::string* errorOutput)
{
    ASSERT(errorOutput);

    std::string tempDir = TempDirMake();
    std::string shaderPath = JoinPaths(tempDir.c_str(), TEMP_SHADER_FILE);

    FileDeletionAssurance deletionAssurance(shaderPath);

    FILEWrapper fileWrapper(fopen(shaderPath.c_str(), "wb"));
    BinaryWriter writer(fileWrapper);

    IfdefMap ifdefs;
    FindOptionIfDefs(inputPath, &ifdefs);

    const u32 nPermutations = 1 << (u32)ifdefs.size();

    writer.WriteRawData("RHS", 4);
    writer.Write32(SHADER_FORMAT_VERSION);
    writer.WriteRawData("LTEM", 4);
    writer.Write32(nPermutations);

    std::vector<u32> numbers;
    numbers.reserve(nPermutations);
    for (u32 i = 0; i < nPermutations; ++i) {
        numbers.push_back(i);
    }

    std::sort(numbers.begin(), numbers.end(), [](u32 a, u32 b) -> bool {
        return NumberOfSetBits(a) > NumberOfSetBits(b);
    });

    std::vector<std::string> macros;
    std::vector<u8> shaderBytes;
    for (u32 k = 0; k < nPermutations; ++k) {
        macros.clear();

        u32 i = numbers[k];
        u64 permuteMask = 0;

        auto mapIter = ifdefs.begin();
        for (u32 j = 0; j < ifdefs.size(); ++j, ++mapIter) {
            if (i & (1 << j)) {
                u32 bitIndex = mapIter->first;
                const std::string& ifdef = mapIter->second;
                macros.push_back(ifdef);
                permuteMask |= u64(1) << bitIndex;
            }
        }

        bool result = InternalCompileShader(inputPath, tempDir.c_str(),
                                            macros, &shaderBytes, errorOutput);
        if (!result)
            return false;

        long permuteHeaderPos = writer.AlignAndTell();
        writer.Write64(permuteMask);
        writer.Write32((u32)shaderBytes.size()); // VS data length
        writer.Write32(0); // PS data length
        long pos_ofsNextPermutation = writer.WriteTemp32();
        writer.Write32(0); // padding (for alignment purposes)
        writer.WriteRawData(&shaderBytes[0], shaderBytes.size());
        writer.OverwriteTemp32(pos_ofsNextPermutation,
                               (u32)(writer.AlignAndTell() - permuteHeaderPos));

    }

    FileMove(shaderPath.c_str(), outputPath);
    deletionAssurance.Stop();
    errorOutput->clear();

    return true;
}

int main(int argc, const char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: MTLShaderCompiler input_path output_path\n");
        return 1;
    }
    const char* inputPath = argv[1];
    const char* outputPath = argv[2];
    std::string errorOutput;
    bool success = Compile(inputPath, outputPath, &errorOutput);
    if (!success) {
        fprintf(stderr, "%s", errorOutput.c_str());
        return 1;
    }
    return 0;
}

// Searches for statements matching the regex '#ifdef\\s+F_\\d\\d\\S*'.
static void FindOptionIfDefs(const char* path, IfdefMap* map)
{
    ASSERT(path);
    ASSERT(map);

    std::ifstream infile(path);

    std::string line;
    while (std::getline(infile, line)) {
        std::string::size_type pos = line.find("#ifdef ");
        if (pos == std::string::npos)
            continue;

        pos += 6; // StrLen("#ifdef") == 6
        for ( ; pos < line.length() && isspace(line[pos]); ++pos)
             ;

        if (pos + 4 > line.length()) continue; // StrLen("F_##") == 4
        if (line[pos++] != 'F') continue;
        if (line[pos++] != '_') continue;
        if (!isdigit(line[pos])) continue;
        int digit1 = line[pos++] - '0';
        if (!isdigit(line[pos])) continue;
        int digit2 = line[pos++] - '0';

        std::string::size_type endPos = pos;
        for (; endPos < line.length() && !isspace(line[endPos]); ++endPos)
            ;
        std::string::size_type startPos = pos - 4; // StrLen("F_##") == 4
        std::string ifdef = line.substr(pos - 4, endPos - startPos);

        int index = digit1 * 10 + digit2;
        (*map)[index] = ifdef;
    }
}

static u32 NumberOfSetBits(u32 i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

static std::string JoinPaths(const char* first, const char* second)
{
    std::string result(first);
    result.append("/");
    result.append(second);
    return result;
}

static bool RunMetal(const char* inputPath,
                     const char* outputPath,
                     const char* diagFilePath,
                     const std::vector<std::string>& macros,
                     std::string* output)
{
    ASSERT(output);

    std::vector<const char*> args;
    args.push_back(TOOL_METAL);
    args.push_back("-emit-llvm");
    args.push_back("-c");
    args.push_back("-ffast-math");
    args.push_back("-mmacosx-version-min=10.9");
    args.push_back("-std=osx-metal1.1");
    args.push_back("-isysroot");
    args.push_back(SYSROOT);
    args.push_back("-serialize-diagnostics");
    args.push_back(diagFilePath);
    args.push_back("-o");
    args.push_back(outputPath);
    for (const std::string& macro : macros) {
        args.push_back("-D");
        args.push_back(macro.c_str());
    }
    args.push_back(inputPath);
    args.push_back(NULL);

    Process process(TOOL_METAL, args);
    if (process.result != PROCESS_SUCCESS)
        FATAL("Could not run 'metal' command-line tool");

    *output = process.stderrStr;

    return process.status == 0;
}

static bool RunMetalAr(const char* inputPath, const char* outputPath,
                       std::string* output)
{
    std::vector<const char*> args;
    args.push_back(TOOL_METAL_AR);
    args.push_back("r");
    args.push_back(outputPath);
    args.push_back(inputPath);
    args.push_back(NULL);

    Process process(TOOL_METAL_AR, args);
    if (process.result != PROCESS_SUCCESS)
        FATAL("Could not run 'metal-ar' command-line tool");

    *output = process.stderrStr;

    return process.status == 0;
}

static bool RunMetalLib(const char* inputPath, const char* outputPath,
                        std::string* output)
{
    std::vector<const char*> args;
    args.push_back(TOOL_METALLIB);
    args.push_back("-o");
    args.push_back(outputPath);
    args.push_back(inputPath);
    args.push_back(NULL);

    Process process(TOOL_METALLIB, args);
    if (process.result != PROCESS_SUCCESS)
        FATAL("Could not run 'metallib' command-line tool");

    *output = process.stderrStr;

    return process.status == 0;
}

static bool InternalCompileShader(const char* inputPath, const char* tempDir,
                                  const std::vector<std::string>& macros,
                                  std::vector<u8>* outputBytes,
                                  std::string* errorOutput)
{
    ASSERT(outputBytes);
    ASSERT(errorOutput);

    std::string airFile = JoinPaths(tempDir, AIR_FILE);
    std::string diagFile = JoinPaths(tempDir, DIAG_FILE);
    std::string metalArFile = JoinPaths(tempDir, METAL_AR_FILE);
    std::string metalLibFile = JoinPaths(tempDir, METAL_LIBRARY_FILE);

    if (!RunMetal(inputPath, airFile.c_str(), diagFile.c_str(), macros, errorOutput))
        return false;
    FileDelete(diagFile.c_str());

    if (!RunMetalAr(airFile.c_str(), metalArFile.c_str(), errorOutput))
        return false;
    FileDelete(airFile.c_str());

    if (!RunMetalLib(metalArFile.c_str(), metalLibFile.c_str(), errorOutput))
        return false;
    FileDelete(metalArFile.c_str());

    FileReadAllBytes(metalLibFile.c_str(), outputBytes);

    FileDelete(metalLibFile.c_str());

    return true;
}
