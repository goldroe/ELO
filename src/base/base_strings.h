#ifndef BASE_STRINGS_H
#define BASE_STRINGS_H

#include "base/base_core.h"
#include "base/base_memory.h"

struct String {
    u8 *data;
    u64 count;
};

typedef char *CString;
struct CString_Header {
    Allocator allocator;
    u64 length;
    u64 capacity;
};

#define CSTRING_HEADER(Str)  ((CString_Header *)(Str) - 1)

// typedef char *cstring;
// struct cstring_header {
//     u64 len;
//     u64 cap;
//     char data[0];
// };

// #define CSTRING_HEADER(str) (cstring_header *)((str) ? ((str) - offsetof(cstring_header, data)) : NULL)
// #define CSTRING_LEN(str)    ((str) ? (CSTRING_HEADER(str))->len : 0)
// #define CSTRING_CAP(str)    ((str) ? (CSTRING_HEADER(str))->cap : 0)

#define str_lit(S) {(u8 *)(S), sizeof((S)) - 1}
#define LIT(S) (int)((S).count), (S).data

enum String_Match_Flags {
    StringMatchFlag_Nil = 0,
    StringMatchFlag_CaseInsensitive = (1<<0),
};
EnumDefineFlagOperators(String_Match_Flags);

u64 djb2_hash_string(String string);
u64 cstr8_length(const char *c);
String str8_zero();
String str8(u8 *c, u64 count);
String str8_cstring(const char *c);
String str8_rng(String string, Rng_U64 rng);

String str8_copy(Allocator allocator, String string);
String str8_concat(Allocator allocator, String first, String second);
bool str8_match(String first, String second, String_Match_Flags flags);

String str8_pushfv(Allocator allocator, const char *fmt, va_list args);
String str8_pushf(Allocator allocator, const char *fmt, ...);
u64 str8_find_substr(String string, String substring);

CString cstring_make(Allocator allocator, const char *str, u64 length);
CString cstring_make(Allocator allocator, const char *str);
CString cstring_make_fmt(Allocator allocator, const char *fmt, ...);
void string_free(CString string);
CString string_append(CString string, const char *other_str, u64 other_len);
CString string_append(CString string, const char *s);
CString string_append_fmt(CString string, const char *fmt, ...);

inline bool str8_equal(String first, String second) {
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
