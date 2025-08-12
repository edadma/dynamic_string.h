/*
 * dynamic_string.h - v0.0.1 - Immutable reference counted string library
 *
 * DUAL LICENSED under your choice of:
 * - MIT License (see LICENSE-MIT)
 * - The Unlicense (see LICENSE-UNLICENSE)
 *
 * USAGE:
 *   In exactly one C file, #define DS_IMPLEMENTATION before including this file:
 *
 *   #define DS_IMPLEMENTATION
 *   #include "dynamic_string.h"
 *
 *   In all other files, just include normally:
 *   #include "dynamic_string.h"
 *
 * EXAMPLE:
 *   ds_string str = ds_create("Hello");
 *   ds_string copy = ds_retain(str);      // Share the same data
 *   ds_string str2 = ds_append(str, " World!");  // Creates new string
 *   printf("%s\n", ds_cstr(str));        // prints "Hello"
 *   printf("%s\n", ds_cstr(str2));       // prints "Hello World!"
 *   ds_release(&str);   // Decrements ref count
 *   ds_release(&copy);  // Decrements ref count
 *   ds_release(&str2);  // Last reference - frees memory
 */

#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration macros (user can override before including)
#ifndef DS_MALLOC
#define DS_MALLOC malloc
#endif

#ifndef DS_REALLOC
#define DS_REALLOC realloc
#endif

#ifndef DS_FREE
#define DS_FREE free
#endif

#ifndef DS_ASSERT
#include <assert.h>
#define DS_ASSERT assert
#endif

// API macros
#ifdef DS_STATIC
#define DS_DEF static
#else
#define DS_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// INTERFACE
// ============================================================================

// Internal data structure (users shouldn't access directly)
typedef struct ds_data {
    size_t length;
    size_t ref_count;
    char array[]; // Flexible array member - the actual string data
} ds_data;

typedef struct {
    ds_data* data;
} ds_string;

// Special null string constant
extern const ds_string DS_NULL_STRING;

// Core reference counting functions
DS_DEF ds_string ds_create(const char* text);
DS_DEF ds_string ds_create_length(const char* text, size_t length);
DS_DEF ds_string ds_retain(ds_string str);
DS_DEF void ds_release(ds_string* str);

// String operations (return new strings - immutable)
DS_DEF ds_string ds_append(ds_string str, const char* text);
DS_DEF ds_string ds_append_char(ds_string str, uint32_t codepoint);
DS_DEF ds_string ds_prepend(ds_string str, const char* text);
DS_DEF ds_string ds_insert(ds_string str, size_t index, const char* text);
DS_DEF ds_string ds_substring(ds_string str, size_t start, size_t len);

// String concatenation
DS_DEF ds_string ds_concat(ds_string a, ds_string b);
DS_DEF ds_string ds_join(ds_string* strings, size_t count, const char* separator);

// Utility functions (read-only)
DS_DEF size_t ds_length(ds_string str);
DS_DEF const char* ds_cstr(ds_string str);
DS_DEF int ds_compare(ds_string a, ds_string b);
DS_DEF int ds_find(ds_string str, const char* needle);
DS_DEF int ds_starts_with(ds_string str, const char* prefix);
DS_DEF int ds_ends_with(ds_string str, const char* suffix);

// Reference count inspection
DS_DEF size_t ds_ref_count(ds_string str);
DS_DEF int ds_is_shared(ds_string str);
DS_DEF int ds_is_empty(ds_string str);

// Unicode codepoint iteration (Rust-style)
typedef struct {
    const char* data;
    size_t pos; // Current byte position
    size_t end; // End byte position
} ds_codepoint_iter;

DS_DEF ds_codepoint_iter ds_codepoints(ds_string str);
DS_DEF uint32_t ds_iter_next(ds_codepoint_iter* iter);
DS_DEF int ds_iter_has_next(const ds_codepoint_iter* iter);

// Unicode utility functions
DS_DEF size_t ds_codepoint_length(ds_string str);
DS_DEF uint32_t ds_codepoint_at(ds_string str, size_t index);

// Convenience macros for common operations
#define ds_empty() ds_create("")
#define ds_from_literal(lit) ds_create(lit)

// ============================================================================
// STRINGBUILDER - Mutable builder for efficient string construction
// ============================================================================

typedef struct {
    ds_data* data; // Internal mutable string data
    size_t capacity; // Capacity for growth (length is in data->length)
} ds_stringbuilder;

// StringBuilder creation and management
DS_DEF ds_stringbuilder ds_sb_create(void);
DS_DEF ds_stringbuilder ds_sb_create_with_capacity(size_t capacity);
DS_DEF void ds_sb_destroy(ds_stringbuilder* sb);

// Mutable operations (modify the builder in-place)
DS_DEF int ds_sb_append(ds_stringbuilder* sb, const char* text);
DS_DEF int ds_sb_append_char(ds_stringbuilder* sb, uint32_t codepoint);
DS_DEF int ds_sb_append_string(ds_stringbuilder* sb, ds_string str);
DS_DEF int ds_sb_insert(ds_stringbuilder* sb, size_t index, const char* text);
DS_DEF void ds_sb_clear(ds_stringbuilder* sb);

// Conversion to immutable string (the magic happens here!)
DS_DEF ds_string ds_sb_to_string(ds_stringbuilder* sb);

// StringBuilder inspection
DS_DEF size_t ds_sb_length(const ds_stringbuilder* sb);
DS_DEF size_t ds_sb_capacity(const ds_stringbuilder* sb);
DS_DEF const char* ds_sb_cstr(const ds_stringbuilder* sb);

// Legacy compatibility (for gradual migration)
DS_DEF void ds_free(ds_string* str); // Alias for ds_release

#ifdef __cplusplus
}
#endif

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#ifdef DS_IMPLEMENTATION

// Global null string constant
const ds_string DS_NULL_STRING = {NULL};

// Internal helper functions
static ds_data* ds_data_create(const size_t length) {
    // Single allocation: struct + string data + null terminator
    ds_data* data = DS_MALLOC(sizeof(ds_data) + length + 1);
    if (!data)
        return NULL;

    data->length = length;
    data->ref_count = 1;
    data->array[length] = '\0'; // Always null-terminate

    return data;
}

static void ds_data_destroy(ds_data* data) {
    if (data) {
        DS_FREE(data); // Single free - no separate buffer allocation
    }
}

DS_DEF ds_string ds_create(const char* text) {
    if (!text) {
        return DS_NULL_STRING;
    }

    const size_t len = strlen(text);
    return ds_create_length(text, len);
}

DS_DEF ds_string ds_create_length(const char* text, const size_t length) {
    ds_string str;

    str.data = ds_data_create(length);
    if (!str.data) {
        return DS_NULL_STRING;
    }

    if (text && length > 0) {
        memcpy(str.data->array, text, length);
    }

    return str;
}

DS_DEF ds_string ds_retain(const ds_string str) {
    if (str.data) {
        str.data->ref_count++;
    }
    return str;
}

DS_DEF void ds_release(ds_string* str) {
    if (str && str->data) {
        str->data->ref_count--;
        if (str->data->ref_count == 0) {
            ds_data_destroy(str->data);
        }
        str->data = NULL;
    }
}

DS_DEF ds_string ds_append(const ds_string str, const char* text) {
    if (!text || !str.data) {
        return str.data ? ds_retain(str) : ds_create(text ? text : "");
    }

    const size_t text_len = strlen(text);
    if (text_len == 0) {
        return ds_retain(str); // Nothing to append
    }

    const size_t new_length = str.data->length + text_len;
    ds_string result;
    result.data = ds_data_create(new_length);

    if (!result.data) {
        return DS_NULL_STRING;
    }

    // Copy original string
    memcpy(result.data->array, str.data->array, str.data->length);
    // Append new text
    memcpy(result.data->array + str.data->length, text, text_len);

    return result;
}

static size_t ds_encode_utf8(const uint32_t codepoint, char* buffer) {
    if (codepoint <= 0x7F) {
        // 1-byte ASCII: 0xxxxxxx
        buffer[0] = (char)codepoint;
        return 1;
    }

    if (codepoint <= 0x7FF) {
        // 2-byte: 110xxxxx 10xxxxxx
        buffer[0] = (char)(0xC0 | codepoint >> 6);
        buffer[1] = (char)(0x80 | codepoint & 0x3F);
        return 2;
    }

    if (codepoint <= 0xFFFF) {
        // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
        buffer[0] = (char)(0xE0 | codepoint >> 12);
        buffer[1] = (char)(0x80 | codepoint >> 6 & 0x3F);
        buffer[2] = (char)(0x80 | codepoint & 0x3F);
        return 3;
    }

    if (codepoint <= 0x10FFFF) {
        // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        buffer[0] = (char)(0xF0 | codepoint >> 18);
        buffer[1] = (char)(0x80 | codepoint >> 12 & 0x3F);
        buffer[2] = (char)(0x80 | codepoint >> 6 & 0x3F);
        buffer[3] = (char)(0x80 | codepoint & 0x3F);
        return 4;
    }


    // Invalid codepoint - use replacement character (U+FFFD)
    buffer[0] = (char)0xEF;
    buffer[1] = (char)0xBF;
    buffer[2] = (char)0xBD;
    return 3;
}

DS_DEF ds_string ds_append_char(const ds_string str, const uint32_t codepoint) {
    char utf8_buffer[4];
    const size_t bytes_needed = ds_encode_utf8(codepoint, utf8_buffer);

    // Create a null-terminated string from the buffer
    char temp_str[5]; // 4 bytes plus null terminator
    memcpy(temp_str, utf8_buffer, bytes_needed);
    temp_str[bytes_needed] = '\0';

    return ds_append(str, temp_str);
}

DS_DEF ds_string ds_prepend(const ds_string str, const char* text) {
    if (!text || !str.data) {
        return str.data ? ds_retain(str) : ds_create(text ? text : "");
    }

    const size_t text_len = strlen(text);
    if (text_len == 0) {
        return ds_retain(str); // Nothing to prepend
    }

    const size_t new_length = str.data->length + text_len;
    ds_string result;
    result.data = ds_data_create(new_length);

    if (!result.data) {
        return DS_NULL_STRING;
    }

    // Copy new text first
    memcpy(result.data->array, text, text_len);
    // Copy original string after
    memcpy(result.data->array + text_len, str.data->array, str.data->length);

    return result;
}

DS_DEF ds_string ds_insert(const ds_string str, const size_t index, const char* text) {
    if (!str.data || !text || index > str.data->length) {
        return str.data ? ds_retain(str) : DS_NULL_STRING;
    }

    const size_t text_len = strlen(text);
    if (text_len == 0) {
        return ds_retain(str); // Nothing to insert
    }

    const size_t new_length = str.data->length + text_len;
    ds_string result;
    result.data = ds_data_create(new_length);

    if (!result.data) {
        return DS_NULL_STRING;
    }

    // Copy part before insertion point
    memcpy(result.data->array, str.data->array, index);
    // Copy inserted text
    memcpy(result.data->array + index, text, text_len);
    // Copy part after insertion point
    memcpy(result.data->array + index + text_len, str.data->array + index, str.data->length - index);

    return result;
}

DS_DEF ds_string ds_substring(const ds_string str, const size_t start, size_t len) {
    if (!str.data || start >= str.data->length) {
        return ds_empty();
    }

    if (start + len > str.data->length) {
        len = str.data->length - start;
    }

    return ds_create_length(str.data->array + start, len);
}

DS_DEF ds_string ds_concat(const ds_string a, const ds_string b) {
    if (!a.data && !b.data)
        return DS_NULL_STRING;
    if (!a.data)
        return ds_retain(b);
    if (!b.data)
        return ds_retain(a);

    const size_t new_length = a.data->length + b.data->length;
    ds_string result;
    result.data = ds_data_create(new_length);

    if (!result.data) {
        return DS_NULL_STRING;
    }

    memcpy(result.data->array, a.data->array, a.data->length);
    memcpy(result.data->array + a.data->length, b.data->array, b.data->length);

    return result;
}

DS_DEF ds_string ds_join(ds_string* strings, const size_t count, const char* separator) {
    if (!strings || count == 0) {
        return ds_empty();
    }

    if (count == 1) {
        return strings[0].data ? ds_retain(strings[0]) : ds_empty();
    }

    // Calculate the total length needed
    size_t total_length = 0;
    const size_t sep_len = separator ? strlen(separator) : 0;

    for (size_t i = 0; i < count; i++) {
        if (strings[i].data) {
            total_length += strings[i].data->length;
        }
        if (i < count - 1) {
            total_length += sep_len;
        }
    }

    ds_string result;
    result.data = ds_data_create(total_length);
    if (!result.data) {
        return DS_NULL_STRING;
    }

    // Build the joined string
    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        if (strings[i].data && strings[i].data->length > 0) {
            memcpy(result.data->array + pos, strings[i].data->array, strings[i].data->length);
            pos += strings[i].data->length;
        }

        if (i < count - 1 && separator && sep_len > 0) {
            memcpy(result.data->array + pos, separator, sep_len);
            pos += sep_len;
        }
    }

    return result;
}

DS_DEF size_t ds_length(const ds_string str) { return str.data ? str.data->length : 0; }

DS_DEF const char* ds_cstr(const ds_string str) { return str.data ? str.data->array : ""; }

DS_DEF int ds_compare(const ds_string a, const ds_string b) {
    // Fast path: same object
    if (a.data == b.data)
        return 0;

    const char* a_str = ds_cstr(a);
    const char* b_str = ds_cstr(b);

    return strcmp(a_str, b_str);
}

DS_DEF int ds_find(const ds_string str, const char* needle) {
    if (!str.data || !needle)
        return -1;

    const char* found = strstr(str.data->array, needle);
    return found ? (int)(found - str.data->array) : -1;
}

DS_DEF int ds_starts_with(const ds_string str, const char* prefix) {
    if (!str.data || !prefix)
        return 0;

    const size_t prefix_len = strlen(prefix);
    if (prefix_len > str.data->length)
        return 0;

    return memcmp(str.data->array, prefix, prefix_len) == 0;
}

DS_DEF int ds_ends_with(const ds_string str, const char* suffix) {
    if (!str.data || !suffix)
        return 0;

    const size_t suffix_len = strlen(suffix);
    if (suffix_len > str.data->length)
        return 0;

    return memcmp(str.data->array + str.data->length - suffix_len, suffix, suffix_len) == 0;
}

DS_DEF size_t ds_ref_count(const ds_string str) { return str.data ? str.data->ref_count : 0; }

DS_DEF int ds_is_shared(const ds_string str) { return str.data && str.data->ref_count > 1; }

DS_DEF int ds_is_empty(const ds_string str) { return !str.data || str.data->length == 0; }

// ============================================================================
// UNICODE CODEPOINT ITERATION
// ============================================================================

// UTF-8 decoding helper function
static uint32_t ds_decode_utf8_at(const char* data, const size_t pos, const size_t end, size_t* bytes_consumed) {
    if (pos >= end) {
        *bytes_consumed = 0;
        return 0;
    }

    const unsigned char first = (unsigned char)data[pos];

    // ASCII: 0xxxxxxx
    if (first <= 0x7F) {
        *bytes_consumed = 1;
        return first;
    }

    // 2-byte: 110xxxxx 10xxxxxx
    if ((first & 0xE0) == 0xC0) {
        if (pos + 1 >= end) {
            *bytes_consumed = 0;
            return 0; // Incomplete sequence
        }
        *bytes_consumed = 2;
        return (first & 0x1F) << 6 | (unsigned char)data[pos + 1] & 0x3F;
    }

    // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
    if ((first & 0xF0) == 0xE0) {
        if (pos + 2 >= end) {
            *bytes_consumed = 0;
            return 0; // Incomplete sequence
        }
        *bytes_consumed = 3;
        return (first & 0x0F) << 12 | ((unsigned char)data[pos + 1] & 0x3F) << 6 | (unsigned char)data[pos + 2] & 0x3F;
    }

    // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    if ((first & 0xF8) == 0xF0) {
        if (pos + 3 >= end) {
            *bytes_consumed = 0;
            return 0; // Incomplete sequence
        }
        *bytes_consumed = 4;
        return (first & 0x07) << 18 | ((unsigned char)data[pos + 1] & 0x3F) << 12 |
            ((unsigned char)data[pos + 2] & 0x3F) << 6 | (unsigned char)data[pos + 3] & 0x3F;
    }

    // Invalid UTF-8
    *bytes_consumed = 1; // Skip this byte
    return 0xFFFD; // Unicode replacement character
}

DS_DEF ds_codepoint_iter ds_codepoints(const ds_string str) {
    ds_codepoint_iter iter;

    if (str.data) {
        iter.data = str.data->array;
        iter.pos = 0;
        iter.end = str.data->length;
    } else {
        iter.data = "";
        iter.pos = 0;
        iter.end = 0;
    }

    return iter;
}

DS_DEF uint32_t ds_iter_next(ds_codepoint_iter* iter) {
    if (!iter || iter->pos >= iter->end) {
        return 0; // End of iteration
    }

    size_t bytes_consumed;
    const uint32_t codepoint = ds_decode_utf8_at(iter->data, iter->pos, iter->end, &bytes_consumed);

    if (bytes_consumed == 0) {
        return 0; // Error or end of string
    }

    iter->pos += bytes_consumed;
    return codepoint;
}

DS_DEF int ds_iter_has_next(const ds_codepoint_iter* iter) { return iter && iter->pos < iter->end; }

DS_DEF size_t ds_codepoint_length(const ds_string str) {
    if (!str.data)
        return 0;

    ds_codepoint_iter iter = ds_codepoints(str);
    size_t count = 0;

    while (ds_iter_next(&iter) != 0) {
        count++;
    }

    return count;
}

DS_DEF uint32_t ds_codepoint_at(const ds_string str, const size_t index) {
    if (!str.data)
        return 0;

    ds_codepoint_iter iter = ds_codepoints(str);
    size_t current_index = 0;
    uint32_t codepoint;

    while ((codepoint = ds_iter_next(&iter)) != 0) {
        if (current_index == index) {
            return codepoint;
        }
        current_index++;
    }

    return 0; // Index out of bounds
}

#ifndef DS_SB_INITIAL_CAPACITY
#define DS_SB_INITIAL_CAPACITY 32
#endif

#ifndef DS_SB_GROWTH_FACTOR
#define DS_SB_GROWTH_FACTOR 2
#endif

// StringBuilder helper functions
static int ds_sb_ensure_capacity(ds_stringbuilder* sb, const size_t required_capacity) {
    if (sb->capacity >= required_capacity) {
        return 1; // Already have enough capacity
    }

    size_t new_capacity = sb->capacity;
    if (new_capacity == 0) {
        new_capacity = DS_SB_INITIAL_CAPACITY;
    }

    while (new_capacity < required_capacity) {
        new_capacity *= DS_SB_GROWTH_FACTOR;
    }

    // Resize the data structure
    ds_data* new_data = DS_REALLOC(sb->data, sizeof(ds_data) + new_capacity);
    if (!new_data) {
        return 0; // Failed to allocate
    }

    sb->data = new_data;
    sb->capacity = new_capacity;
    return 1; // Success
}

// Check if we need copy-on-write (string is shared)
static int ds_sb_ensure_unique(ds_stringbuilder* sb) {
    if (!sb->data || sb->data->ref_count <= 1) {
        return 1; // Already unique or null
    }

    // String is shared - need to create our own copy
    const size_t current_length = sb->data->length;
    const size_t new_capacity = sb->capacity;

    // Create the new data structure
    ds_data* new_data = DS_MALLOC(sizeof(ds_data) + new_capacity);
    if (!new_data) {
        return 0; // Failed to allocate
    }

    // Copy the content
    new_data->length = current_length;
    new_data->ref_count = 1;
    memcpy(new_data->array, sb->data->array, current_length + 1);

    // Release old reference and switch to new data
    sb->data->ref_count--;
    sb->data = new_data;

    return 1; // Success
}

DS_DEF ds_stringbuilder ds_sb_create(void) { return ds_sb_create_with_capacity(DS_SB_INITIAL_CAPACITY); }


DS_DEF ds_stringbuilder ds_sb_create_with_capacity(size_t capacity) {
    ds_stringbuilder sb;

    if (capacity == 0)
        capacity = DS_SB_INITIAL_CAPACITY;

    sb.data = (ds_data*)DS_MALLOC(sizeof(ds_data) + capacity);
    if (sb.data) {
        sb.data->length = 0;
        sb.data->ref_count = 1;
        sb.data->array[0] = '\0';
        sb.capacity = capacity;
    } else {
        sb.capacity = 0;
    }

    return sb;
}

DS_DEF void ds_sb_destroy(ds_stringbuilder* sb) {
    if (sb && sb->data) {
        sb->data->ref_count--;
        if (sb->data->ref_count == 0) {
            DS_FREE(sb->data);
        }
        sb->data = NULL;
        sb->capacity = 0;
    }
}

DS_DEF int ds_sb_append(ds_stringbuilder* sb, const char* text) {
    if (!sb || !text || !sb->data)
        return 0;

    const size_t text_len = strlen(text);
    if (text_len == 0)
        return 1;

    // Ensure we have a unique copy and enough capacity
    if (!ds_sb_ensure_unique(sb))
        return 0;
    if (!ds_sb_ensure_capacity(sb, sb->data->length + text_len + 1))
        return 0;

    // Append the text
    memcpy(sb->data->array + sb->data->length, text, text_len);
    sb->data->length += text_len;
    sb->data->array[sb->data->length] = '\0';

    return 1; // Success
}

DS_DEF int ds_sb_append_char(ds_stringbuilder* sb, const uint32_t codepoint) {
    if (!sb || !sb->data)
        return 0;

    char utf8_buffer[4];
    const size_t bytes_needed = ds_encode_utf8(codepoint, utf8_buffer);

    // Ensure we have a unique copy and enough capacity
    if (!ds_sb_ensure_unique(sb))
        return 0;
    if (!ds_sb_ensure_capacity(sb, sb->data->length + bytes_needed + 1))
        return 0;

    // Append the UTF-8 bytes
    memcpy(sb->data->array + sb->data->length, utf8_buffer, bytes_needed);
    sb->data->length += bytes_needed;
    sb->data->array[sb->data->length] = '\0';

    return 1; // Success
}

DS_DEF int ds_sb_append_string(ds_stringbuilder* sb, const ds_string str) {
    if (!sb || !str.data)
        return 0;

    // Ensure we have a unique copy and enough capacity
    if (!ds_sb_ensure_unique(sb))
        return 0;
    if (!ds_sb_ensure_capacity(sb, sb->data->length + str.data->length + 1))
        return 0;

    // Append the string
    memcpy(sb->data->array + sb->data->length, str.data->array, str.data->length);
    sb->data->length += str.data->length;
    sb->data->array[sb->data->length] = '\0';

    return 1; // Success
}

DS_DEF int ds_sb_insert(ds_stringbuilder* sb, const size_t index, const char* text) {
    if (!sb || !text || !sb->data || index > sb->data->length)
        return 0;

    const size_t text_len = strlen(text);
    if (text_len == 0)
        return 1;

    // Ensure we have a unique copy and enough capacity
    if (!ds_sb_ensure_unique(sb))
        return 0;
    if (!ds_sb_ensure_capacity(sb, sb->data->length + text_len + 1))
        return 0;

    // Move content after the insertion point
    memmove(sb->data->array + index + text_len, sb->data->array + index, sb->data->length - index + 1);

    // Insert the text
    memcpy(sb->data->array + index, text, text_len);
    sb->data->length += text_len;

    return 1; // Success
}

DS_DEF void ds_sb_clear(ds_stringbuilder* sb) {
    if (!sb || !sb->data)
        return;

    // Ensure we have a unique copy
    if (!ds_sb_ensure_unique(sb))
        return;

    sb->data->length = 0;
    sb->data->array[0] = '\0';
}

DS_DEF ds_string ds_sb_to_string(ds_stringbuilder* sb) {
    if (!sb || !sb->data) {
        return DS_NULL_STRING;
    }

    // THE MAGIC: Shrink to the exact size and prepare for sharing
    size_t exact_size = sizeof(ds_data) + sb->data->length + 1;
    ds_data* shrunk_data = (ds_data*)DS_REALLOC(sb->data, exact_size);

    if (shrunk_data) {
        sb->data = shrunk_data;
        sb->capacity = sb->data->length + 1; // Update capacity to match
    }

    // Increment ref count for sharing
    sb->data->ref_count++;

    // Return the string handle
    ds_string result;
    result.data = sb->data;
    return result;
}

DS_DEF size_t ds_sb_length(const ds_stringbuilder* sb) { return sb && sb->data ? sb->data->length : 0; }

DS_DEF size_t ds_sb_capacity(const ds_stringbuilder* sb) { return sb ? sb->capacity : 0; }

DS_DEF const char* ds_sb_cstr(const ds_stringbuilder* sb) { return sb && sb->data ? sb->data->array : ""; }

// Legacy compatibility
DS_DEF void ds_free(ds_string* str) { ds_release(str); }

#endif // DS_IMPLEMENTATION

#endif // DYNAMIC_STRING_H
