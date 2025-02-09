#include "path.h"

internal inline bool is_separator(u8 c) {
    return c == '/' || c == '\\';
}

internal String8 path_join(Arena *arena, String8 parent, String8 child) {
    Assert(parent.data);
    String8 result = str8_zero();
    bool ends_in_slash = is_separator(parent.data[parent.count - 1]);
    u64 count = parent.count + child.count - ends_in_slash + 1;
    result.data = push_array(arena, u8, count + 1);
    MemoryCopy(result.data, parent.data, parent.count - ends_in_slash);
    result.data[parent.count - ends_in_slash] = '/';
    MemoryCopy(result.data + parent.count - ends_in_slash + 1, child.data, child.count);
    result.count = count;
    return result;
}

internal String8 path_strip_extension(Arena *arena, String8 path) {
    Assert(path.data);
    for (u64 i = path.count - 1; i > 0; i--) {
        switch (path.data[i]) {
        case '.':
        {
            String8 result = str8_copy(arena, str8(path.data + i + 1, path.count - i - 1));
            return result;
        }
        case '/':
        case '\\':
            //@Note no extension
            return str8_zero();
        }
    }
    return str8_zero();
}

internal String8 path_dir_name(String8 path) {
    String8 result = str8_zero();
    if (path.data) {
        u64 end = path.count - 1;
        while (end) {
            if (is_separator(path.data[end - 1])) break;
            end--;
        }
        result = str8(path.data, end);
    }
    return result;
}

internal String8 path_file_name(String8 path) {
    String8 result = str8_zero();
    if (path.data) {
        if (is_separator(path.data[path.count - 1])) {
            return result;
        }
        u64 start = path.count - 1;
        while (start > 0) {
            if (is_separator(path.data[start])) {
                result = str8(path.data + start + 1, path.count - start - 1);
                break;
            }
            start--;
        }
    }
    return result;
}

internal String8 path_remove_extension(String8 path) {
    String8 result = path;
    for (u64 i = path.count; i >= 0; i--) {
        if (is_separator(path.data[i - 1])) {
            return path;
        }

        if (path.data[i - 1] == '.') {
            return str8(path.data, i - 1);
        }
    }
    return result;
}

internal String8 path_strip_dir_name(Arena *arena, String8 path) {
    String8 result = str8_zero();
    if (path.data) {
        u64 end = path.count - 1;
        while (end) {
            if (is_separator(path.data[end - 1])) break;
            end--;
        }
        result = str8_copy(arena, str8(path.data, end));
        
    }
    return result;
}

internal String8 path_strip_file_name(Arena *arena, String8 path) {
    String8 result = str8_zero();
    if (path.data) {
        if (is_separator(path.data[path.count - 1])) {
            return result;
        }
        u64 start = path.count - 1;
        while (start > 0) {
            if (is_separator(path.data[start])) {
                result = str8_copy(arena, str8(path.data + start + 1, path.count - start - 1));
                break;
            }
            start--;
        }
    }
    return result;
}

internal u64 path_last_segment(String8 path) {
    u64 result = 0;
    for (u64 i = path.count; i > 0; i -= 1) {
        if (is_separator(path.data[i - 1])) {
            result = i;
            break;
        }
    }
    return result;
}

internal String8 normalize_path(Arena *arena, String8 path) {
    Assert(path.data);
    String8 result = str8_zero();
    Arena *scratch = arena_alloc(get_malloc_allocator(), path.count * 2);
    String8 buffer;
    buffer.data = push_array(scratch, u8, path.count + 1);
    buffer.count = 0;
    for (u64 idx = 0; idx < path.count; idx += 1) {
        //@Todo Check the last path segment even if no separator at end
        if (is_separator(path.data[idx])) {
            u64 seg_pos = path_last_segment(buffer);
            Rng_U64 rng = rng_u64(seg_pos, idx);
            String8 segment = str8_rng(path, rng);
            if (str8_match(segment, str8_lit("."), StringMatchFlag_Nil)) {
                buffer.count -= rng.max - rng.min;
            } else if (str8_match(segment, str8_lit(".."), StringMatchFlag_Nil)) {
                String8 b = buffer;
                b.count = rng.min - 1;
                u64 prev_seg = path_last_segment(b);
                buffer.count = prev_seg;
            }

            if (buffer.data[buffer.count - 1] != '/') {
                buffer.data[buffer.count] = '/';
                buffer.count += 1;
            }
            
        } else {
            buffer.data[buffer.count] = path.data[idx];
            buffer.count += 1;
        }
    }
    result = str8_copy(arena, buffer);
    return result;
}

internal bool path_is_absolute(String8 path) {
    switch (path.data[0]) {
#if defined(__linux__)
    case '~':
#endif
    case '\\':
    case '/':
        return true;
    }
    if (isalpha(path.data[0]) && path.data[1] == ':') return true;
    return false;
}

internal bool path_is_relative(String8 path) {
    return !path_is_absolute(path);
}

