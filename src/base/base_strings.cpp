#include <cctype>
#include <cstdarg>

#include "base/base_core.h"
#include "base/base_memory.h"
#include "base_strings.h"

#include <stb_sprintf.h>

u64 cstr8_length(const char *c) {
    if (c == nullptr) return 0;
    u64 result = 0;
    while (*c++) {
        result++;
    }
    return result;
}

String str8_zero() {
    String result = {0, 0};
    return result;
}

String str8(u8 *c, u64 count) {
    String result = {(u8 *)c, count};
    return result;
}

String str8_cstring(const char *c) {
    String result;
    result.count = cstr8_length(c);
    result.data = (u8 *)c;
    return result;
}

String str8_rng(String string, Rng_U64 rng) {
    String result;
    result.data = string.data + rng.min;
    result.count = rng.max - rng.min;
    return result;
}

String str8_copy(Allocator allocator, String string) {
    String result;
    result.count = string.count;
    result.data = array_alloc(allocator, u8, result.count + 1);
    MemoryCopy(result.data, string.data, string.count);
    result.data[result.count] = 0;
    return result;
}

String str8_concat(Allocator allocator, String first, String second) {
    String result;
    result.count = first.count + second.count;
    result.data = array_alloc(allocator, u8, result.count + 1);
    MemoryCopy(result.data, first.data, first.count);
    MemoryCopy(result.data + first.count, second.data, second.count);
    result.data[result.count] = 0;
    return result;
}

bool str8_match(String first, String second, String_Match_Flags flags) {
    if (first.count != second.count) return false;
    u8 a, b;
    for (u64 i = 0; i < first.count; i++) {
        a = first.data[i];
        b = second.data[i];
        if (flags & StringMatchFlag_CaseInsensitive) {
            a = (u8)tolower(a);
            b = (u8)tolower(b);
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

String str8_pushfv(Allocator allocator, const char *fmt, va_list args) {
    va_list args_;
    va_copy(args_, args);
    String result;
    int bytes = stbsp_vsnprintf(NULL, NULL, fmt, args_) + 1;
    result.data = array_alloc(allocator, u8, bytes);
    result.count = stbsp_vsnprintf((char *)result.data, bytes, fmt, args_);
    result.data[result.count] = 0;
    va_end(args_);
    return result;
}

String str8_pushf(Allocator allocator, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String result;
    int bytes = stbsp_vsnprintf(NULL, NULL, fmt, args) + 1;
    result.data = array_alloc(allocator, u8, bytes);
    result.count = stbsp_vsnprintf((char *)result.data, bytes, fmt, args);
    result.data[result.count] = 0;
    va_end(args);
    return result;
}

String str8_jump(String string, u64 count) {
    String result;
    result.data = string.data + count;
    result.count = string.count - count;
    return result;
}

u64 str8_find_substr(String string, String substring) {
    if (substring.count > string.count) {
        return string.count;
    }

    u64 result = string.count;
    for (u64 string_cursor = 0; string_cursor < string.count; string_cursor++) {
        u64 rem = string.count - string_cursor;
        if (rem < substring.count) {
            break;
        }
        
        if (string.data[string_cursor] == substring.data[0]) {
            int cmp = memcmp(string.data + string_cursor, substring.data, substring.count);
            if (cmp == 0) {
                result = string_cursor;
                break;
            }
        }
    }
    return result;
}

u64 djb2_hash_string(String string) {
    u64 result = 5381;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}

CString cstring_make(Allocator allocator, const char *str, u64 length) {
    CString_Header *header = (CString_Header *)alloc(allocator, sizeof(CString_Header) + length + 1);
    header->allocator = allocator;
    header->length = length;
    header->capacity = length;
    CString string = (char *)header + sizeof(CString_Header);
    MemoryCopy(string, str, length);
    string[length] = 0;
    return string;
}

CString cstring_make(Allocator allocator, const char *str) {
    u64 len = str ? strlen(str): 0;
    return cstring_make(allocator, str, len);
}

CString cstring_make_fmt(Allocator allocator, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = stbsp_vsnprintf(NULL, NULL, fmt, args);
    char *str = (char *)malloc(len + 1);
    stbsp_vsnprintf(str, len + 1, fmt, args);
    CString string = cstring_make(allocator, str, len);
    va_end(args);
    return string;
}

void string_free(CString string) {
    CString_Header *header = CSTRING_HEADER(string);
    Allocator allocator = header->allocator;
    free(allocator, header);
}

CString string_append(CString string, const char *other_str, u64 other_len) {
    if (other_len > 0) {
        CString_Header *header = CSTRING_HEADER(string);
        Allocator a  = header->allocator;
        u64 curr_len = header->length;
        u64 curr_cap = header->capacity;
        u64 new_len = curr_len + other_len;
        u64 new_cap = curr_cap;

        if (new_len > curr_cap) {
            new_cap = new_len * 2 + 1;

            CString_Header *new_header = (CString_Header *)allocator_resize(a, header, sizeof(CString_Header) + new_cap +  1, DEFAULT_MEMORY_ALIGNMENT);
            header = new_header;
            string = (char *)header + sizeof(CString_Header);
        }

        MemoryCopy(string + curr_len, other_str, other_len);
        string[new_len] = 0;
        header->length = new_len;
        header->capacity = new_cap;
    }

    return string;
}

CString string_append(CString string, const char *s) {
    u64 length = s ? strlen(s) : 0;
    return string_append(string, s, length);
}

CString string_append_fmt(CString string, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = stbsp_vsnprintf(NULL, NULL, fmt, args);
    char *str_fmt = (char *)malloc(len + 1);
    stbsp_vsnprintf(str_fmt, len + 1, fmt, args);
    va_end(args);
    return string_append(string, str_fmt, len);
}

// cstring cstring__prepend(cstring string, const char *s) {
//     cstring result = string;
//     cstring_header *old_header = string ? CSTRING_HEADER(string) : nullptr;
//     u64 len = old_header ? old_header->len : 0;
//     u64 s_len = s ? strlen(s) : 0;
//     u64 new_len = len + s_len;
//     u64 cap = old_header ? old_header->cap : 0;
//     u64 new_cap = cap;
//     if (len + s_len > cap) {
//         new_cap = new_len * 2 + 1;
//         cstring_header *new_header = (cstring_header *)realloc(old_header, offsetof(cstring_header, data) + new_cap + 1);
//         result = (cstring)new_header->data;
//     }

//     cstring__set_len(result, new_len);
//     cstring__set_cap(result, new_cap);

//     if (string) {
//         char *buffer = (char *)malloc(len);
//         MemoryCopy(buffer, string, len);
//         MemoryCopy(result, s, s_len);
//         MemoryCopy(result + s_len, buffer, len);
//         free(buffer);
//     } else {
//         MemoryCopy(result, s, s_len);
//     }

//     result[new_len] = 0;
//     return result;
// }

// void cstring_prepend(cstring *string, const char *s) {
//     *string = cstring__prepend(*string, s);
// }

// cstring cstring_append(cstring string, const char *s) {
//     cstring result;
//     if (string) {
//         result = cstring__append(string, s);
//     } else {
//         result = make_cstring(s);
//     }
//     return result;
// }

