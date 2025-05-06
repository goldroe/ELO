#include "os_core_win32.h"

global OS_Key keycode_table[256];
global s64 win32_performance_frequency;

internal OS_Event_Flags os_event_flags() {
    OS_Event_Flags result = (OS_Event_Flags)0;
    if (GetKeyState(VK_MENU)  & 0x8000) result |= OS_EventFlag_Alt; 
    if (GetKeyState(VK_CONTROL) & 0x8000) result |= OS_EventFlag_Control;
    if (GetKeyState(VK_SHIFT) & 0x8000)   result |= OS_EventFlag_Shift;
    
    return result;
}

internal OS_Key os_key_from_vk(u32 vk) {
    local_persist bool first_call = true;
    if (first_call) {
        for (int i = 0; i < 26; i++) {
            keycode_table['A' + i] = (OS_Key)(OS_KEY_A + i);
        }
        for (int i = 0; i < 10; i++) {
            keycode_table['0' + i] = (OS_Key)(OS_KEY_0 + i);
        }
        keycode_table[VK_ESCAPE] = OS_KEY_ESCAPE;
        keycode_table[VK_HOME] = OS_KEY_HOME;
        keycode_table[VK_END] = OS_KEY_END;
        keycode_table[VK_SPACE] = OS_KEY_SPACE;
        keycode_table[VK_OEM_COMMA] = OS_KEY_COMMA;
        keycode_table[VK_OEM_PERIOD] = OS_KEY_PERIOD;
        keycode_table[VK_RETURN] = OS_KEY_ENTER;
        keycode_table[VK_BACK] = OS_KEY_BACKSPACE;
        keycode_table[VK_DELETE] = OS_KEY_DELETE;
        keycode_table[VK_LEFT] = OS_KEY_LEFT;
        keycode_table[VK_RIGHT] = OS_KEY_RIGHT;
        keycode_table[VK_UP] = OS_KEY_UP;
        keycode_table[VK_DOWN] = OS_KEY_DOWN;
        keycode_table[VK_PRIOR] = OS_KEY_PAGEUP;
        keycode_table[VK_NEXT] = OS_KEY_PAGEDOWN;
        keycode_table[VK_OEM_4] = OS_KEY_OPENBRACKET;
        keycode_table[VK_OEM_6] = OS_KEY_CLOSEBRACKET;
        keycode_table[VK_OEM_1] = OS_KEY_SEMICOLON;
        keycode_table[VK_OEM_2] = OS_KEY_SLASH;
        keycode_table[VK_OEM_5] = OS_KEY_BACKSLASH;
        keycode_table[VK_OEM_MINUS] = OS_KEY_MINUS;
        keycode_table[VK_OEM_PLUS] = OS_KEY_PLUS;
        keycode_table[VK_OEM_3] = OS_KEY_TICK;
        keycode_table[VK_OEM_7] = OS_KEY_QUOTE;
        for (int i = 0; i < 12; i++) {
            keycode_table[VK_F1 + i] = (OS_Key)(OS_KEY_F1 + i);
        }
    }
    return keycode_table[vk];
}

internal inline s64 get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return (s64)result.QuadPart;
}

internal inline f32 get_ms_elapsed(s64 start, s64 end) {
    f32 result = 1000.0f * ((f32)(end - start) / (f32)win32_performance_frequency);
    return result;
}
 
internal bool os_chdir(String8 path) {
    BOOL result = SetCurrentDirectory((LPCSTR)path.data);
    return result;
}

internal bool os_file_exists(String8 file_name) {
    bool result = PathFileExistsA((LPCSTR)file_name.data);
    return result;
}

internal bool os_valid_handle(OS_Handle handle) {
    bool result = (HANDLE)handle != INVALID_HANDLE_VALUE;
    return result;
}

internal bool os_create_directory(String8 path) {
    BOOL result = false;
    if (CreateDirectory((LPCSTR)path.data, NULL)) {
        result = true;
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) {
            result = true;
        }
    }
    return result;
}

internal OS_Handle os_open_file(String8 file_name, OS_Access_Flags flags) {
    DWORD access = 0, shared = 0, creation = 0;
    if (flags & OS_AccessFlag_Read)    access  |= GENERIC_READ;
    if (flags & OS_AccessFlag_Write)   access  |= GENERIC_WRITE;
    if (flags & OS_AccessFlag_Append)  access  |= GENERIC_WRITE;
    if (flags & OS_AccessFlag_Execute) access  |= GENERIC_EXECUTE;
    if (flags & OS_AccessFlag_Read)    shared   = FILE_SHARE_READ;
    if (flags & OS_AccessFlag_Write)   shared   = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    if (flags & OS_AccessFlag_Read)    creation = OPEN_EXISTING;
    if (flags & OS_AccessFlag_Write)   creation = CREATE_ALWAYS;
    if (flags & OS_AccessFlag_Append)  creation = OPEN_ALWAYS;

    HANDLE file_handle = CreateFileA((LPCSTR)file_name.data, access, shared, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
    OS_Handle handle = (OS_Handle)file_handle;
    return handle;
}

internal void os_set_file_pointer(OS_Handle file_handle, u32 position) {
    DWORD dwPosition = SetFilePointer((HANDLE)file_handle, position, NULL, FILE_BEGIN);
    if (dwPosition == INVALID_SET_FILE_POINTER) {
        DWORD err = GetLastError();
        fprintf(stderr, "SetFilePointer: error setting file pointer %d\n", err);
    }
}

internal void os_write_file(OS_Handle file_handle, void *data, u64 size) {
    DWORD bytes_written = 0;
    if (WriteFile((HANDLE)file_handle, data, (DWORD)size, &bytes_written, NULL)) {
    } else {
        DWORD error = GetLastError();
        printf("WriteFile: error writing file %d\n", error);
    }
}

internal u64 os_read_entire_file(OS_Handle file_handle, void **out_data) {
    void *data = NULL;
    u64 size = 0;
    if ((HANDLE)file_handle != INVALID_HANDLE_VALUE) {
        u64 bytes_to_read;
        if (GetFileSizeEx((HANDLE)file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            Assert(bytes_to_read <= UINT32_MAX);
            data = (u8 *)malloc(bytes_to_read);
            DWORD bytes_read;
            if (ReadFile((HANDLE)file_handle, data, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
                *out_data = data;
                size = (u64)bytes_read;
            } else {
                //@Todo Error handling
            }
        } else {
            //@Todo Error handling
        }
    } else {
        //@Todo Error handling
    }
    return size;
}

internal String8 os_read_file_string(OS_Handle file_handle) {
    u8 *data = NULL;
    u64 size = 0;
    if ((HANDLE)file_handle != INVALID_HANDLE_VALUE) {
        u64 bytes_to_read;
        if (GetFileSizeEx((HANDLE)file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            Assert(bytes_to_read <= UINT32_MAX);
            data = (u8 *)malloc(bytes_to_read + 1);
            DWORD bytes_read;
            if (ReadFile((HANDLE)file_handle, data, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
                size = (u64)bytes_read;
                data[bytes_read] = 0;
            } else {
                //@Todo Error handling
            }
        } else {
            //@Todo Error handling
        }
    } else {
        //@Todo Error handling
    }
    String8 result = str8(data, size);
    return result;
}

internal void os_close_handle(OS_Handle handle) {
    if ((HANDLE)handle != INVALID_HANDLE_VALUE) {
        CloseHandle((HANDLE)handle);
    }
}

internal void os_quit(int exit_code) {
    PostQuitMessage(exit_code);
}

internal void os_local_time(int *hour, int *minute, int *second) {
    SYSTEMTIME system_time;
    GetLocalTime(&system_time);
    if (hour) *hour = system_time.wHour;
    if (minute) *minute = system_time.wMinute; 
    if (second) *second = system_time.wSecond; 
}

internal OS_File_Flags os_file_attributes(String8 file_name) {
    OS_File_Flags flags = OS_FileFlag_Nil;
    DWORD attribs = GetFileAttributesA((char *)file_name.data);
    if (attribs & FILE_ATTRIBUTE_READONLY)    flags |= OS_FileFlag_ReadOnly;
    if (attribs & FILE_ATTRIBUTE_HIDDEN)      flags |= OS_FileFlag_Hidden;
    if (attribs & FILE_ATTRIBUTE_SYSTEM)      flags |= OS_FileFlag_System;
    if (attribs & FILE_ATTRIBUTE_DIRECTORY)   flags |= OS_FileFlag_Directory;
    if (attribs & FILE_ATTRIBUTE_NORMAL)      flags |= OS_FileFlag_Normal;
    return flags;
}

internal OS_File os_file_from_win32_data(Allocator allocator, WIN32_FIND_DATAA win32_data) {
    OS_File file{};
    file.file_name = str8_copy(allocator, str8_cstring(win32_data.cFileName));
    file.file_size = ((u64)win32_data.nFileSizeHigh<<32) | win32_data.nFileSizeLow;
    OS_File_Flags flags = OS_FileFlag_Nil;
    DWORD attribs = win32_data.dwFileAttributes;
    if (attribs & FILE_ATTRIBUTE_READONLY)    flags |= OS_FileFlag_ReadOnly;
    if (attribs & FILE_ATTRIBUTE_HIDDEN)      flags |= OS_FileFlag_Hidden;
    if (attribs & FILE_ATTRIBUTE_SYSTEM)      flags |= OS_FileFlag_System;
    if (attribs & FILE_ATTRIBUTE_DIRECTORY)   flags |= OS_FileFlag_Directory;
    if (attribs & FILE_ATTRIBUTE_NORMAL)      flags |= OS_FileFlag_Normal;
    file.flags = flags;
    return file;
}

internal OS_Handle os_find_first_file(Allocator allocator, String8 path, OS_File *file) {
    String8 find_path = str8_concat(allocator, path, str8_lit("\\*"));
    WIN32_FIND_DATAA win32_data;
    HANDLE find_file_handle = FindFirstFileA((const char *)find_path.data, &win32_data);
    if (find_file_handle) {
        *file = os_file_from_win32_data(allocator, win32_data);
    } else {
        DWORD err = GetLastError();
        printf("find_first_file ERROR '%d'\n", err);
    }
    return (OS_Handle)find_file_handle;
}

internal bool os_find_next_file(Allocator allocator, OS_Handle find_file_handle, OS_File *file) {
    if ((HANDLE)find_file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    WIN32_FIND_DATAA win32_data;
    if (FindNextFileA((HANDLE)find_file_handle, &win32_data)) {
        *file = os_file_from_win32_data(allocator, win32_data);
        return true;
    } else {
        DWORD err = GetLastError();
        if (err != ERROR_NO_MORE_FILES) {
            printf("find_next_file ERROR '%d'\n", err);
        }
        return false;
    }
}

internal void os_find_close(OS_Handle find_file_handle) {
    FindClose((HANDLE)find_file_handle);
}

internal bool os_path_exists(String8 path) {
    return PathFileExistsA((char *)path.data);
}

internal String8 os_home_path(Allocator allocator) {
    char buffer[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", buffer, MAX_PATH);
    String8 result = str8_copy(allocator, str8_cstring(buffer));
    return result;
}

internal String8 os_current_dir(Allocator allocator) {
    DWORD length = GetCurrentDirectoryA(0, NULL);
    u8 *buffer = array_alloc(allocator, u8, length + 2);
    DWORD ret = GetCurrentDirectoryA(length, (LPSTR)buffer);
    for (DWORD i = 0; i < length; i++) {
        if (buffer[i] == '\\') buffer[i] = '/';
    }
    buffer[ret] = '/';
    String8 result = str8(buffer, (u64)ret + 1);
    return result;
}

internal String8 os_exe_path(Allocator allocator) {
    char buffer[MAX_PATH] = {};
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    String8 result = path_strip_dir_name(allocator, str8((u8 *)buffer, len));
    return result;
}
