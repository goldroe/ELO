internal Rng_U64 rng_u64(u64 min, u64 max) { if (min > max) Swap(u64, min, max); Rng_U64 result; result.min = min; result.max = max; return result; }
internal u64 rng_u64_len(Rng_U64 rng) { u64 result = rng.max - rng.min; return result; }

internal Rng_S64 rng_s64(s64 min, s64 max) { if (min > max) Swap(s64, min, max); Rng_S64 result; result.min = min; result.max = max; return result; }
internal s64 rng_s64_len(Rng_S64 rng) { s64 result = rng.max - rng.min; return result; }

internal inline Rect make_rect(f32 x, f32 y, f32 w, f32 h) {Rect result = {x, y, x + w, y + h}; return result;}
internal inline Rect make_rect(V2_F32 p, V2_F32 dim) {Rect result = {p.x, p.y, p.x + dim.x, p.y + dim.y}; return result;}
internal inline Rect make_rect_center(V2_F32 position, V2_F32 size) {Rect result = make_rect(position.x - size.x/2.0f, position.y - size.y/2.0f, size.x, size.y); return result;}
internal inline Rect rect_zero() {Rect result = {0, 0, 0, 0}; return result;}

internal void shift_rect(Rect *rect, f32 x, f32 y) { rect->x0 += x; rect->x1 += x; rect->y0 += y; rect->y1 += y; }

internal V2_F32 rect_dim(Rect rect) {V2_F32 result; result.x = rect.x1 - rect.x0; result.y = rect.y1 - rect.y0; return result;}
internal inline f32 rect_height(Rect rect) {f32 result = rect.y1 - rect.y0; return result;}
internal inline f32 rect_width(Rect rect) {f32 result = rect.x1 - rect.x0; return result;}

internal inline bool operator==(Rect a, Rect b) {
    return a.x0 == b.x0 && a.x1 == b.x1 && a.y0 == b.y0 && a.y1 == b.y1;
}
internal inline bool operator!=(Rect a, Rect b) {
    return !(a == b);
}

internal bool rect_contains(Rect rect, V2_F32 v) {
    bool result = v.x >= rect.x0 &&
        v.x <= rect.x1 &&
        v.y >= rect.y0 &&
        v.y <= rect.y1;
    return result; 
}

internal Axis2 axis_flip(Axis2 axis) {Axis2 result; if (axis == Axis_X) result = Axis_Y; else result = Axis_X; return result;}

internal inline bool operator==(V3_S32 left, V3_S32 right) {return left.x == right.x && left.y == right.y && left.z == right.z;}
internal inline bool operator!=(V3_S32 left, V3_S32 right) {return !(left == right);}

internal inline RGBA make_rgba(u8 r, u8 g, u8 b, u8 a) {
    RGBA result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;
    return result;
}
