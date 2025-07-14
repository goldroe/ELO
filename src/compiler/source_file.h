#if !defined(SOURCE_FILE_H)
#define SOURCE_FILE_H

struct Report;

struct Source_File {
    Source_File *next = nullptr;
    Source_File *prev = nullptr;
    String8 path;
    String8 text;
    Array<Report*> reports;
};

struct Source_File_Map {
    Source_File *first;
    Source_File *last;
    int count;
};

#endif //SOURCE_FILE_H
