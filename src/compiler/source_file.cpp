#include "base/base_strings.h"
#include "source_file.h"
#include "os/os.h"

global Source_File_Map source_file_map;

internal Source_File *source_file_create(String8 file_path) {
    for (Source_File *file = source_file_map.first; file; file = file->next) {
        if (str8_equal(file_path, file->path)) {
            return file;
        }
    }

    Source_File *file = nullptr;
    OS_Handle file_handle = os_open_file(file_path, OS_AccessFlag_Read);
    if (os_valid_handle(file_handle)) {
    file = new Source_File;
        file->text = os_read_file_string(file_handle);
        file->path = str8_copy(heap_allocator(), file_path);
        os_close_handle(file_handle);
    }
    return file;
}

void add_source_file(Source_File *source_file) {
    for (Source_File *file = source_file_map.first; file; file = file->next) {
        if (str8_equal(source_file->path, file->path)) {
            return;
        }
    }
    DLLPushBack(source_file_map.first, source_file_map.last, source_file, next, prev);
    source_file_map.count++;
}

