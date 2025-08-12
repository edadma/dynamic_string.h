# dynamic_string.h

A modern, efficient, single-file string library for C featuring:

- **Reference counting** with automatic memory management
- **Immutable strings** for safety and sharing
- **Copy-on-write StringBuilder** for efficient construction
- **Unicode support** with UTF-8 storage and codepoint iteration
- **Zero dependencies** - just drop in the `.h` file
- **FFI-friendly** design for use with other languages

## Quick Example

```c
#define DS_IMPLEMENTATION
#include "dynamic_string.h"

int main() {
    // Create and share strings efficiently
    ds_string greeting = ds_create("Hello");
    ds_string copy = ds_retain(greeting);  // Shares memory, no copying!
    
    // Immutable operations return new strings
    ds_string full = ds_append(greeting, " World! 🌍");
    
    // Unicode-aware iteration
    ds_codepoint_iter iter = ds_codepoints(full);
    uint32_t codepoint;
    while ((codepoint = ds_iter_next(&iter)) != 0) {
        printf("U+%04X ", codepoint);
    }
    
    // Automatic cleanup via reference counting
    ds_release(&greeting);
    ds_release(&copy);
    ds_release(&full);
    
    return 0;
}
```

## Features

### 🔄 Reference Counting
- **Automatic memory management** - no manual `free()` needed
- **Efficient sharing** - multiple variables can point to same string data
- **Copy-on-write** - strings are only copied when actually modified

### 🛡️ Immutable Strings
- **Thread-safe reading** - immutable strings can be safely shared between threads
- **Functional style** - operations return new strings, originals unchanged
- **No accidental modification** - prevents common C string bugs

### ⚡ Efficient StringBuilder
- **In-place construction** - builds the final string directly in its target location
- **Zero-copy conversion** - `ds_sb_to_string()` doesn't copy data
- **Reusable** - can continue using builder after conversion (copy-on-write)

### 🌍 Unicode Support
- **UTF-8 storage** - compact and C-compatible
- **Codepoint iteration** - Rust-style iterator for proper Unicode handling
- **Mixed text support** - seamlessly handle ASCII, emoji, and international text

### 📦 Single File Header
- **Easy integration** - just `#include "dynamic_string.h"`
- **No build dependencies** - self-contained implementation
- **Customizable** - override allocators and other behavior with macros

## Usage

### Basic Usage

```c
// In exactly ONE .c file:
#define DS_IMPLEMENTATION
#include "dynamic_string.h"

// In all other files:
#include "dynamic_string.h"
```

### String Operations

```c
// Creation
ds_string str = ds_create("Hello");
ds_string empty = ds_create("");

// Operations (return new strings)
ds_string result = ds_append(str, " World");
ds_string upper = ds_prepend(result, ">>> ");
ds_string sub = ds_substring(str, 1, 3);

// Sharing
ds_string shared = ds_retain(str);  // Increment reference count

// Cleanup
ds_release(&str);     // Decrement reference count
ds_release(&shared);  // Last reference - memory freed automatically
```

### StringBuilder for Efficient Construction

```c
// Build strings efficiently
ds_stringbuilder sb = ds_sb_create();

for (int i = 0; i < 1000; i++) {
    ds_sb_append(&sb, "data ");
    ds_sb_append_char(&sb, '0' + i % 10);
}

// Convert to immutable string (zero-copy!)
ds_string result = ds_sb_to_string(&sb);

// Can continue using builder (triggers copy-on-write)
ds_sb_append(&sb, " more");

ds_release(&result);
ds_sb_destroy(&sb);
```

### Unicode Handling

```c
ds_string unicode = ds_create("Hello 🌍 世界");

// Byte vs codepoint length
printf("Bytes: %zu\n", ds_length(unicode));           // 16 bytes  
printf("Codepoints: %zu\n", ds_codepoint_length(unicode));  // 9 codepoints

// Iterate through codepoints
ds_codepoint_iter iter = ds_codepoints(unicode);
uint32_t cp;
while ((cp = ds_iter_next(&iter)) != 0) {
    printf("U+%04X ", cp);
}

// Random access by codepoint
uint32_t emoji = ds_codepoint_at(unicode, 6);  // Gets 🌍
```

## Memory Layout

Each string uses a **single allocation** with metadata and data stored together:

```
[ref_count|length|H|e|l|l|o| |🌍|\0]
```

This provides:
- **Better cache locality** - metadata and data in same cache line
- **Fewer allocations** - no separate buffer allocation
- **Automatic null termination** - works with C string functions

## Configuration

Override behavior with macros before including:

```c
#define DS_MALLOC my_malloc
#define DS_FREE my_free  
#define DS_STATIC           // Make all functions static
#define DS_IMPLEMENTATION
#include "dynamic_string.h"
```

## Project Structure

```
your-project/
├── dynamic_string.h      # The library (copy this file)
├── example.c            # Usage examples
├── CMakeLists.txt       # Build configuration
└── README.md           # This file
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

## License

Dual licensed under your choice of:
- [MIT License](LICENSE-MIT)
- [The Unlicense](LICENSE-UNLICENSE) (public domain)

Choose whichever works better for your project!

## Design Philosophy

This library brings modern string handling to C by combining:

- **Rust's approach** - UTF-8 storage with codepoint iteration
- **Swift's model** - reference counting with copy-on-write
- **STB-style delivery** - single header file, easy integration
- **C compatibility** - works with existing C string functions

The result is a string library that's both **safe and fast**, making C string handling as pleasant as higher-level languages while maintaining C's performance characteristics.