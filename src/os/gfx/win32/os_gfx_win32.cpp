HCURSOR win32_hcursor;
Arena *win32_event_arena;
OS_Event_List win32_events;
bool win32_resizing;

internal void os_post_quit_message(int exit_code) {
    PostQuitMessage(exit_code);
}

internal OS_Event *win32_push_event(OS_Event_Kind kind) {
    OS_Event *result = push_array(win32_event_arena, OS_Event, 1);
    result->kind = kind;
    result->next = nullptr;
    result->flags = os_event_flags();
    SLLQueuePush(win32_events.first, win32_events.last, result);
    win32_events.count += 1;
    return result;
}

internal LRESULT CALLBACK win32_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    OS_Event *event = nullptr;
    bool release = false;

    LRESULT result = 0;
    switch (Msg) {
    case WM_MOUSEWHEEL:
    {
        event = win32_push_event(OS_EventKind_Scroll);
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        event->delta.y = delta;
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam); 
        int y = GET_Y_LPARAM(lParam); 
        event = win32_push_event(OS_EventKind_MouseMove);
        event->pos.x = x;
        event->pos.y = y;
        break;
    }

    case WM_SETCURSOR:
    {
        SetCursor(win32_hcursor);
        break;
    }

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        release = true;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        event = win32_push_event(release ? OS_EventKind_MouseUp : OS_EventKind_MouseDown);
        switch (Msg) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
            event->key = OS_KEY_LEFTMOUSE;
            break;
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN:
            event->key = OS_KEY_MIDDLEMOUSE;
            break;
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
            event->key = OS_KEY_RIGHTMOUSE; 
            break;
        }
        int x = GET_X_LPARAM(lParam); 
        int y = GET_Y_LPARAM(lParam); 
        event->pos.x = x;
        event->pos.y = y;
        if (release) {
            ReleaseCapture();
        } else {
            SetCapture(hWnd);
        }
        break;
    }

    case WM_SYSCHAR:
    {
        // result = DefWindowProcA(hWnd, Msg, wParam, lParam);
        break;
    }
    case WM_CHAR:
    {
        u16 vk = wParam & 0x0000ffff;
        u8 c = (u8)vk;
        if (c == '\r') c = '\n';
        if (c >= 32 && c != 127) {
            event = win32_push_event(OS_EventKind_Text);
            event->text = str8_copy(win32_event_arena, str8(&c, 1));
        }
        break;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP:
        release = true;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        u8 down = ((lParam >> 31) == 0);
        u8 alt_mod = (lParam & (1 << 29)) != 0;
        u32 virtual_keycode = (u32)wParam;
        if (virtual_keycode < 256) {
            event = win32_push_event(release ? OS_EventKind_Release : OS_EventKind_Press);
            event->key = os_key_from_vk(virtual_keycode);
        }
        break;
    }

    case WM_SIZE:
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        break;
    }

    case WM_DPICHANGED:
    {
        WORD y = HIWORD(wParam);
        WORD x = LOWORD(wParam);
        RECT *rect = (RECT *)lParam;
        break;
    }

    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        UINT len = DragQueryFileA(hDrop, 0, NULL, 0);
        String8 string = str8_zero();
        string.data = push_array(win32_event_arena, u8, len + 1);
        string.count = len;
        DragQueryFileA(hDrop, 0, (LPSTR)string.data, len + 1);
        event = win32_push_event(OS_EventKind_DropFile);
        event->text = string;
        DragFinish(hDrop);
        break;
    }

    // case WM_SIZING:
    // {
    //     win32_resizing = true;
    //     break;
    // }
    // case WM_ENTERSIZEMOVE:
    // {
    //     win32_resizing = true;
    //     break;
    // }
    // case WM_EXITSIZEMOVE:
    // {
    //     win32_resizing = false;
    //     break;
    // }
    
    case WM_CREATE:
    {
        break;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
    return result;
}

internal String8 os_get_clipboard_text(Arena *arena) {
    String8 result = str8_zero();
    if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(0)) {
        HANDLE handle = GetClipboardData(CF_TEXT);
        if (handle) {
            u8 *buffer = (u8 *)GlobalLock(handle);
            if (buffer) {
                result = str8_copy(arena, str8_cstring((char *)buffer));
                GlobalUnlock(handle);
            }
        }
        CloseClipboard();
    }
    return result;
}

internal void os_set_clipboard_text(String8 text) {
    if (OpenClipboard(0)) {
        HANDLE handle = GlobalAlloc(GMEM_MOVEABLE, text.count + 1);
        if (handle) {
            u8 *buffer = (u8 *)GlobalLock(handle);
            MemoryCopy(buffer, text.data, text.count);
            buffer[text.count] = 0;
            GlobalUnlock(handle);
            SetClipboardData(CF_TEXT, handle);
        }
    }
}

internal void os_set_cursor(OS_Cursor cursor) {
    local_persist HCURSOR hcursor;
    switch (cursor) {
    default:
    case OS_Cursor_Hidden:
        hcursor = NULL;
        break;
    case OS_Cursor_Arrow:
        hcursor = LoadCursorA(NULL, IDC_ARROW);
        break;
    case OS_Cursor_Ibeam:
        hcursor = LoadCursorA(NULL, IDC_IBEAM);
        break;
    case OS_Cursor_Hand:
        hcursor = LoadCursorA(NULL, IDC_HAND);
        break;
    case OS_Cursor_SizeNS:
        hcursor = LoadCursorA(NULL, IDC_SIZENS);
        break;
    case OS_Cursor_SizeWE:
        hcursor = LoadCursorA(NULL, IDC_SIZEWE);
        break;
    }
    
    if (win32_hcursor != hcursor) {
        PostMessageA(0, WM_SETCURSOR, 0, 0);
        win32_hcursor = hcursor;
    }
}

internal Rect os_client_rect_from_window(OS_Handle window_handle) {
    RECT client_rect;
    GetClientRect((HWND)window_handle, &client_rect);
    Rect result;
    result.x0 = (f32)client_rect.left;
    result.x1 = (f32)client_rect.right;
    result.y0 = (f32)client_rect.top;
    result.y1 = (f32)client_rect.bottom;
    return result;
}

internal V2_F32 os_get_window_dim(OS_Handle window_handle) {
    V2_F32 result{};
    RECT rect;
    int width = 0, height = 0;
    if (GetClientRect((HWND)window_handle, &rect)) {
        result.x = (f32)(rect.right - rect.left);
        result.y = (f32)(rect.bottom - rect.top);
    }
    return result;
}

internal V2_F32 os_mouse_from_window(OS_Handle window_handle) {
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient((HWND)window_handle, &pt);
    V2_F32 result;
    result.x = (f32)pt.x;
    result.y = (f32)pt.y;
    return result;
}

internal bool os_window_is_focused(OS_Handle window_handle) {
    HWND active_hwnd = GetActiveWindow();
    bool result = (OS_Handle)active_hwnd == window_handle;
    return result;
}
