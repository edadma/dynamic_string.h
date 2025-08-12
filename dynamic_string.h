/**
 * @file dynamic_string.h
 * @brief Modern, efficient, single-file string library for C
 * @version 0.0.1
 * @date 2025
 *
 * @details
 * A modern, efficient, single-file string library for C featuring:
 * - **Reference counting** with automatic memory management
 * - **Immutable strings** for safety and sharing
 * - **Copy-on-write StringBuilder** for efficient construction
 * - **Unicode support** with UTF-8 storage and codepoint iteration
 * - **Zero dependencies** - just drop in the .h file
 * - **FFI-friendly** design for use with other languages
 *
 * @section usage_section Usage
 * @code{.c}
 * #define DS_IMPLEMENTATION
 * #include "dynamic_string.h"
 *
 * int main() {
 *     ds_string greeting = ds_create("Hello");
 *     ds_string full = ds_append(greeting, " World!");
 *     printf("%s\n", ds_cstr(full));
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
 * @brief Internal string data structure with reference counting
 *
 * This structure contains the actual string data along with metadata for
 * reference counting and length tracking. The string data immediately follows
 * the structure in memory for optimal cache locality.
 *
 * @warning This is an internal structure - users should not access fields directly
 * @note Memory layout: [length|ref_count|string_data|null_terminator]
 *
 * @var ds_data::length
 * Number of bytes in the string (excluding null terminator)
 *
 * @var ds_data::ref_count
 * Number of ds_string handles pointing to this data
 *
 * @var ds_data::array
 * Flexible array member containing the actual UTF-8 string data plus null terminator
 */
typedef struct ds_data {
    size_t length;
    size_t ref_count;
    char array[]; // Flexible array member - the actual string data
} ds_data;

/**
 * @brief Handle to an immutable, reference-counted string
 *
 * This is the main string type used throughout the library. Multiple ds_string
 * instances can safely share the same underlying data through reference counting.
 * All string operations are immutable - they return new strings rather than
 * modifying existing ones.
 *
 * @note Strings are immutable - all operations return new strings
 * @note Multiple handles can safely share the same data
 * @note Always call ds_release() when done with a string
 *
 * @par Example:
 * @code{.c}
 * ds_string str = ds_create("Hello");
 * ds_string shared = ds_retain(str);  // Both point to same data
 * ds_string new_str = ds_append(str, " World");  // New string created
 *
 * ds_release(&str);
 * ds_release(&shared);
 * ds_release(&new_str);
 * @endcode
 *
 * @var ds_string::data
 * Pointer to the internal data structure (may be NULL for DS_NULL_STRING)
 */
typedef struct {
    ds_data* data;
} ds_string;

/**
 * @brief Null string constant representing an invalid or empty string handle
 *
 * This constant represents a null string that contains no data and should be used
 * to initialize ds_string variables or as a return value for error conditions.
 *
 * @note This is equivalent to a ds_string with a NULL data pointer
 * @warning Do not call ds_release() on DS_NULL_STRING - it's safe but unnecessary
 *
 * @par Example:
 * @code{.c}
 * ds_string str = DS_NULL_STRING;  // Safe initialization
 * if (str.data == NULL) {
 *     printf("String is null\n");
 * }
 * @endcode
 */
extern const ds_string DS_NULL_STRING;

/**
 * @defgroup core_functions Core String Functions
 * @brief Basic string creation, retention, and release functions
 * @{
 */

/**
 * @brief Create a new string from a C string
 *
 * Creates a new ds_string by copying data from a null-terminated C string.
 * The resulting string is independent of the input buffer.
 *
 * @param text Null-terminated C string to copy (may be NULL)
 * @return New ds_string instance, or DS_NULL_STRING on failure
 *
 * @par Example:
 * @code{.c}
 * ds_string str = ds_create("Hello World");
 * printf("Created: %s\n", ds_cstr(str));
 * ds_release(&str);
 * @endcode
 *
 * @note If text is NULL, returns DS_NULL_STRING
 * @see ds_create_length(), ds_empty()
 */
DS_DEF ds_string ds_create(const char* text);

/**
 * @brief Create a string from a buffer with explicit length
 *
 * Creates a new ds_string by copying the specified number of bytes
 * from the input buffer. The buffer does not need to be null-terminated.
 *
 * @param text Source buffer (may contain embedded nulls)
 * @param length Number of bytes to copy from buffer
 * @return New ds_string instance, or DS_NULL_STRING on failure
 *
 * @par Example:
 * @code{.c}
 * const char buffer[] = "Hello\0World";
 * ds_string str = ds_create_length(buffer, 11);  // Includes embedded null
 * printf("Length: %zu\n", ds_length(str));        // Prints: 11
 * ds_release(&str);
 * @endcode
 *
 * @note The resulting string will be null-terminated regardless of input
 * @see ds_create()
 */
DS_DEF ds_string ds_create_length(const char* text, size_t length);

/**
 * @brief Increment reference count and return shared handle
 *
 * Creates a new handle to the same string data without copying.
 * Both the original and returned handles refer to the same memory.
 *
 * @param str String to retain (may be DS_NULL_STRING)
 * @return New handle to the same string data
 *
 * @par Example:
 * @code{.c}
 * ds_string original = ds_create("Shared data");
 * ds_string copy = ds_retain(original);    // No memory copy!
 *
 * printf("Ref count: %zu\n", ds_ref_count(original));  // Prints: 2
 * printf("Same data: %s\n", (original.data == copy.data) ? "YES" : "NO");
 *
 * ds_release(&original);
 * ds_release(&copy);
 * @endcode
 *
 * @note This is very efficient - no memory allocation or copying occurs
 * @see ds_release(), ds_ref_count()
 */
DS_DEF ds_string ds_retain(ds_string str);

/**
 * @brief Decrement reference count and free memory if last reference
 *
 * Decrements the reference count of the string. If this was the last
 * reference, the memory is freed. The handle is set to DS_NULL_STRING.
 *
 * @param str Pointer to string handle to release (may be NULL or point to DS_NULL_STRING)
 *
 * @par Example:
 * @code{.c}
 * ds_string str = ds_create("Hello");
 * ds_string shared = ds_retain(str);
 *
 * ds_release(&str);     // Decrements count, memory still valid
 * printf("%s\n", ds_cstr(shared));  // Still works - "Hello"
 * ds_release(&shared);  // Last reference - memory freed
 * @endcode
 *
 * @warning After calling ds_release(), the handle should not be used
 * @note Safe to call multiple times or with NULL pointer
 * @see ds_retain(), ds_ref_count()
 */
DS_DEF void ds_release(ds_string* str);

/** @} */

// String operations (return new strings - immutable)

/**
 * @brief Append text to a string
 *
 * Creates a new string by appending the given text to the end of the
 * input string. The original string is unchanged (immutable).
 *
 * @param str Source string (may be DS_NULL_STRING)
 * @param text Text to append (may be NULL)
 * @return New string with appended text, or DS_NULL_STRING on failure
 *
 * @par Example:
 * @code{.c}
 * ds_string base = ds_create("Hello");
 * ds_string result = ds_append(base, " World!");
 *
 * printf("Original: %s\n", ds_cstr(base));    // "Hello"
 * printf("Result: %s\n", ds_cstr(result));    // "Hello World!"
 *
 * ds_release(&base);
 * ds_release(&result);
 * @endcode
 *
 * @note Returns retained copy of original if text is NULL or empty
 * @see ds_prepend(), ds_insert(), ds_append_char()
 */
DS_DEF ds_string ds_append(ds_string str, const char* text);

/**
 * @brief Append a Unicode codepoint to a string
 *
 * Creates a new string by appending the given Unicode codepoint to the end
 * of the input string. The codepoint is automatically encoded as UTF-8.
 *
 * @param str Source string (may be DS_NULL_STRING)
 * @param codepoint Unicode codepoint to append (U+0000 to U+10FFFF)
 * @return New string with appended codepoint, or DS_NULL_STRING on failure
 *
 * @par Example:
 * @code{.c}
 * ds_string base = ds_create("Hello");
 * ds_string with_emoji = ds_append_char(base, 0x1F44B);  // 👋 waving hand
 * ds_string with_space = ds_append_char(with_emoji, 0x20);  // space character
 *
 * printf("Result: %s\n", ds_cstr(with_space));  // "Hello👋 "
 *
 * ds_release(&base);
 * ds_release(&with_emoji);
 * ds_release(&with_space);
 * @endcode
 *
 * @note Invalid codepoints (> U+10FFFF) are replaced with U+FFFD (replacement character)
 * @note Returns retained copy of original if codepoint is 0
 * @see ds_append(), ds_sb_append_char()
 */
DS_DEF ds_string ds_append_char(ds_string str, uint32_t codepoint);

/**
 * @brief Prepend text to the beginning of a string
 *
 * Creates a new string by inserting the given text at the beginning of the
 * input string. The original string is unchanged (immutable).
 *
 * @param str Source string (may be DS_NULL_STRING)
 * @param text Text to prepend (may be NULL)
 * @return New string with prepended text, or DS_NULL_STRING on failure
 *
 * @par Example:
 * @code{.c}
 * ds_string base = ds_create("World!");
 * ds_string result = ds_prepend(base, "Hello ");
 *
 * printf("Original: %s\n", ds_cstr(base));    // "World!"
 * printf("Result: %s\n", ds_cstr(result));    // "Hello World!"
 *
 * ds_release(&base);
 * ds_release(&result);
 * @endcode
 *
 * @note Returns retained copy of original if text is NULL or empty
 * @note If str is DS_NULL_STRING, creates new string from text
 * @see ds_append(), ds_insert()
 */
DS_DEF ds_string ds_prepend(ds_string str, const char* text);

/**
 * @brief Insert text at a specific position in a string
 *
 * Creates a new string by inserting the given text at the specified byte
 * position in the input string. The original string is unchanged.
 *
 * @param str Source string (may be DS_NULL_STRING)
 * @param index Byte position where to insert text (0-based)
 * @param text Text to insert (may be NULL)
 * @return New string with inserted text, or DS_NULL_STRING on failure
 *
 * @par Example:
 * @code{.c}
 * ds_string base = ds_create("Hello World");
 * ds_string result = ds_insert(base, 6, "Beautiful ");
 *
 * printf("Result: %s\n", ds_cstr(result));  // "Hello Beautiful World"
 *
 * ds_release(&base);
 * ds_release(&result);
 * @endcode
 *
 * @warning Index is in bytes, not Unicode codepoints
 * @note Returns retained copy of original if text is NULL or empty
 * @note If index > string length, returns retained copy of original
 * @see ds_append(), ds_prepend()
 */
DS_DEF ds_string ds_insert(ds_string str, size_t index, const char* text);

/**
 * @brief Extract a substring from a string
 *
 * Creates a new string containing a portion of the input string, starting
 * at the specified byte position and extending for the given length.
 *
 * @param str Source string (may be DS_NULL_STRING)
 * @param start Starting byte position (0-based)
 * @param len Number of bytes to include in substring
 * @return New string containing the substring, or empty string if invalid range
 *
 * @par Example:
 * @code{.c}
 * ds_string text = ds_create("Hello World");
 * ds_string hello = ds_substring(text, 0, 5);    // "Hello"
 * ds_string world = ds_substring(text, 6, 5);    // "World"
 * ds_string partial = ds_substring(text, 6, 100); // "World" (clamped)
 *
 * ds_release(&text);
 * ds_release(&hello);
 * ds_release(&world);
 * ds_release(&partial);
 * @endcode
 *
 * @warning Positions are in bytes, not Unicode codepoints
 * @note If start >= string length, returns empty string
 * @note If start + len > string length, len is clamped to available data
 * @see ds_codepoint_at() for Unicode-aware character access
 */
DS_DEF ds_string ds_substring(ds_string str, size_t start, size_t len);

// String concatenation

/**
 * @brief Concatenate two strings
 *
 * Creates a new string by joining two strings together. This is equivalent
 * to ds_append() but takes two ds_string parameters instead of a string and C string.
 *
 * @param a First string (may be DS_NULL_STRING)
 * @param b Second string (may be DS_NULL_STRING)
 * @return New string containing a + b, or DS_NULL_STRING if both inputs are null
 *
 * @par Example:
 * @code{.c}
 * ds_string hello = ds_create("Hello");
 * ds_string world = ds_create(" World");
 * ds_string result = ds_concat(hello, world);
 *
 * printf("Result: %s\n", ds_cstr(result));  // "Hello World"
 *
 * ds_release(&hello);
 * ds_release(&world);
 * ds_release(&result);
 * @endcode
 *
 * @note If either string is DS_NULL_STRING, returns retained copy of the other
 * @note More efficient than ds_append() when both inputs are ds_string
 * @see ds_append(), ds_join()
 */
DS_DEF ds_string ds_concat(ds_string a, ds_string b);

/**
 * @brief Join multiple strings with a separator
 *
 * Creates a new string by joining an array of strings together with the
 * specified separator between each string.
 *
 * @param strings Array of ds_string to join (may contain DS_NULL_STRING entries)
 * @param count Number of strings in the array
 * @param separator Separator to insert between strings (may be NULL)
 * @return New string with all strings joined, or empty string if count is 0
 *
 * @par Example:
 * @code{.c}
 * ds_string words[] = {
 *     ds_create("The"),
 *     ds_create("quick"),
 *     ds_create("brown"),
 *     ds_create("fox")
 * };
 *
 * ds_string sentence = ds_join(words, 4, " ");
 * printf("Result: %s\n", ds_cstr(sentence));  // "The quick brown fox"
 *
 * ds_string csv = ds_join(words, 4, ",");
 * printf("CSV: %s\n", ds_cstr(csv));  // "The,quick,brown,fox"
 *
 * // Clean up
 * for (int i = 0; i < 4; i++) ds_release(&words[i]);
 * ds_release(&sentence);
 * ds_release(&csv);
 * @endcode
 *
 * @note DS_NULL_STRING entries in the array are treated as empty strings
 * @note If separator is NULL, strings are joined without separation
 * @note If count is 1, returns retained copy of the single string
 * @see ds_concat()
 */
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
