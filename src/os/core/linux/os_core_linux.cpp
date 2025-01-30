internal bool os_path_exists(Arena *arena) {
    bool result = access(fname, F_OK) == 0;
    return result;
}

internal String8 os_current_dir(Arena *arena) {
    char *c = get_current_dir_name();
    String8 result = str8_cstring(c);
    return result;
}

internal String8 os_home_path(Arena *arena) {
    char *c = getenv("HOME");
    if (c == NULL) {
        c = getpwuid(getuid())->pw_dir;
    } 
    String8 result = str8_copy(arena, str8_cstring(c));
    return result;
}
