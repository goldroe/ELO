#ifndef PATH_H
#define PATH_H

internal String8 path_strip_extension(String8 path);
internal String8 path_strip_dir_name(Arena *arena, String8 path);
internal String8 path_strip_file_name(String8 path);
internal String8 normalize_path(Arena *arena, String8 path);
internal bool path_is_absolute(String8 path);
internal bool path_is_relative(String8 path);

#endif // PATH_H
