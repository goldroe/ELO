#ifndef OS_CORE_H
#define OS_CORE_H

#if OS_WINDOWS
#include "win32/os_core_win32.h"
#elif OS_LINUX
#include "linux/os_core_linux.h"
#endif

typedef u64 OS_Handle;

enum OS_Access_Flags {
    OS_AccessFlag_Read     = (1<<0),
    OS_AccessFlag_Write    = (1<<1),
    OS_AccessFlag_Execute  = (1<<2),
    OS_AccessFlag_Append   = (1<<3),
};
EnumDefineFlagOperators(OS_Access_Flags);

enum OS_Key {
    OS_KEY_NIL,
    OS_KEY_A,
    OS_KEY_B,
    OS_KEY_C,
    OS_KEY_D,
    OS_KEY_E,
    OS_KEY_F,
    OS_KEY_G,
    OS_KEY_H,
    OS_KEY_I,
    OS_KEY_J,
    OS_KEY_K,
    OS_KEY_L,
    OS_KEY_M,
    OS_KEY_N,
    OS_KEY_O,
    OS_KEY_P,
    OS_KEY_Q,
    OS_KEY_R,
    OS_KEY_S,
    OS_KEY_T,
    OS_KEY_U,
    OS_KEY_V,
    OS_KEY_W,
    OS_KEY_X,
    OS_KEY_Y,
    OS_KEY_Z,

    OS_KEY_0,
    OS_KEY_1,
    OS_KEY_2,
    OS_KEY_3,
    OS_KEY_4,
    OS_KEY_5,
    OS_KEY_6,
    OS_KEY_7,
    OS_KEY_8,
    OS_KEY_9,

    OS_KEY_SPACE,
    OS_KEY_COMMA,
    OS_KEY_PERIOD,
    OS_KEY_QUOTE,

    OS_KEY_OPENBRACKET,
    OS_KEY_CLOSEBRACKET,
    OS_KEY_SEMICOLON,
    OS_KEY_SLASH,
    OS_KEY_BACKSLASH,
    OS_KEY_MINUS,
    OS_KEY_PLUS,

    OS_KEY_TAB,
    OS_KEY_TICK,

    OS_KEY_ESCAPE,
    OS_KEY_ENTER,
    OS_KEY_BACKSPACE,
    OS_KEY_DELETE,
    OS_KEY_LEFT,
    OS_KEY_RIGHT,
    OS_KEY_UP,
    OS_KEY_DOWN,

    OS_KEY_HOME,
    OS_KEY_END,
    OS_KEY_PAGEUP,
    OS_KEY_PAGEDOWN,

    OS_KEY_F1,
    OS_KEY_F2,
    OS_KEY_F3,
    OS_KEY_F4,
    OS_KEY_F5,
    OS_KEY_F6,
    OS_KEY_F7,
    OS_KEY_F8,
    OS_KEY_F9,
    OS_KEY_F10,
    OS_KEY_F11,
    OS_KEY_F12,

    OS_KEY_SUPER,

    OS_KEY_LEFTMOUSE,
    OS_KEY_MIDDLEMOUSE,
    OS_KEY_RIGHTMOUSE,

    OS_KEY_COUNT
};

enum OS_Cursor {
    OS_Cursor_Nil,
    OS_Cursor_Hidden,
    OS_Cursor_Arrow,
    OS_Cursor_Ibeam,
    OS_Cursor_Hand,
    OS_Cursor_SizeNS,
    OS_Cursor_SizeWE
};

enum OS_Event_Kind {
    OS_EventKind_Error,
    OS_EventKind_Press,
    OS_EventKind_Release,
    OS_EventKind_MouseMove,
    OS_EventKind_MouseUp,
    OS_EventKind_MouseDown,
    OS_EventKind_Scroll,
    OS_EventKind_Text,
    OS_EventKind_DropFile,
    OS_EventKind_COUNT
};

enum OS_Event_Flags {
    OS_EventFlag_Control  = (1<<0),
    OS_EventFlag_Alt      = (1<<1),
    OS_EventFlag_Shift    = (1<<2),
};
EnumDefineFlagOperators(OS_Event_Flags);

struct OS_Event {
    OS_Event *next;
    OS_Event_Kind kind = OS_EventKind_Error;
    OS_Event_Flags flags;
    OS_Key key;
    String8 text;
    V2_S32 delta;
    V2_S32 pos;
};

struct OS_Event_List {
    OS_Event *first;
    OS_Event *last;
    int count;
};

enum OS_File_Flags {
    OS_FileFlag_Nil          = 0,
    OS_FileFlag_ReadOnly     = (1<<0),
    OS_FileFlag_Hidden       = (1<<1),
    OS_FileFlag_System       = (1<<2),
    OS_FileFlag_Directory    = (1<<3),
    OS_FileFlag_Normal       = (1<<4),
};
EnumDefineFlagOperators(OS_File_Flags)

struct OS_File {
    OS_File_Flags flags;
    String8 file_name;
    u64 file_size;
    u64 creation_time;
    u64 last_access_time;
    u64 last_write_time;
};

struct OS_File_Node {
    OS_File_Node *next;
    OS_File file;
};

struct OS_File_List {
    OS_File_Node *first;
    OS_File_Node *last;
    int count;
};

internal V2_F32 os_get_window_dim(OS_Handle window_handle);
internal void os_quit_application(int exit_code);

internal OS_Handle os_find_first_file(Arena *arena, String8 path, OS_File *file);
internal bool os_find_next_file(Arena *arena, OS_Handle find_file_handle, OS_File *file);
internal void os_find_close(OS_Handle find_file_handle);

internal inline s64 get_wall_clock();
internal inline f32 get_ms_elapsed(s64 start, s64 end);

#endif // OS_CORE_H
