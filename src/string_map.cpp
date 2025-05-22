
template <typename T>
struct String_Map_Entry {
    String_Map_Entry *prev = nullptr;
    String_Map_Entry *next = nullptr;
    String key;
    u32 hash;
    T value;
};

template <typename T>
struct String_Map_Bucket {
    String_Map_Entry<T> *first = nullptr;
    String_Map_Entry<T> *last = nullptr;
};

template <typename T>
struct String_Map {
    String_Map_Bucket<T> *buckets = nullptr;
    int bucket_count = 0;
};

template <typename T>
struct String_Map_Result {
    b32 found;
    T value;
};

internal u32 string__hash(String string) {
    u32 result = 5381;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}

template <typename T>
internal void string_map_init(String_Map<T> *string_map, int bucket_count) {
    string_map->bucket_count = bucket_count;
    string_map->buckets = new String_Map_Bucket<T>[bucket_count];
}

template <typename T>
internal void string_map_insert(String_Map<T> *map, String key, T value) {
    u32 hash = string__hash(key);
    int hash_index = hash % map->bucket_count;
    String_Map_Bucket<T> *bucket = &map->buckets[hash_index];
    for (String_Map_Entry<T> *entry = bucket->first; entry; entry = entry->next) {
        if (entry->hash == hash && entry->key == key) {
            return;
        }
    }

    String_Map_Entry<T> *entry = new String_Map_Entry<T>;
    entry->key = key;
    entry->hash = hash;
    entry->value = value;
    entry->prev = bucket->last;
    if (bucket->last) {
        bucket->last->next = entry;
    } else {
        bucket->first = bucket->last = entry;
    }
}

template <typename T>
internal String_Map_Result<T> string_map__find(String_Map<T> *string_map, String key) {
    String_Map_Result<T> result;
    result.found = false;

    u32 hash = string__hash(key);
    int hash_index = hash % string_map->bucket_count;
    String_Map_Bucket<T> *bucket = &string_map->buckets[hash_index];
    for (String_Map_Entry<T> *entry = bucket->first; entry; entry = entry->next) {
        if (entry->hash == hash && entry->key == key) {
            result.found = true;
            result.value  = entry->value;
            break;
        }
    }

    return result;
}

template <typename T>
internal bool string_map_find(String_Map<T> *string_map, String8 key, T *out_value) {
    String_Map_Result<T> search_result = string_map__find(string_map, key);
    if (search_result.found) {
        *out_value = search_result.value;
    }
    return search_result.found;
}

