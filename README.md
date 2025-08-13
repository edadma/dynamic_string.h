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
    ds_string greeting = ds("Hello");
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
ds_string path = ds("/tmp/file.txt");
FILE* f = fopen(path, "r");              // Direct usage!
if (strstr(path, "tmp")) { ... }         // Works with strstr!
printf("Path: %s\n", path);              // No conversion needed!
```

### 🔄 Reference Counting

- **Automatic memory management** - no manual `free()` needed
- **Efficient sharing** - multiple variables can point to same string data
- **Copy-on-write** - strings are only copied when actually modified

```c
ds_string original = ds("Shared data");
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
ds_string base = ds("Hello");
ds_string modified = ds_append(base, " World");
// base is still "Hello", modified is "Hello World"
```

### ⚡ Efficient StringBuilder

- **In-place construction** - builds strings efficiently without intermediate allocations
- **Single-use design** - simple and memory-safe
- **Automatic growth** - handles capacity management

```c
ds_stringbuilder sb = ds_sb_create();
for (int i = 0; i < 1000; i++) {
    ds_sb_append(&sb, "data ");
}
ds_string result = ds_sb_to_string(&sb);  // sb becomes consumed
ds_sb_destroy(&sb);  // Always safe to call
```

### 🌍 Unicode Support

- **UTF-8 storage** - compact and C-compatible
- **Codepoint iteration** - proper Unicode handling
- **Mixed text support** - seamlessly handle ASCII, emoji, and international text

```c
ds_string unicode = ds("Hello 🌍 世界");
printf("Bytes: %zu, Codepoints: %zu\n", 
       ds_length(unicode), ds_codepoint_length(unicode));

// Iterate through codepoints
ds_codepoint_iter iter = ds_codepoints(unicode);
uint32_t cp;
while ((cp = ds_iter_next(&iter)) != 0) {
    printf("U+%04X ", cp);
}
```

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
ds_string str = ds("Hello");
// Use str directly with any C function...
ds_release(&str);  // Always clean up
```

**Transform and replace:**

```c
ds_string str = ds("Hello");
ds_string new_str = ds_append(str, " World");
ds_release(&str);  // Release original
str = new_str;     // Replace reference
// Use str...
ds_release(&str);
```

**Multiple operations - use StringBuilder:**

```c
ds_stringbuilder sb = ds_sb_create();
ds_sb_append(&sb, "Building ");
ds_sb_append(&sb, "efficiently");
ds_string result = ds_sb_to_string(&sb);  // sb consumed here
ds_sb_destroy(&sb);  // Always safe to call
// Use result...
ds_release(&result);
```

**❌ Don't do this (memory leaks):**

```c
ds_string str = ds("Hello");
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
ds_string ds(const char* text);                    // Create string
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
ds_stringbuilder ds_sb_create(void);
ds_stringbuilder ds_sb_create_with_capacity(size_t capacity);
void ds_sb_destroy(ds_stringbuilder* sb);

// Building operations (modify in-place)
int ds_sb_append(ds_stringbuilder* sb, const char* text);
int ds_sb_append_char(ds_stringbuilder* sb, uint32_t codepoint);
int ds_sb_append_string(ds_stringbuilder* sb, ds_string str);
int ds_sb_insert(ds_stringbuilder* sb, size_t index, const char* text);
void ds_sb_clear(ds_stringbuilder* sb);

// Conversion (StringBuilder becomes consumed)
ds_string ds_sb_to_string(ds_stringbuilder* sb);

// Inspection
size_t ds_sb_length(const ds_stringbuilder* sb);
size_t ds_sb_capacity(const ds_stringbuilder* sb);
const char* ds_sb_cstr(const ds_stringbuilder* sb);
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
#define ds_empty() ds("")
#define ds_from_literal(lit) ds(lit)
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

### v0.0.2 (Current)

- **Breaking**: `ds_create()` renamed to `ds()` for brevity
- **Breaking**: Removed `ds_cstr()` - direct C compatibility
- **Breaking**: `ds_ref_count()` renamed to `ds_refcount()`
- **Breaking**: StringBuilder becomes single-use after `ds_sb_to_string()`
- **Fixed**: Memory corruption bugs in StringBuilder
- **Improved**: Consistent internal API usage

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