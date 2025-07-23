#ifndef BASE_STRINGS_H
#define BASE_STRINGS_H

#include "base/base_core.h"
#include "base/base_memory.h"

struct String {
    u8 *data;
    u64 count;
};
typedef String String8;

typedef char *cstring;
struct cstring_header {
    u64 len;
    u64 cap;
    char data[0];
};

#define CSTRING_HEADER(str) (cstring_header *)((str) ? ((str) - offsetof(cstring_header, data)) : NULL)
#define CSTRING_LEN(str)    ((str) ? (CSTRING_HEADER(str))->len : 0)
#define CSTRING_CAP(str)    ((str) ? (CSTRING_HEADER(str))->cap : 0)

#define str8_lit(S) {(u8 *)(S), sizeof((S)) - 1}
#define str_lit(S) str8_lit(S)
#define LIT(S) (int)((S).count), (S).data

enum String_Match_Flags {
    StringMatchFlag_Nil = 0,
    StringMatchFlag_CaseInsensitive = (1<<0),
};
EnumDefineFlagOperators(String_Match_Flags);

internal u64 djb2_hash_string(String8 string);
internal u64 cstr8_length(const char *c);
internal String8 str8_zero();
internal String8 str8(u8 *c, u64 count);
internal String8 str8_cstring(const char *c);
internal String8 str8_rng(String8 string, Rng_U64 rng);

internal String8 str8_copy(Allocator allocator, String8 string);
internal String8 str8_concat(Allocator allocator, String8 first, String8 second);
internal bool str8_match(String8 first, String8 second, String_Match_Flags flags);

internal String8 str8_pushfv(Allocator allocator, const char *fmt, va_list args);
internal String8 str8_pushf(Allocator allocator, const char *fmt, ...);
internal u64 str8_find_substr(String8 string, String8 substring);

internal cstring make_cstring_len(const char *str, u64 len);
internal cstring make_cstring(const char *str);
internal void cstring_prepend(cstring *string, const char *s);
internal cstring cstring_append(cstring string, const char *s);
internal void cstring_append(cstring *string, const char *s);
internal cstring cstring_fmt(const char *fmt, ...);
internal cstring cstring_append_fmt(cstring string, const char *fmt, ...);
internal void cstring_append_fmt(cstring *string, const char *fmt, ...);

inline bool str8_equal(String8 first, String8 second) {
    return first.count == second.count && (memcmp(first.data, second.data, first.count) == 0);
}

inline bool operator==(String first, String second) {
    return str8_equal(first, second);
}

inline bool operator==(String first, const char *cstr) {
    String second = str8((u8 *)cstr, strlen(cstr));
    return str8_equal(first, second);
}


#endif // BASE_STRINGS_H
