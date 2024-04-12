#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef float float32;
typedef double float64;

struct Source_Range {
    int line_begin;
    int line_end;
    int column_begin;
    int column_end;
    int char_begin;
    int char_end;
};

#endif // TYPES_H
