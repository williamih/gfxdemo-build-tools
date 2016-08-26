#include "TempDir.h"

#include <vector>
#import <Cocoa/Cocoa.h>

#include <Core/Str.h>

std::string TempDirMake()
{
    NSString* tempDir = NSTemporaryDirectory();
    if (tempDir == nil)
        tempDir = @"/tmp";
    tempDir = [tempDir stringByAppendingPathComponent: @"temp.XXXXXX"];

    const char* strTemplate = [tempDir fileSystemRepresentation];

    std::vector<char> vec;
    size_t len = StrLen(strTemplate);
    vec.resize(len + 1);
    memcpy(&vec[0], strTemplate, len + 1);

    char* result = mkdtemp(&vec[0]);
    if (!result) {
        NSLog(@"Failed to create temporary directory (mkdtemp)");
        exit(1);
    }

    return result;
}
