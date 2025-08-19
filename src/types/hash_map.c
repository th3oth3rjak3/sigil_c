// File:    hash_map.c
// Purpose: Implement hash_map.h
// Author:  Jake Hathaway
// Date:    2025-08-19

#include "src/types/hash_map.h"
#include "src/memory/memory.h"
#include "src/types/object.h"
#include "src/types/value.h"

#define TABLE_MAX_LOAD 0.75

void
init_hashmap(HashMap* hash_map) {
    hash_map->capacity = 0;
    hash_map->count = 0;
    hash_map->entries = NULL;
}

void
free_hashmap(HashMap* hash_map) {
    FREE_ARRAY(GlobalAllocator, Entry, hash_map->entries, hash_map->capacity);
    init_hashmap(hash_map);
}

static Entry*
find_entry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry*   tombstone = NULL;

    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL)
                    tombstone = entry;
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void
adjust_capacity(HashMap* hash_map, int capacity) {
    // Create a new entries array to copy into of the right size.
    Entry* new_entries = ALLOC_WITH(GlobalAllocator, Entry, capacity);
    // Pre-set all of the contents to valid values.
    for (int i = 0; i < capacity; i++) {
        new_entries[i].key = NULL;
        new_entries[i].value = NIL_VAL;
    }

    for (int i = 0; i < hash_map->capacity; i++) {
        // Existing value to copy
        Entry* entry = &hash_map->entries[i];
        if (entry->key == NULL)
            continue;

        // Location to copy to
        Entry* dest = find_entry(new_entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
    }

    FREE_ARRAY(GlobalAllocator, Entry, hash_map->entries, hash_map->capacity);
    hash_map->entries = new_entries;
    hash_map->capacity = capacity;
}

void
hashmap_copy_all(HashMap* source, HashMap* dest) {
    for (int i = 0; i < source->capacity; i++) {
        Entry* entry = &source->entries[i];
        if (entry->key != NULL) {
            hashmap_set(dest, entry->key, entry->value);
        }
    }
}

bool
hashmap_set(HashMap* hash_map, ObjString* key, Value value) {
    if (hash_map->count + 1 > hash_map->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(hash_map->capacity);
        adjust_capacity(hash_map, capacity);
    }

    Entry* entry = find_entry(hash_map->entries, hash_map->capacity, key);
    bool   is_new = entry->key == NULL;
    if (is_new)
        hash_map->count++;

    entry->key = key;
    entry->value = value;
    return is_new;
}

bool
hashmap_get(HashMap* hash_map, ObjString* key, Value* value) {
    if (hash_map->count == 0)
        return false;

    Entry* entry = find_entry(hash_map->entries, hash_map->capacity, key);
    if (entry->key == NULL)
        return false;

    *value = entry->value;
    return true;
}

bool
hashmap_delete(HashMap* hash_map, ObjString* key) {
    if (hash_map->count == 0)
        return false;

    Entry* entry = find_entry(hash_map->entries, hash_map->capacity, key);
    if (entry->key == NULL)
        return false;

    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}