#include <stdlib.h>
#include <initializer_list>
#include <assert.h>

#include "base/base_memory.h"

#ifndef ARRAY_H
#define ARRAY_H

template <class T>
class Array {
public:
    Allocator allocator;
    T *data;
    u64 count;
    u64 capacity;

    T &operator[](size_t index) {
        assert(index < count);
        return data[index];
    }

    T* begin() { return data; }
    T* end() { return data + count; }
};

template <typename T>
internal void array__grow(Array<T> *array, size_t num) {
    size_t new_cap = 0;
    while (new_cap < array->capacity + num) {
        new_cap = 2 * new_cap + 1;
    }
    u64 size = new_cap * sizeof(T);
    T *new_mem = (T *)allocator_resize(array->allocator, array->data, size, alignof(T));
    array->data = new_mem;
    array->capacity = new_cap;
}

template <typename T>
internal void array_add(Array<T> *array, T element) {
    if (array->count + 1 > array->capacity) {
        array__grow(array, 1);
    }
    array->data[array->count] = element;
    array->count++;
}

template <typename T>
internal void array_remove(Array<T> *array, size_t index) {
    assert(index >= 0);
    assert(index < array->count);
    array->count -= 1;
    size_t len = array->count - index;
    if (len > 0) {
        T *dst = array->data + index;
        T *src = array->data + index + 1;
        memcpy(dst, src, sizeof(T) * len);
    }
}

template <typename T>
internal void array_pop(Array<T> *array) {
    assert(array->count > 0);
    array->count--;
}

template <typename T>
internal void array_clear(Array<T> *array) {
    if (array->data) {
        free(array->allocator, array->data);
        array->data = nullptr;
    }
    array->count = 0;
    array->capacity = 0;
}

template <typename T>
internal void array_reserve(Array<T> *array, size_t new_capacity) {
    assert(new_capacity > 0);
    if (array->capacity < new_capacity) {
        array__grow(array, new_capacity);
    }
}

template <typename T>
internal void array_reset_count(Array<T> *array) {
    array->count = 0;
}

template <typename T>
internal T *array_begin(Array<T> &array) {
    return array.data;
}

template <typename T>
internal T *array_end(Array<T> &array) {
    return array.data + array.count - 1;
}
    
template <typename T>
internal T& array_front(Array<T> &array) {
    assert(array.count > 0);
    return array.data[0];
}

template <typename T>
internal T& array_back(Array<T> &array) {
    assert(array.count > 0);
    return array.data[array.count - 1];
}

// template <typename T>
// void swap(Array<T> &x) {
//     Array<T> temp(this);
//     data = x.data;
//     count = x.count;
//     capacity = x.capacity;

//     x.data = temp.data;
//     x.count = temp.count;
//     x.capacity = temp.capacity;
// }


template <typename T>
internal void array_init(Array<T> *array, Allocator a) {
    array->allocator = a;
    array->data = nullptr;
    array->capacity = 0;
    array->count = 0;
}

template <typename T>
internal void array_init(Array<T> *array, Allocator a, size_t num) {
    array->allocator = a;
    array->data = nullptr;
    array->count = 0;
    array->capacity = 0;
    array__grow(&array, num);
}

template <typename T>
internal Array<T> array_make(const Allocator a) {
    Array<T> array = {a};
    return array;
}

template <typename T>
internal Array<T> array_make(Allocator a, std::initializer_list<T> init) {
    Array<T> array = {a};
    array__grow(&array, init.size());
    array.count = init.size();
    const T *ptr = init.begin();
    MemoryCopy(array.data, ptr, sizeof(T) * init.size());
    return array;
}

template <typename T>
internal T *array_find(Array<T> &array, T val) {
    for (u64 i = 0; i < array.count; i++) {
        T *ptr = &array.data[i];
        if (value == *ptr) {
            return value;
        }
    }
    return nullptr;
}

// Custom foreach iterator for custom Array container
// Maybe don't include the zero intializer? 
// Maybe put the entire declaration (typed) inside the for loop or innerscope?
#define Array_Foreach(Var, Arr) \
    if (Arr.count > 0) Var = Arr[0]; \
    else Var = {0}; \
    for (size_t _iterator = 0; _iterator < Arr.count; _iterator++, Var = Arr.data[_iterator]) \

template <typename T>
internal Array<T> array_copy(Array<T> &array) {
    Array<T> copy = {};
    copy.allocator = array.allocator;
    copy.capacity = array.capacity;
    copy.count = array.count;
    if (array.capacity > 0) {
        array.data = array_alloc(array.allocator, T, array.capacity);
    }
    return copy;
}

#endif // ARRAY_H
