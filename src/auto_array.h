#ifndef AUTO_ARRAY_H
#define AUTO_ARRAY_H

#include <stdlib.h>
#include <initializer_list>
#include <assert.h>

template <class T>
class Auto_Array {
public:
    T *data = nullptr;
    u64 count = 0;
    u64 capacity = 0;

    void reserve(size_t num_elements);

    Auto_Array() {
        data = nullptr;
        count = 0;
        capacity = 0;
    }

    Auto_Array(const Auto_Array<T> &array) {
        data = array.data;
        count = array.count;
        capacity = array.capacity;
    }

    void grow(size_t num_elements) {
        size_t new_cap = 0;
        while (new_cap < capacity + num_elements) {
            new_cap = 2 * new_cap + 1;
        }
        data = (T *)realloc(data, new_cap * sizeof(T));
        capacity = new_cap;
    }

    void pop() {
        assert(count > 0);
        count--;
    }

    T* push(T element) {
        if (count + 1 > capacity) {
            grow(1);
        }
        data[count] = element;
        count++;
        return &data[count - 1];
    }

    T &operator[](size_t index) {
        assert(index < count);
        return data[index];
    }

    bool is_empty() { return count == 0; }

    void clear() {
        if (data) free(data);
        data = nullptr;
        capacity = count = 0;
    }

    void reset_count() {
        count = 0;
    }
    
    T* begin() {
        return data;
    }
    T* end() {
        return data + count;
    }

    T& front() {
        assert(count > 0);
        return data[0];
    }

    T& back() {
        assert(count > 0);
        return data[count - 1];
    }

    void swap(Auto_Array<T> &x) {
        Auto_Array<T> temp(this);
        data = x.data;
        count = x.count;
        capacity = x.capacity;

        x.data = temp.data;
        x.count = temp.count;
        x.capacity = temp.capacity;
    }

    void remove(size_t index);
};

template<typename T>
void Auto_Array<T>::reserve(size_t num_elements) {
    data = (T *)malloc(num_elements * sizeof(T));
    capacity = num_elements;
}

// Custom foreach iterator for custom Array container
// Maybe don't include the zero intializer? 
// Maybe put the entire declaration (typed) inside the for loop or innerscope?
#define Array_Foreach(Var, Arr) \
    if (Arr.count > 0) Var = Arr[0]; \
    else Var = {0}; \
    for (size_t _iterator = 0; _iterator < Arr.count; _iterator++, Var = Arr.data[_iterator]) \

template<typename T>
void Auto_Array<T>::remove(size_t index) {
    assert(index >= 0 && index < count);
    count -= 1;
    size_t len = count - index;
    if (len > 0) {
        T *dst = data + index;
        T *src = data + index + 1;
        memcpy(dst, src, sizeof(T) * len);
    }
}

template<typename T>
internal void array_copy(Auto_Array<T> *dst, Auto_Array<T> const &src) {
    dst->data = (T*)malloc(sizeof(T) * src.capacity);
    dst->count = src.count;
    dst->capacity = src.capacity;
    memcpy(dst->data, src.data, sizeof(T) * src.count);
}

#endif // AUTO_ARRAY_H
