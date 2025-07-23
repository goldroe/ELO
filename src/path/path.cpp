#include <cctype>

#include "base/base_core.h"
#include "path.h"

internal inline bool is_separator(u8 c) {
    return c == '/' || c == '\\';
}

internal String path_join(Allocator allocator, String parent, String child) {
    Assert(parent.data);
    String result = str8_zero();
    bool ends_in_slash = is_separator(parent.data[parent.count - 1]);
    u64 count = parent.count + child.count - ends_in_slash + 1;
    result.data = array_alloc(allocator, u8, count + 1);
    MemoryCopy(result.data, parent.data, parent.count - ends_in_slash);
    result.data[parent.count - ends_in_slash] = '/';
    MemoryCopy(result.data + parent.count - ends_in_slash + 1, child.data, child.count);
    result.data[count] = 0;
    result.count = count;
    return result;
}

internal String path_strip_extension(Allocator allocator, String path) {
    Assert(path.data);
    for (u64 i = path.count - 1; i > 0; i--) {
        switch (path.data[i]) {
        case '.':
        {
            String result = str8_copy(allocator, str8(path.data + i + 1, path.count - i - 1));
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

internal String path_get_extension(String path) {
    Assert(path.data);
    for (u64 i = path.count - 1; i > 0; i--) {
        switch (path.data[i]) {
        case '.':
        {
            String result = str8(path.data + i + 1, path.count - i - 1);
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

internal String path_dir_name(String path) {
    String result = str8_zero();
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

internal String path_file_name(String path) {
    String result = str8_zero();
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

internal String path_remove_extension(String path) {
    String result = path;
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

internal String path_strip_dir_name(Allocator allocator, String path) {
    String result = str8_zero();
    if (path.data) {
        u64 end = path.count - 1;
        while (end) {
            if (is_separator(path.data[end - 1])) break;
            end--;
        }
        result = str8_copy(allocator, str8(path.data, end));
        
    }
    return result;
}

internal String path_strip_file_name(Allocator allocator, String path) {
    String result = str8_zero();
    if (path.data) {
        if (is_separator(path.data[path.count - 1])) {
            return result;
        }
        u64 start = path.count - 1;
        while (start > 0) {
            if (is_separator(path.data[start])) {
                result = str8_copy(allocator, str8(path.data + start + 1, path.count - start - 1));
                break;
            }
            start--;
        }
    }
    return result;
}

internal u64 path_last_segment(String path) {
    u64 result = 0;
    for (u64 i = path.count; i > 0; i -= 1) {
        if (is_separator(path.data[i - 1])) {
            result = i;
            break;
        }
    }
    return result;
}

internal String normalize_path(Allocator allocator, String path) {
    Assert(path.data);
    String result = str8_zero();
    result.data = array_alloc(allocator, u8, path.count + 1);
    result.count = 0;
    for (u64 idx = 0; idx < path.count; idx += 1) {
        //@Todo Check the last path segment even if no separator at end
        if (is_separator(path.data[idx])) {
            u64 seg_pos = path_last_segment(result);
            Rng_U64 rng = rng_u64(seg_pos, idx);
            String segment = str8_rng(path, rng);
            if (str8_match(segment, str8_lit("."), StringMatchFlag_Nil)) {
                result.count -= rng.max - rng.min;
            } else if (str8_match(segment, str8_lit(".."), StringMatchFlag_Nil)) {
                String b = result;
                b.count = rng.min - 1;
                u64 prev_seg = path_last_segment(b);
                result.count = prev_seg;
            }

            if (result.data[result.count - 1] != '/') {
                result.data[result.count] = '/';
                result.count += 1;
            }
        } else {
            result.data[result.count] = path.data[idx];
            result.count += 1;
        }
    }
    result.data[result.count] = 0;
    return result;
}

internal bool path_is_absolute(String path) {
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

internal bool path_is_relative(String path) {
    return !path_is_absolute(path);
}
