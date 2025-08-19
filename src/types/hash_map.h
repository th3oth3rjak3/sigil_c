// File:    hash_map.h
// Purpose: Definitions for a hash map interface.
// Author:  Jake Hathaway
// Date:    2025-08-18

#pragma once

#include "src/types/object.h"
#include "src/types/value.h"

/// An entry in the hash map.
typedef struct {
    ObjString* key;   // The key used to store and find the value.
    Value      value; // The value to store and retrieve.
} Entry;

/// A hash map implementation to store keys and values.
typedef struct {
    int    count;    // The number of entries in the hash map.
    int    capacity; // The total number of allowed entries in the hash map.
    Entry* entries;  // The start of the entries.
} HashMap;

/// Initialize a new hash map.
///
/// Params:
/// - hash_map: The hash map to initialize.
void
init_hashmap(HashMap* hash_map);

/// Free the resources for the hash map.
///
/// Params:
/// - hash_map: The hash map to free the resources for.
void
free_hashmap(HashMap* hash_map);

/// Add or update the value associated with the given key in the
/// hash map.
///
/// Params:
/// - hash_map: The hash map to update.
/// - key: The key for the entry.
/// - value: The value to store with the key.
///
/// Returns:
/// - bool: True if and only if a new entry was added, otherwise false.
bool
hashmap_set(HashMap* hash_map, ObjString* key, Value value);

/// Get an element from the hash map with the corresponding key.
///
/// Params:
/// - hash_map: The hash map to get the value from.
/// - key: The key to use to search for the value.
/// - value: An output parameter that will hold the value if successfully found.
///
/// Returns:
/// - bool: True if the value was found, otherwise false.
bool
hashmap_get(HashMap* hash_map, ObjString* key, Value* value);

/// Delete an entry from the hash map with the corresponding key.
///
/// Params:
/// - hash_map: The hash map to delete the key/value pair from.
/// - key: The key used to find the entry to delete.
///
/// Returns:
/// - bool: True when the entry was found and the delete succeeds, otherwise
/// false.
bool
hashmap_delete(HashMap* hash_map, ObjString* key);

/// Copy all the contents from one hash map to another.
///
/// Params:
/// - source: The source map.
/// - dest: The destination map.
void
hashmap_copy_all(HashMap* source, HashMap* dest);