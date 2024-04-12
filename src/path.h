#ifndef PATH_H
#define PATH_H

char *copy_string(const char *str);
char *read_entire_file(char *file_name);
char *path_join(char *left, char *right);
char *path_strip_extension(char *path);
char *path_strip_dir_name(char *path);
char *path_strip_file_name(char *path);
char *path_strip_file_name_without_extension(char *path);
char *path_normalize(char *path);
char *path_home_name();
char *path_current_dir();
bool path_file_exists(char *path);
bool path_is_absolute(const char *path);
bool path_is_relative(const char *path);
char *path_get_full_path(char *path);

#endif // PATH_H
