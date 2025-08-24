// File:    hash_map.c
// Purpose: Implement hash_map.h
// Author:  Jake Hathaway
// Date:    2025-08-19

#include "hash_map.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void
init_hashmap(HashMap* hash_map) {
    hash_map->capacity = 0;
    hash_map->count = 0;
    hash_map->entries = NULL;
}

void
free_hashmap(HashMap* hash_map) {
    FREE_ARRAY(Entry, hash_map->entries, hash_map->capacity);
    init_hashmap(hash_map);
}

static Entry*
find_entry(Entry* entries, int capacity, const ObjString* key) {
    uint32_t index = key->hash & (capacity - 1);
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

        index = (index + 1) & (capacity - 1);
    }
}

static void
adjust_capacity(HashMap* hash_map, int capacity) {
    // Create a new entries array to copy into of the right size.
    Entry* new_entries = ALLOCATE(Entry, capacity);
    // Pre-set all of the contents to valid values.
    for (int i = 0; i < capacity; i++) {
        new_entries[i].key = NULL;
        new_entries[i].value = NIL_VAL;
    }

    hash_map->count = 0;
    for (int i = 0; i < hash_map->capacity; i++) {
        // Existing value to copy
        Entry* entry = &hash_map->entries[i];
        if (entry->key == NULL)
            continue;

        // Location to copy to
        Entry* dest = find_entry(new_entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        hash_map->count++;
    }

    FREE_ARRAY(Entry, hash_map->entries, hash_map->capacity);
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
    if (is_new && IS_NIL(entry->value))
        hash_map->count++;

    entry->key = key;
    entry->value = value;
    return is_new;
}

bool
hashmap_get(HashMap* hash_map, const ObjString* key, Value* value) {
    if (hash_map->count == 0)
        return false;

    const Entry* entry = find_entry(hash_map->entries, hash_map->capacity, key);
    if (entry->key == NULL)
        return false;

    *value = entry->value;
    return true;
}

bool
hashmap_delete(HashMap* hash_map, const ObjString* key) {
    if (hash_map->count == 0)
        return false;

    Entry* entry = find_entry(hash_map->entries, hash_map->capacity, key);
    if (entry->key == NULL)
        return false;

    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

ObjString*
hashmap_find_string(
    HashMap* hash_map, const char* chars, int length, uint32_t hash) {
    if (hash_map->count == 0) {
        return NULL;
    }

    uint32_t index = hash & (hash_map->capacity - 1);
    for (;;) {
        Entry* entry = &hash_map->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value))
                return NULL;
        } else if (
            entry->key->length == length && entry->key->hash == hash
            && memcmp(entry->key->chars, chars, length) == 0) {
            // We found it!
            return entry->key;
        }

        index = (index + 1) & (hash_map->capacity - 1);
    }
}

void
mark_hashmap(HashMap* hash_map) {
    for (int i = 0; i < hash_map->capacity; i++) {
        Entry* entry = &hash_map->entries[i];
        mark_object((Obj*)entry->key);
        mark_value(entry->value);
    }
}

void
hashmap_remove_white(HashMap* hash_map) {
    for (int i = 0; i < hash_map->capacity; i++) {
        Entry* entry = &hash_map->entries[i];
        if (entry->key != NULL && !entry->key->obj.is_marked) {
            hashmap_delete(hash_map, entry->key);
        }
    }
}