#if !defined(SOURCE_FILE_H)
#define SOURCE_FILE_H

#include "base/base_strings.h"
#include "array.h"

struct Report;

struct Source_File {
    Source_File *next = nullptr;
    Source_File *prev = nullptr;
    String path;
    String text;
    Array<Report*> reports;
};

struct Source_File_Map {
    Source_File *first;
    Source_File *last;
    int count;
};


extern Source_File_Map source_file_map;

internal Source_File *source_file_create(String8 file_path);
void add_source_file(Source_File *source_file);


#endif //SOURCE_FILE_H
