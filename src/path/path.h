#ifndef PATH_H
#define PATH_H

#include "base/base_memory.h"
#include "base/base_strings.h"

internal String path_join(Allocator allocator, String parent, String child);
internal String path_strip_extension(Allocator allocator, String path);
internal String path_get_extension(String path);
internal String path_dir_name(String path);
internal String path_file_name(String path);
internal String path_remove_extension(String path);
internal String path_strip_dir_name(Allocator allocator, String path);
internal String path_strip_file_name(Allocator allocator, String path);
internal u64 path_last_segment(String path);
internal String normalize_path(Allocator allocator, String path);
internal bool path_is_absolute(String path);
internal bool path_is_relative(String path);


#endif // PATH_H
