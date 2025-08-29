# dynamic_string.h

A modern, efficient, single-file string library for C featuring:

- **Direct C compatibility** - `ds_string` works with ALL C functions (no conversion needed!)
- **Reference counting** with automatic memory management
- **Immutable strings** for safety and sharing
- **Efficient StringBuilder** for string construction
- **Unicode support** with UTF-8 storage and codepoint iteration
- **Zero dependencies** - just drop in the `.h` file
- **Memory safe** - eliminates common C string bugs

## Quick Example

```c
#define DS_IMPLEMENTATION
#include "dynamic_string.h"

int main() {
    // Create strings with the short, clean API
    ds_string greeting = ds_new("Hello");
    ds_string full = ds_append(greeting, " World! 🌍");
    
    // Works directly with ALL C functions - no conversion needed!
    printf("%s\n", full);                    // Direct usage!
    int result = strcmp(full, "Hello");      // Works with strcmp!
    FILE* f = fopen(full, "r");              // Works with fopen!
    
    // Unicode-aware iteration
    ds_codepoint_iter iter = ds_codepoints(full);
    uint32_t codepoint;
    while ((codepoint = ds_iter_next(&iter)) != 0) {
        printf("U+%04X ", codepoint);
    }
    
    // Automatic cleanup via reference counting
    ds_release(&greeting);
    ds_release(&full);
    
    return 0;
}
```

## Major Features

### 🔗 Direct C Compatibility

The biggest advantage: `ds_string` IS a `char*` that works with every C function:

```c
ds_string path = ds_new("/tmp/file.txt");
FILE* f = fopen(path, "r");              // Direct usage!
if (strstr(path, "tmp")) { ... }         // Works with strstr!
printf("Path: %s\n", path);              // No conversion needed!
```

### 🔄 Reference Counting

- **Automatic memory management** - no manual `free()` needed
- **Efficient sharing** - multiple variables can point to same string data
- **Copy-on-write** - strings are only copied when actually modified

```c
ds_string original = ds_new("Shared data");
ds_string copy = ds_retain(original);    // Shares memory, no copying!
printf("Ref count: %zu\n", ds_refcount(original));  // 2

ds_release(&original);
ds_release(&copy);                       // Automatic cleanup
```

### 🛡️ Immutable Strings

- **Thread-safe reading** - immutable strings can be safely shared between threads
- **Functional style** - operations return new strings, originals unchanged
- **No accidental modification** - prevents common C string bugs

```c
ds_string base = ds_new("Hello");
ds_string modified = ds_append(base, " World");
// base is still "Hello", modified is "Hello World"
```

### ⚡ Efficient StringBuilder

- **In-place construction** - builds strings efficiently without intermediate allocations
- **Reference-counted** - safe sharing between functions
- **Rich API** - formatting, numeric operations, and content manipulation
- **Automatic growth** - handles capacity management

```c
ds_builder sb = ds_builder_create();
ds_builder_append_format(sb, "Processing %d items:\n", 1000);
for (int i = 0; i < 1000; i++) {
    ds_builder_append_format(sb, "Item %d: ", i);
    ds_builder_append_double(sb, i * 3.14, 2);
    ds_builder_append(sb, "\n");
}
ds_string result = ds_builder_to_string(sb);  // sb becomes consumed
ds_builder_release(&sb);  // Always safe to call
```

### 🌍 Unicode Support

- **UTF-8 storage** - compact and C-compatible
- **Codepoint iteration** - proper Unicode handling
- **Mixed text support** - seamlessly handle ASCII, emoji, and international text

```c
ds_string unicode = ds_new("Hello 🌍 世界");
printf("Bytes: %zu, Codepoints: %zu\n", 
       ds_length(unicode), ds_codepoint_length(unicode));

// Iterate through codepoints
ds_codepoint_iter iter = ds_codepoints(unicode);
uint32_t cp;
while ((cp = ds_iter_next(&iter)) != 0) {
    printf("U+%04X ", cp);
}
```

## ⚠️ Error Handling

**Important: NULL Parameter Validation**

As of v0.3.0, all functions perform strict NULL parameter validation using assertions:

```c
// ✅ Valid usage
ds_string str = ds_new("Hello");
size_t len = ds_length(str);  // Works fine
ds_release(&str);

// ❌ Will trigger assertion failure
size_t len = ds_length(NULL);  // Assertion: ds_length: str cannot be NULL
ds_string result = ds_append(NULL, "text");  // Assertion: ds_append: str cannot be NULL
```

**Benefits:**
- **Immediate error detection** - bugs are caught at the source
- **Clear error messages** - assertions specify exactly which parameter is NULL
- **Consistent behavior** - all functions handle NULL parameters the same way
- **Development safety** - prevents silent failures and undefined behavior

**Migration from older versions:**
- Code that passes valid (non-NULL) parameters continues to work unchanged
- Code that relied on graceful NULL handling will now trigger assertions
- Use a debugger or assertion handler to catch and fix NULL parameter issues

## Usage Patterns

### Basic Usage

```c
// In exactly ONE .c file:
#define DS_IMPLEMENTATION
#include "dynamic_string.h"

// In all other files:
#include "dynamic_string.h"
```

### Correct Memory Management Patterns

**Simple operations:**

```c
ds_string str = ds_new("Hello");
// Use str directly with any C function...
ds_release(&str);  // Always clean up
```

**Transform and replace:**

```c
ds_string str = ds_new("Hello");
ds_string new_str = ds_append(str, " World");
ds_release(&str);  // Release original
str = new_str;     // Replace reference
// Use str...
ds_release(&str);
```

**Multiple operations - use StringBuilder:**

```c
ds_builder sb = ds_builder_create();
ds_builder_append(&sb, "Building ");
ds_builder_append(&sb, "efficiently");
ds_string result = ds_builder_to_string(&sb);  // sb consumed here
ds_builder_release(&sb);  // Always safe to call
// Use result...
ds_release(&result);
```

**❌ Don't do this (memory leaks):**

```c
ds_string str = ds_new("Hello");
str = ds_append(str, " World");  // LEAK! Lost reference to "Hello"
str = ds_append(str, "!");       // LEAK! Lost reference to "Hello World"
```

### When to Use StringBuilder vs Simple Operations

**Use simple operations for:**

- One or two concatenations
- Transforming existing strings
- Functional-style processing

**Use StringBuilder for:**

- Building strings in loops
- Multiple concatenations (3+)
- Performance-critical string construction
- When you don't know final size

## API Reference

### Core Functions

```c
// Creation and memory management
ds_string ds_new(const char* text);                    // Create string
ds_string ds_create_length(const char* text, size_t length);
ds_string ds_retain(ds_string str);                // Share reference
void ds_release(ds_string* str);                   // Release reference

// String operations (immutable - return new strings)
ds_string ds_append(ds_string str, const char* text);
ds_string ds_append_char(ds_string str, uint32_t codepoint);
ds_string ds_prepend(ds_string str, const char* text);
ds_string ds_insert(ds_string str, size_t index, const char* text);
ds_string ds_substring(ds_string str, size_t start, size_t len);
ds_string ds_concat(ds_string a, ds_string b);
ds_string ds_join(ds_string* strings, size_t count, const char* separator);

// Utility functions
size_t ds_length(ds_string str);
size_t ds_refcount(ds_string str);
int ds_compare(ds_string a, ds_string b);
int ds_find(ds_string str, const char* needle);
int ds_starts_with(ds_string str, const char* prefix);
int ds_ends_with(ds_string str, const char* suffix);
int ds_is_shared(ds_string str);
int ds_is_empty(ds_string str);
```

### StringBuilder Functions

```c
// StringBuilder creation and management
ds_builder ds_builder_create(void);
ds_builder ds_builder_create_with_capacity(size_t capacity);
ds_builder ds_builder_retain(ds_builder sb);
void ds_builder_release(ds_builder* sb);

// Basic building operations (modify in-place)
int ds_builder_append(ds_builder sb, const char* text);
int ds_builder_append_char(ds_builder sb, uint32_t codepoint);
int ds_builder_append_string(ds_builder sb, ds_string str);
int ds_builder_insert(ds_builder sb, size_t index, const char* text);
void ds_builder_clear(ds_builder sb);

// Formatting functions (NEW in v0.3.1)
int ds_builder_append_format(ds_builder sb, const char* fmt, ...);
int ds_builder_append_format_v(ds_builder sb, const char* fmt, va_list args);

// Numeric append functions (NEW in v0.3.1)
int ds_builder_append_int(ds_builder sb, int value);
int ds_builder_append_uint(ds_builder sb, unsigned int value);
int ds_builder_append_long(ds_builder sb, long value);
int ds_builder_append_double(ds_builder sb, double value, int precision);

// Buffer operations (NEW in v0.3.1)
int ds_builder_append_length(ds_builder sb, const char* text, size_t length);
int ds_builder_prepend(ds_builder sb, const char* text);
int ds_builder_replace_range(ds_builder sb, size_t start, size_t end, const char* replacement);

// Content manipulation (NEW in v0.3.1)
int ds_builder_remove_range(ds_builder sb, size_t start, size_t length);

// Conversion (StringBuilder becomes consumed)
ds_string ds_builder_to_string(ds_builder sb);

// Inspection
size_t ds_builder_length(ds_builder sb);
size_t ds_builder_capacity(ds_builder sb);
const char* ds_builder_cstr(ds_builder sb);
```

### Unicode Functions

```c
// Unicode iteration and analysis
ds_codepoint_iter ds_codepoints(ds_string str);
uint32_t ds_iter_next(ds_codepoint_iter* iter);
int ds_iter_has_next(const ds_codepoint_iter* iter);
size_t ds_codepoint_length(ds_string str);
uint32_t ds_codepoint_at(ds_string str, size_t index);
```

### Convenience Macros

```c
#define ds_empty() ds_new("")
#define ds_from_literal(lit) ds_new(lit)
```

## Configuration

Override behavior with macros before including:

```c
#define DS_MALLOC my_malloc
#define DS_FREE my_free  
#define DS_REALLOC my_realloc
#define DS_STATIC           // Make all functions static
#define DS_IMPLEMENTATION
#include "dynamic_string.h"
```

## Building Examples

```bash
git clone https://github.com/yourusername/dynamic_string.h
cd dynamic_string.h
mkdir build && cd build
cmake ..
make
./example
```

## Version History

### v0.3.1 (Current)
- **Added**: `ds_builder_append_format()` and `ds_builder_append_format_v()` - Printf-style formatting directly into StringBuilder
- **Added**: Numeric append functions: `ds_builder_append_int()`, `ds_builder_append_uint()`, `ds_builder_append_long()`, `ds_builder_append_double()`
- **Added**: Buffer operations: `ds_builder_append_length()`, `ds_builder_prepend()`, `ds_builder_replace_range()`
- **Added**: Content manipulation: `ds_builder_remove_range()`
- **Documentation**: Complete Doxygen documentation for all new StringBuilder functions
- **Documentation**: Added comprehensive unit tests for all new functions (83 total tests)
- **Enhancement**: StringBuilder now supports advanced string building operations for more efficient construction

### v0.3.0
- **BREAKING CHANGE**: Renamed `ds_stringbuilder` to `ds_builder` for consistency
- **BREAKING CHANGE**: `ds_builder` is now heap-allocated and reference-counted like `ds_string`
- **BREAKING CHANGE**: Replaced `ds_builder_destroy()` with `ds_builder_release()` that sets pointer to NULL
- **Added**: `ds_builder_retain()` for safe sharing of builders between different parts of code
- **Changed**: Builder functions now take `ds_builder` (pointer) instead of `ds_builder*`
- **Improved**: Consistent retain/release semantics across both `ds_string` and `ds_builder`
- **Fixed**: Memory management is now fully reference-counted for builders

### v0.2.2
- **BREAKING CHANGE**: All functions now assert on NULL `ds_string` parameters instead of handling them gracefully
- **API Safety**: Consistent assertion-based parameter validation across entire API
- **Error Detection**: NULL parameters now trigger immediate assertion failures with descriptive error messages
- **Testing**: Implemented comprehensive assertion testing framework with signal handling
- **Documentation**: Updated all function documentation to specify parameters "must not be NULL"
- **Compatibility**: Maintains 100% backward compatibility for valid (non-NULL) usage patterns
- **Testing**: All 71 tests pass with new assertion-based error handling

### v0.2.1
- **Critical Fix**: Buffer overflow vulnerability in `ds_create_length()` - now correctly handles requested length vs source string length
- **Critical Fix**: Segmentation fault in test suite caused by dangerous NULL parameter testing
- **Security**: Added comprehensive NULL parameter validation with assertions for `ds_create_length()`, `ds_prepend()`, `ds_insert()`, `ds_substring()`, `ds_replace()`, and `ds_replace_all()`
- **Fixed**: `ds_insert()` beyond-bounds behavior now inserts at string end instead of returning unchanged
- **Testing**: Corrected 6 failing tests that had incorrect expectations, all 73 tests now pass
- **API Safety**: Functions now fail fast with clear assertion messages instead of silent undefined behavior

### v0.2.0

- **Added**: Atomic reference counting support (`DS_ATOMIC_REFCOUNT`) for safe concurrent reference sharing
- **Added**: `ds_contains()` - clean boolean check for substring presence
- **Added**: `ds_find_last()` - find last occurrence of substring
- **Added**: `ds_hash()` - FNV-1a hash function for use in hash tables
- **Added**: `ds_compare_ignore_case()` - case-insensitive string comparison
- **Added**: `ds_truncate()` - smart truncation with optional ellipsis
- **Added**: `ds_format_v()` - va_list version of ds_format for wrapper functions
- **Added**: `ds_escape_json()` / `ds_unescape_json()` - JSON string escaping/unescaping
- **Removed**: Version macros (not needed for single-header libraries)
- **Improved**: All complex string building operations now use StringBuilder internally for efficiency
- **Documentation**: Added CLAUDE.md for AI assistance and comprehensive usage examples

### v0.1.0

- **Added**: Version macros (`DS_VERSION_MAJOR`, `DS_VERSION_MINOR`, `DS_VERSION_PATCH`, `DS_VERSION_STRING`)
- **Added**: Programmatic version checking with `DS_VERSION` macro
- **Documentation**: Comprehensive doxygen comments with code examples and cross-references
- **Build**: GitHub Actions workflow for automatic documentation generation and deployment

### v0.0.2

- **Breaking**: `ds()` renamed to `ds_new()` for clarity
- **Breaking**: `ds_sb_*` functions renamed to `ds_builder_*` for readability
- **Breaking**: Removed `ds_cstr()` - direct C compatibility
- **Breaking**: `ds_ref_count()` renamed to `ds_refcount()`
- **Breaking**: StringBuilder becomes single-use after `ds_builder_to_string()`
- **Fixed**: Memory corruption bugs in StringBuilder
- **Improved**: Consistent internal API usage
- **Added**: String transformation functions (`ds_trim`, `ds_split`, `ds_format`)

### v0.0.1

- Initial release with basic functionality

## Memory Layout

Each string uses a **single allocation** with metadata and data stored together:

```
Memory: [refcount|length|string_data|\0]
                         ^
           ds_string points here
```

This provides:

- **Better cache locality** - metadata and data in same allocation
- **Fewer allocations** - no separate buffer allocation
- **Direct C compatibility** - pointer points to actual string data
- **Automatic null termination** - works with C string functions

## License

Dual licensed under your choice of:

- [MIT License](LICENSE-MIT)
- [The Unlicense](LICENSE-UNLICENSE) (public domain)

Choose whichever works better for your project!

## Design Philosophy

This library brings modern string handling to C by combining:

- **Rust's approach** - UTF-8 storage with codepoint iteration
- **Swift's model** - reference counting with automatic memory management
- **STB-style delivery** - single header file, easy integration
- **C compatibility** - works seamlessly with existing C code

The result is a string library that's both **safe and fast**, making C string handling as pleasant as higher-level
languages while maintaining C's performance characteristics and perfect compatibility with existing C APIs.