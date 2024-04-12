#include "path.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlwapi.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


char *copy_string(const char *str) {
    char *result = (char *)malloc(strlen(str) + 1);
    strcpy(result, str);
    return result;
}

char *read_entire_file(char *file_name) {
    char *result = NULL;
    HANDLE file_handle = CreateFileA((LPCSTR)file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        uint64_t bytes_to_read;
        if (GetFileSizeEx(file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            assert(bytes_to_read <= UINT32_MAX);
            result = (char *)malloc(bytes_to_read + 1);
            result[bytes_to_read] = 0;
            DWORD bytes_read;
            if (ReadFile(file_handle, result, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
            } else {
                // TODO: error handling
                printf("ReadFile: error reading file, %s!\n", file_name);
            }
       } else {
            // TODO: error handling
            printf("GetFileSize: error getting size of file: %s!\n", file_name);
       }
       CloseHandle(file_handle);
    } else {
        // TODO: error handling
        printf("CreateFile: error opening file: %s!\n", file_name);
    }
    return result;
}

#define IS_SLASH(C) ((C) == '/' || (C) == '\\')

char *path_join(char *left, char *right) {
    assert(left && right);
    size_t len = strlen(left) + strlen(right) + 1;
    char *result = (char *)malloc(len + 1);
    result[len] = 0;
    strcpy(result, left);
    strcat(result, "/");
    strcat(result, right);
    return result;
}

char *path_strip_extension(char *path) {
    assert(path);
    for (char *ptr = path + strlen(path) - 1; ptr != path; ptr--) {
        switch (*ptr) {
        case '.':
            return ptr;
        case '/':
        case '\'':
            // No extension for this file
            return NULL;
        }
    }
    return NULL;
}

char *path_strip_dir_name(char *path) {
    assert(path);
    char *ptr = path + strlen(path) - 1;
    while (ptr != path) {
        if (IS_SLASH(*ptr)) {
            break;
        }
        ptr--;
    }

    char *dir_name = NULL;
    size_t len = ptr - path;
    if (len) {
        dir_name = (char *)malloc(len + 1);
        dir_name[len] = 0;
        strncpy(dir_name, path, len);
    }
    return dir_name;
}

char *path_strip_file_name(char *path) {
    assert(path);
    char *ptr = path + strlen(path) - 1;
    if (IS_SLASH(*ptr)) {
        return NULL;
    }
    
    while (ptr != path) {
        if (IS_SLASH(*ptr)) {
            return ptr + 1;
        }
        ptr--;
    }
    return NULL;
}

char *path_strip_file_name_without_extension(char *path) {
    assert(path);
    char *file_name = path_strip_file_name(path);
    
    char *ptr;
    for (ptr = path + strlen(path) - 1; ptr != file_name; ptr--) {
        if (*ptr == '.') {
            break; 
        } else if (IS_SLASH(*ptr)) { // No extension
            return NULL;
        }
    }

    size_t len_before_dot = ptr - file_name;
    char *result = (char *)malloc(len_before_dot + 1);
    result[len_before_dot] = 0;
    strncpy(result, file_name, len_before_dot);
    return result;
}

char *path_normalize(char *path) {
    // @todo should we append current directory if the path is relative?
    // might help with dot2 being clamped to root of path
    assert(path);
    size_t len = 0;
    char *buffer = (char *)malloc(strlen(path) + 1);
    char *stream = path;

    while (*stream) {
        if (IS_SLASH(*stream)) {
            stream++;
            char *start = stream;
            bool dot = false;
            bool dot2 = false;
            if (*stream == '.') {
                stream++;
                if (*stream == '.') {
                    dot2 = true;
                    stream++;
                } else {
                    dot = true;
                }
            }

            // Check if the current dirname is actually '..' or '.' rather than some file like '.foo' or '..foo'
            if (*stream && !IS_SLASH(*stream)) {
                dot = false;
                dot2 = false;
            }

            // If not a dot dirname then just continue on like normal, reset stream to after the seperator
            if (!dot && !dot2) {
                buffer[len++] = '/';
                stream = start;
            } else if (dot2) {
                // Walk back directory, go until seperator or beginning of stream
                // @todo probably want to just clamp to root
                char *prev = buffer + len - 1;
                while (prev != buffer && !IS_SLASH(*prev)) {
                    prev--;
                }
                len = prev - buffer;
            }
        } else {
            buffer[len++] = *stream++;
        }
    }

    char *normal = (char *)malloc(len + 1);
    strncpy(normal, buffer, len);
    normal[len] = 0;
    free(buffer);
    return normal;
}

#ifdef _WIN32
char *path_home_name() {
    char buffer[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", buffer, MAX_PATH);
    char *result = (char *)malloc(strlen(buffer) + 1);
    strcpy(result, buffer);
    return result;
}
#elif defined(__linux__)
char *path_home_name() {
    char *result = NULL;
    if ((result = getenv("HOME")) == NULL) {
        // result = getpwuid(getuid())->pw_dir;
    }
    return result;
}
#endif

#ifdef _WIN32
char *path_current_dir() {
    DWORD length = GetCurrentDirectoryA(0, NULL);
    char *result = (char *)malloc(length);
    DWORD ret = GetCurrentDirectoryA(length, result);
    return result;
}
#elif defined(__linux__)
char *path_current_dir() {
    char *result = getcwd(NULL, 0);
    return result;
}
#endif

#ifdef _WIN32
bool path_file_exists(char *path) {
    return PathFileExistsA(path);
}
#endif

bool path_is_absolute(const char *path) {
    const char *ptr = path;
    switch (*ptr) {
#if defined(__linux__)
    case '~':
#endif
    case '\'':
    case '/':
        return true;
    }

    if (isalpha(*ptr++)) {
        if (*ptr == ':') {
            return true;
        }
    }
    return false;
}

bool path_is_relative(const char *path) {
    return !path_is_absolute(path);
}

char *path_get_full_path(char *path) {
    char *current_dir = path_current_dir();
    char *full_path = path_join(current_dir, path);
    char *normalized = path_normalize(full_path);
    free(current_dir);
    free(full_path);
    return normalized;
}
