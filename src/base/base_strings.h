#ifndef BASE_STRINGS_H
#define BASE_STRINGS_H

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

internal String8 str8(u8 *c, u64 count);
internal String8 str8_zero();
internal u64 cstr8_length(const char *c);
internal String8 str8_cstring(const char *c);

#endif // BASE_STRINGS_H
