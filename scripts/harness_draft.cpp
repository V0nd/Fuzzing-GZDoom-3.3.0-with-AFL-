#include <cstdint>      //uint8_t, size_t
#include <cstdio>       //FILE, fopen, fread, fprintf, perror
#include "files.h"      //FileReader

class FResourceFile;
FResourceFile *CheckWad(const char *filename, FileReader &file, bool quiet);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FileReader reader;
    reader.OpenMemory(data, size);

    //CheckWad returns nullptr on size check or magic check failure
    FResourceFile *result = nullptr;
    try 
    {
        result = CheckWad("fuzz_input.wad", reader, true);
    }
    catch (...) 
    {
        //suppress C++ exceptions, we want crashes only
    }

    if (result != nullptr) 
    {
        delete result;
    }

    return 0;
}

int main(int argc, char **argv) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "Usage: %s <wad_file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) 
    {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    //check ftell, don't want any crashes in harness
    if (size < 0) 
    {
        fprintf(stderr, "ftell failed\n");
        fclose(f);
        return 1;
    }

    fseek(f, 0, SEEK_SET);
    uint8_t *buf = new uint8_t[size];

    size_t bytes_read = fread(buf, 1, size, f);
    fclose(f);

    //safety check to see if fread got it right
    if (bytes_read != (size_t)size) 
    {
        fprintf(stderr, "fread failed: read %zu of %ld\n", bytes_read, size);
        delete[] buf;
        return 1;
    }

    LLVMFuzzerTestOneInput(buf, size);

    delete[] buf;
    return 0;
}
