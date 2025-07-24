#ifndef PATH_H
#define PATH_H

#include "base/base_memory.h"
#include "base/base_strings.h"

String path_join(Allocator allocator, String parent, String child);
String path_strip_extension(Allocator allocator, String path);
String path_get_extension(String path);
String path_dir_name(String path);
String path_file_name(String path);
String path_remove_extension(String path);
String path_strip_dir_name(Allocator allocator, String path);
String path_strip_file_name(Allocator allocator, String path);
u64 path_last_segment(String path);
String normalize_path(Allocator allocator, String path);
bool path_is_absolute(String path);
bool path_is_relative(String path);


#endif // PATH_H
