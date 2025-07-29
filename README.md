## ELO
A generic-purpose programming language for building software.

Built around the simplicity of C yet with more convenience.

### Hello World

```elo
#import "libc/stdio.elo";

main :: () {
  printf("Hello, World.\n");
}
```

### Usage
Invoke ELO.exe and specify source files.
 - ` > ELO.exe [file..] options`

Outputs executable and object file using the LLVM backend.
 

Compiler options:
- Link Library:    -l{file.lib} (e.g -luser32.lib)
- Dump IR (LLVM):  -dump_ir

### Build
Depends on LLVM binaries (version 18.1.8). Build LLVM and place in bin folder.

Run `cmake -S . -B build`

## Quick Start


### Compile-Time Constructs

#### Builtin Types
```elo
  void
  bool
  int, i8, i16, i32, i64
  uint, u8, u16, u32, u64
  isize, usize
  string, cstring
  any
```

#### Constants
```elo
A :: 100;
B :: "abcd";
C :: A + 100;
D :: 9.8;
```

#### Procedure
```elo
factorial :: (n: int) -> int {
  if n == 0 {
    return 1;
  }
  return n * factorial(n - 1);
}

swap :: (a: int, b: int) -> int, int {
  return b, a;
}

FN :: (x: int) -> int;

```

#### Enum
```elo
Date :: enum {
  Monday,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
  Saturday,
  Sunday
}
```

#### Struct
```elo
Foo :: struct {
  E :: enum {
    A = 0,
    B,
    C,
    D = 1024,
    E = D
  }

  CONSTANT :: 100;

  a, b, c: int;
  e: E;

  proc :: (foo: *Foo) -> int {
    return foo.a + foo.b + foo.c;
  }
}
```

#### Union
```elo
Vector2 :: union {
  struct { x, y: f32; }
  struct { u, v: f32; }
  elements: [2]f32;
}
```

### Statements

#### If Statement
```elo
if <condition>
  <body>
else if <condition>
  <body>
else
  <body>
```

#### Ifcase statement
```elo
ifcase <condition> {
case <expr>:
  <body>
case <expr>..<expr> //range
  <body>
case:               //default
  <body>
}

// if-else chain
ifcase {
case <expr>: //if
  <body>
case <expr>: //else if
  <body>
case:        //else
  <body>
}
```

#### While Statement
```elo
while <condition>?
  <body>
```

#### For Statement
```elo
for <init> ; <condition> ; <post>
  <body>
```

#### Range Statement
```elo
for index, value in <expr>
  <body>
for value in <expr>..<expr>
  <body>
```
