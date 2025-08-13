/**
 * @file dynamic_string.h
 * @brief Modern, efficient, single-file string library for C
 * @version 0.0.2
 * @date Aug 13, 2025
 *
 * @details
 * A modern, efficient, single-file string library for C featuring:
 * - **Reference counting** with automatic memory management
 * - **Immutable strings** for safety and sharing
 * - **Copy-on-write StringBuilder** for efficient construction
 * - **Unicode support** with UTF-8 storage and codepoint iteration
 * - **Zero dependencies** - just drop in the .h file
 * - **Direct C compatibility** - ds_string works with all C functions
 *
 * @section usage_section Usage
 * @code{.c}
 * #define DS_IMPLEMENTATION
 * #include "dynamic_string.h"
 *
 * int main() {
 *     ds_string greeting = ds("Hello");
 *     ds_string full = ds_append(greeting, " World!");
 *     printf("%s\n", full);  // Direct usage - no ds_cstr() needed!
 *     ds_release(&greeting);
 *     ds_release(&full);
 *     return 0;
 * }
 * @endcode
 *
 * @section license_section License
 * Dual licensed under your choice of:
 * - MIT License
 * - The Unlicense (public domain)
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

/**
 * @brief String handle - points directly to null-terminated string data
 *
 * This is a char* that points directly to UTF-8 string data. Metadata
 * (refcount, length) is stored at negative offsets before the string data.
 * This allows ds_string to be used directly with all C string functions.
 *
 * Memory layout: [refcount|length|string_data|\0]
 *                                ^
 *                  ds_string points here
 *
 * @note Use directly with printf, strcmp, fopen, etc. - no conversion needed!
 * @note NULL represents an invalid/empty string handle
 */
typedef char* ds_string;

/**
 * @defgroup core_functions Core String Functions
 * @brief Basic string creation, retention, and release functions
 * @{
 */

/**
 * @brief Create a new string from a C string
 * @param text Null-terminated C string to copy (may be NULL)
 * @return New ds_string instance, or NULL on failure
 */
DS_DEF ds_string ds(const char* text);

/**
 * @brief Create a string from a buffer with explicit length
 * @param text Source buffer (may contain embedded nulls)
 * @param length Number of bytes to copy from buffer
 * @return New ds_string instance, or NULL on failure
 */
DS_DEF ds_string ds_create_length(const char* text, size_t length);

/**
 * @brief Increment reference count and return shared handle
 * @param str String to retain (may be NULL)
 * @return New handle to the same string data
 */
DS_DEF ds_string ds_retain(ds_string str);

/**
 * @brief Decrement reference count and free memory if last reference
 * @param str Pointer to string handle to release (may be NULL)
 */
DS_DEF void ds_release(ds_string* str);

/** @} */

// String operations (return new strings - immutable)

/**
 * @brief Append text to a string
 * @param str Source string (may be NULL)
 * @param text Text to append (may be NULL)
 * @return New string with appended text, or NULL on failure
 */
DS_DEF ds_string ds_append(ds_string str, const char* text);

/**
 * @brief Append a Unicode codepoint to a string
 * @param str Source string (may be NULL)
 * @param codepoint Unicode codepoint to append (invalid codepoints become U+FFFD)
 * @return New string with appended text, retained original if text is NULL/empty, or NULL if allocation fails
 */
DS_DEF ds_string ds_append_char(ds_string str, uint32_t codepoint);

/**
 * @brief Prepend text to the beginning of a string
 * @param str Source string (may be NULL)
 * @param text Text to prepend (may be NULL)
 * @return New string with prepended text, or NULL on failure
 */
DS_DEF ds_string ds_prepend(ds_string str, const char* text);

/**
 * @brief Insert text at a specific position in a string
 * @param str Source string (may be NULL)
 * @param index Byte position where to insert text (0-based)
 * @param text Text to insert (may be NULL)
 * @return New string with inserted text, or original string if index is invalid, or NULL on allocation failure
 */
DS_DEF ds_string ds_insert(ds_string str, size_t index, const char* text);

/**
 * @brief Extract a substring from a string
 * @param str Source string (may be NULL)
 * @param start Starting byte position (0-based)
 * @param len Number of bytes to include in substring
 * @return New string containing the substring, or empty string if invalid range
 */
DS_DEF ds_string ds_substring(ds_string str, size_t start, size_t len);

// String concatenation

/**
 * @brief Concatenate two strings
 * @param a First string (may be NULL)
 * @param b Second string (may be NULL)
 * @return New string containing a + b, or NULL if both inputs are null
 */
DS_DEF ds_string ds_concat(ds_string a, ds_string b);

/**
 * @brief Join multiple strings with a separator
 * @param strings Array of ds_string to join (may contain NULL entries)
 * @param count Number of strings in the array
 * @param separator Separator to insert between strings (may be NULL)
 * @return New string with all strings joined, or empty string if count is 0
 */
DS_DEF ds_string ds_join(ds_string* strings, size_t count, const char* separator);

// Utility functions (read-only)
DS_DEF size_t ds_length(ds_string str);
DS_DEF int ds_compare(ds_string a, ds_string b);
DS_DEF int ds_find(ds_string str, const char* needle);
DS_DEF int ds_starts_with(ds_string str, const char* prefix);
DS_DEF int ds_ends_with(ds_string str, const char* suffix);

// Reference count inspection
DS_DEF size_t ds_refcount(ds_string str);
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
#define ds_empty() ds("")
#define ds_from_literal(lit) ds(lit)

// ============================================================================
// STRINGBUILDER - Mutable builder for efficient string construction
// ============================================================================

typedef struct {
    ds_string data; // Points to string data (same layout as ds_string)
    size_t capacity; // Capacity for growth (length is in metadata)
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

#ifdef __cplusplus
}
#endif

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#ifdef DS_IMPLEMENTATION

/**
 * @brief Internal metadata structure stored before string data
 */
typedef struct ds_internal {
    size_t refcount;
    size_t length;
} ds_internal;

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Get pointer to metadata for a string
 * @param str String handle (must not be NULL)
 * @return Pointer to metadata structure
 */
static inline ds_internal* ds_meta(ds_string str) { return (ds_internal*)(str - sizeof(ds_internal)); }

/**
 * @brief Allocate memory for string with metadata
 * @param length Length of string data in bytes
 * @return Pointer to string data portion, or NULL on failure
 */
static ds_string ds_alloc(size_t length) {
    // Allocate: metadata + string data + null terminator
    void* block = DS_MALLOC(sizeof(ds_internal) + length + 1);
    DS_ASSERT(block && "Memory allocation failed");

    // Initialize metadata
    ds_internal* meta = (ds_internal*)block;
    meta->refcount = 1;
    meta->length = length;

    // Return pointer to string data portion
    ds_string str = (char*)block + sizeof(ds_internal);
    str[length] = '\0'; // Always null-terminate

    return str;
}

/**
 * @brief Free string memory
 * @param str String handle (must not be NULL)
 */
static void ds_dealloc(ds_string str) {
    if (str) {
        // Get original malloc pointer and free it
        void* block = (char*)str - sizeof(ds_internal);
        DS_FREE(block);
    }
}

// ============================================================================
// CORE STRING FUNCTIONS
// ============================================================================

DS_DEF size_t ds_length(ds_string str) { return str ? ds_meta(str)->length : 0; }

DS_DEF size_t ds_refcount(ds_string str) { return str ? ds_meta(str)->refcount : 0; }

DS_DEF int ds_is_shared(ds_string str) { return str && ds_meta(str)->refcount > 1; }

DS_DEF int ds_is_empty(ds_string str) { return !str || ds_meta(str)->length == 0; }

DS_DEF ds_string ds(const char* text) {
    if (!text)
        return NULL;

    size_t len = strlen(text);
    return ds_create_length(text, len);
}

DS_DEF ds_string ds_create_length(const char* text, size_t length) {
    ds_string str = ds_alloc(length);

    if (text && length > 0) {
        memcpy(str, text, length);
    }

    return str;
}

DS_DEF ds_string ds_retain(ds_string str) {
    if (str) {
        ds_meta(str)->refcount++;
    }
    return str;
}

DS_DEF void ds_release(ds_string* str) {
    if (str && *str) {
        ds_internal* meta = ds_meta(*str);
        meta->refcount--;
        if (meta->refcount == 0) {
            ds_dealloc(*str);
        }
        *str = NULL;
    }
}

DS_DEF ds_string ds_append(ds_string str, const char* text) {
    if (!text) {
        return str ? ds_retain(str) : NULL;
    }
    if (!str) {
        return ds(text);
    }

    size_t text_len = strlen(text);
    if (text_len == 0) {
        return ds_retain(str);
    }

    size_t new_length = ds_meta(str)->length + text_len;
    ds_string result = ds_alloc(new_length);

    // Copy original string
    memcpy(result, str, ds_meta(str)->length);
    // Append new text
    memcpy(result + ds_meta(str)->length, text, text_len);

    return result;
}

static size_t ds_encode_utf8(uint32_t codepoint, char* buffer) {
    if (codepoint <= 0x7F) {
        buffer[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7FF) {
        buffer[0] = (char)(0xC0 | codepoint >> 6);
        buffer[1] = (char)(0x80 | codepoint & 0x3F);
        return 2;
    }
    if (codepoint <= 0xFFFF) {
        buffer[0] = (char)(0xE0 | codepoint >> 12);
        buffer[1] = (char)(0x80 | codepoint >> 6 & 0x3F);
        buffer[2] = (char)(0x80 | codepoint & 0x3F);
        return 3;
    }
    if (codepoint <= 0x10FFFF) {
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

DS_DEF ds_string ds_append_char(ds_string str, uint32_t codepoint) {
    char utf8_buffer[4];
    size_t bytes_needed = ds_encode_utf8(codepoint, utf8_buffer);

    char temp_str[5];
    memcpy(temp_str, utf8_buffer, bytes_needed);
    temp_str[bytes_needed] = '\0';

    return ds_append(str, temp_str);
}

DS_DEF ds_string ds_prepend(ds_string str, const char* text) {
    if (!text) {
        return str ? ds_retain(str) : NULL;
    }
    if (!str) {
        return ds(text);
    }

    size_t text_len = strlen(text);
    if (text_len == 0) {
        return ds_retain(str);
    }

    size_t new_length = ds_meta(str)->length + text_len;
    ds_string result = ds_alloc(new_length);

    // Copy new text first
    memcpy(result, text, text_len);
    // Copy original string after
    memcpy(result + text_len, str, ds_meta(str)->length);

    return result;
}

DS_DEF ds_string ds_insert(ds_string str, size_t index, const char* text) {
    if (!str || !text || index > ds_meta(str)->length) {
        return str ? ds_retain(str) : NULL;
    }

    size_t text_len = strlen(text);
    if (text_len == 0) {
        return ds_retain(str);
    }

    size_t str_len = ds_meta(str)->length;
    size_t new_length = str_len + text_len;
    ds_string result = ds_alloc(new_length);

    // Copy part before insertion point
    memcpy(result, str, index);
    // Copy inserted text
    memcpy(result + index, text, text_len);
    // Copy part after insertion point
    memcpy(result + index + text_len, str + index, str_len - index);

    return result;
}

DS_DEF ds_string ds_substring(ds_string str, size_t start, size_t len) {
    if (!str || start >= ds_meta(str)->length) {
        return ds("");
    }

    size_t str_len = ds_meta(str)->length;
    if (start + len > str_len) {
        len = str_len - start;
    }

    return ds_create_length(str + start, len);
}

DS_DEF ds_string ds_concat(ds_string a, ds_string b) {
    if (!a && !b)
        return NULL;
    if (!a)
        return ds_retain(b);
    if (!b)
        return ds_retain(a);

    size_t new_length = ds_meta(a)->length + ds_meta(b)->length;
    ds_string result = ds_alloc(new_length);

    memcpy(result, a, ds_meta(a)->length);
    memcpy(result + ds_meta(a)->length, b, ds_meta(b)->length);

    return result;
}

DS_DEF ds_string ds_join(ds_string* strings, size_t count, const char* separator) {
    if (!strings || count == 0) {
        return ds("");
    }

    if (count == 1) {
        return strings[0] ? ds_retain(strings[0]) : ds("");
    }

    // Calculate total length needed
    size_t total_length = 0;
    size_t sep_len = separator ? strlen(separator) : 0;

    for (size_t i = 0; i < count; i++) {
        if (strings[i]) {
            total_length += ds_meta(strings[i])->length;
        }
        if (i < count - 1) {
            total_length += sep_len;
        }
    }

    ds_string result = ds_alloc(total_length);

    // Build the joined string
    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        if (strings[i] && ds_meta(strings[i])->length > 0) {
            size_t str_len = ds_meta(strings[i])->length;
            memcpy(result + pos, strings[i], str_len);
            pos += str_len;
        }

        if (i < count - 1 && separator && sep_len > 0) {
            memcpy(result + pos, separator, sep_len);
            pos += sep_len;
        }
    }

    return result;
}

DS_DEF int ds_compare(ds_string a, ds_string b) {
    // Fast path: same object
    if (a == b)
        return 0;

    const char* a_str = a ? a : "";
    const char* b_str = b ? b : "";

    return strcmp(a_str, b_str);
}

DS_DEF int ds_find(ds_string str, const char* needle) {
    if (!str || !needle)
        return -1;

    const char* found = strstr(str, needle);
    return found ? (int)(found - str) : -1;
}

DS_DEF int ds_starts_with(ds_string str, const char* prefix) {
    if (!str || !prefix)
        return 0;

    size_t prefix_len = strlen(prefix);
    if (prefix_len > ds_meta(str)->length)
        return 0;

    return memcmp(str, prefix, prefix_len) == 0;
}

DS_DEF int ds_ends_with(ds_string str, const char* suffix) {
    if (!str || !suffix)
        return 0;

    size_t suffix_len = strlen(suffix);
    size_t str_len = ds_meta(str)->length;
    if (suffix_len > str_len)
        return 0;

    return memcmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

// ============================================================================
// UNICODE CODEPOINT ITERATION
// ============================================================================

static uint32_t ds_decode_utf8_at(const char* data, size_t pos, size_t end, size_t* bytes_consumed) {
    if (pos >= end) {
        *bytes_consumed = 0;
        return 0;
    }

    unsigned char first = (unsigned char)data[pos];

    if (first <= 0x7F) {
        *bytes_consumed = 1;
        return first;
    }

    if ((first & 0xE0) == 0xC0) {
        if (pos + 1 >= end) {
            *bytes_consumed = 0;
            return 0;
        }
        *bytes_consumed = 2;
        return (first & 0x1F) << 6 | (unsigned char)data[pos + 1] & 0x3F;
    }

    if ((first & 0xF0) == 0xE0) {
        if (pos + 2 >= end) {
            *bytes_consumed = 0;
            return 0;
        }
        *bytes_consumed = 3;
        return (first & 0x0F) << 12 | ((unsigned char)data[pos + 1] & 0x3F) << 6 | (unsigned char)data[pos + 2] & 0x3F;
    }

    if ((first & 0xF8) == 0xF0) {
        if (pos + 3 >= end) {
            *bytes_consumed = 0;
            return 0;
        }
        *bytes_consumed = 4;
        return (first & 0x07) << 18 | ((unsigned char)data[pos + 1] & 0x3F) << 12 |
            ((unsigned char)data[pos + 2] & 0x3F) << 6 | (unsigned char)data[pos + 3] & 0x3F;
    }

    *bytes_consumed = 1;
    return 0xFFFD; // Unicode replacement character
}

DS_DEF ds_codepoint_iter ds_codepoints(ds_string str) {
    ds_codepoint_iter iter;

    if (str) {
        iter.data = str;
        iter.pos = 0;
        iter.end = ds_meta(str)->length;
    } else {
        iter.data = "";
        iter.pos = 0;
        iter.end = 0;
    }

    return iter;
}

DS_DEF uint32_t ds_iter_next(ds_codepoint_iter* iter) {
    if (!iter || iter->pos >= iter->end) {
        return 0;
    }

    size_t bytes_consumed;
    uint32_t codepoint = ds_decode_utf8_at(iter->data, iter->pos, iter->end, &bytes_consumed);

    if (bytes_consumed == 0) {
        return 0;
    }

    iter->pos += bytes_consumed;
    return codepoint;
}

DS_DEF int ds_iter_has_next(const ds_codepoint_iter* iter) { return iter && iter->pos < iter->end; }

DS_DEF size_t ds_codepoint_length(ds_string str) {
    if (!str)
        return 0;

    ds_codepoint_iter iter = ds_codepoints(str);
    size_t count = 0;

    while (ds_iter_next(&iter) != 0) {
        count++;
    }

    return count;
}

DS_DEF uint32_t ds_codepoint_at(ds_string str, size_t index) {
    if (!str)
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

    return 0;
}

// ============================================================================
// STRINGBUILDER
// ============================================================================

#ifndef DS_SB_INITIAL_CAPACITY
#define DS_SB_INITIAL_CAPACITY 32
#endif

#ifndef DS_SB_GROWTH_FACTOR
#define DS_SB_GROWTH_FACTOR 2
#endif

// StringBuilder helper functions
static int ds_sb_ensure_capacity(ds_stringbuilder* sb, size_t required_capacity) {
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

    // Get original block pointer and resize
    void* old_block = (char*)sb->data - sizeof(ds_internal);
    void* new_block = DS_REALLOC(old_block, sizeof(ds_internal) + new_capacity);
    DS_ASSERT(new_block && "Memory re-allocation failed");

    sb->data = (char*)new_block + sizeof(ds_internal);
    sb->capacity = new_capacity;
    return 1;
}

static int ds_sb_ensure_unique(ds_stringbuilder* sb) {
    if (!sb->data)
        return 0;

    ds_internal* meta = ds_meta(sb->data);
    if (meta->refcount <= 1) {
        return 1; // Already unique
    }

    // Need to create our own copy - just allocate exactly what we need
    size_t current_length = meta->length;
    ds_string new_str = ds_alloc(current_length);

    // Copy current content
    memcpy(new_str, sb->data, current_length);

    // Release old reference properly
    ds_string old_str = sb->data;
    ds_release(&old_str);

    // Update StringBuilder - capacity is just what ds_alloc gave us
    sb->data = new_str;
    sb->capacity = current_length + 1; // ds_alloc gives us length + null terminator

    return 1;
}

DS_DEF ds_stringbuilder ds_sb_create(void) { return ds_sb_create_with_capacity(DS_SB_INITIAL_CAPACITY); }

DS_DEF ds_stringbuilder ds_sb_create_with_capacity(size_t capacity) {
    ds_stringbuilder sb;

    if (capacity == 0)
        capacity = DS_SB_INITIAL_CAPACITY;

    void* block = DS_MALLOC(sizeof(ds_internal) + capacity);
    DS_ASSERT(block && "Memory allocation failed");

    ds_internal* meta = (ds_internal*)block;
    meta->refcount = 1;
    meta->length = 0;

    sb.data = (char*)block + sizeof(ds_internal);
    sb.data[0] = '\0';
    sb.capacity = capacity;

    return sb;
}

DS_DEF void ds_sb_destroy(ds_stringbuilder* sb) {
    if (sb) {
        ds_release(&sb->data); // Handles refcount decrement and freeing
        sb->capacity = 0;
    }
}

DS_DEF int ds_sb_append(ds_stringbuilder* sb, const char* text) {
    if (!sb || !text || !sb->data)
        return 0;

    size_t text_len = strlen(text);
    if (text_len == 0)
        return 1;

    if (!ds_sb_ensure_unique(sb))
        return 0;

    ds_internal* meta = ds_meta(sb->data);
    if (!ds_sb_ensure_capacity(sb, meta->length + text_len + 1))
        return 0;

    memcpy(sb->data + meta->length, text, text_len);
    meta->length += text_len;
    sb->data[meta->length] = '\0';

    return 1;
}

DS_DEF int ds_sb_append_char(ds_stringbuilder* sb, uint32_t codepoint) {
    if (!sb || !sb->data)
        return 0;

    char utf8_buffer[4];
    size_t bytes_needed = ds_encode_utf8(codepoint, utf8_buffer);

    if (!ds_sb_ensure_unique(sb))
        return 0;

    ds_internal* meta = ds_meta(sb->data);
    if (!ds_sb_ensure_capacity(sb, meta->length + bytes_needed + 1))
        return 0;

    memcpy(sb->data + meta->length, utf8_buffer, bytes_needed);
    meta->length += bytes_needed;
    sb->data[meta->length] = '\0';

    return 1;
}

DS_DEF int ds_sb_append_string(ds_stringbuilder* sb, ds_string str) {
    if (!sb || !str)
        return 0;

    if (!ds_sb_ensure_unique(sb))
        return 0;

    ds_internal* sb_meta = ds_meta(sb->data);
    ds_internal* str_meta = ds_meta(str);

    if (!ds_sb_ensure_capacity(sb, sb_meta->length + str_meta->length + 1))
        return 0;

    memcpy(sb->data + sb_meta->length, str, str_meta->length);
    sb_meta->length += str_meta->length;
    sb->data[sb_meta->length] = '\0';

    return 1;
}

DS_DEF int ds_sb_insert(ds_stringbuilder* sb, size_t index, const char* text) {
    if (!sb || !text || !sb->data)
        return 0;

    ds_internal* meta = ds_meta(sb->data);
    if (index > meta->length)
        return 0;

    size_t text_len = strlen(text);
    if (text_len == 0)
        return 1;

    if (!ds_sb_ensure_unique(sb))
        return 0;
    if (!ds_sb_ensure_capacity(sb, meta->length + text_len + 1))
        return 0;

    // Move content after insertion point
    memmove(sb->data + index + text_len, sb->data + index, meta->length - index + 1);

    // Insert the text
    memcpy(sb->data + index, text, text_len);
    meta->length += text_len;

    return 1;
}

DS_DEF void ds_sb_clear(ds_stringbuilder* sb) {
    if (!sb || !sb->data)
        return;

    if (!ds_sb_ensure_unique(sb))
        return;

    ds_internal* meta = ds_meta(sb->data);
    meta->length = 0;
    sb->data[0] = '\0';
}

DS_DEF ds_string ds_sb_to_string(ds_stringbuilder* sb) {
    if (!sb || !sb->data) {
        return NULL;
    }

    ds_internal* meta = ds_meta(sb->data);

    // Shrink to exact size
    size_t exact_size = sizeof(ds_internal) + meta->length + 1;
    void* old_block = (char*)sb->data - sizeof(ds_internal);
    void* shrunk_block = DS_REALLOC(old_block, exact_size);

    ds_string result;
    if (shrunk_block) {
        result = (char*)shrunk_block + sizeof(ds_internal);
        meta = (ds_internal*)shrunk_block;
    } else {
        result = sb->data; // Use original if realloc failed
    }

    // IMPORTANT: Mark StringBuilder as consumed to prevent reuse
    // This eliminates the complex copy-on-write bugs
    sb->data = NULL;
    sb->capacity = 0;

    return result;
}

DS_DEF size_t ds_sb_length(const ds_stringbuilder* sb) {
    if (!sb || !sb->data)
        return 0;
    ds_internal* meta = ds_meta(sb->data);
    return meta->length;
}

DS_DEF size_t ds_sb_capacity(const ds_stringbuilder* sb) { return sb ? sb->capacity : 0; }

DS_DEF const char* ds_sb_cstr(const ds_stringbuilder* sb) { return sb && sb->data ? sb->data : ""; }

#endif // DS_IMPLEMENTATION

#endif // DYNAMIC_STRING_H
