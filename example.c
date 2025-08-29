#define DS_IMPLEMENTATION
#include "dynamic_string.h"

void demonstrate_immutability() {
    printf("=== Immutable Strings Demo ===\n");

    ds_string base = ds("Hello");
    printf("Base: '%s' (refs: %zu)\n", base, ds_refcount(base));

    // Operations return NEW strings - original is unchanged
    ds_string with_world = ds_append(base, " World");
    ds_string with_exclamation = ds_append(with_world, "!");
    ds_string with_prefix = ds_prepend(base, "Hi! ");

    printf("After operations:\n");
    printf("  Base: '%s' (unchanged!)\n", base);
    printf("  With world: '%s'\n", with_world);
    printf("  With exclamation: '%s'\n", with_exclamation);
    printf("  With prefix: '%s'\n", with_prefix);

    // Each string is independent
    ds_release(&base);
    ds_release(&with_world);
    ds_release(&with_exclamation);
    ds_release(&with_prefix);
}

void demonstrate_sharing() {
    printf("\n=== Efficient Sharing Demo ===\n");

    ds_string original = ds("This string will be shared efficiently");
    printf("Original: '%s' (refs: %zu)\n", original, ds_refcount(original));

    // Create many references - all share the same memory block
    ds_string shared[5];
    for (int i = 0; i < 5; i++) {
        shared[i] = ds_retain(original);
    }

    printf("After creating 5 shared references:\n");
    printf("  Ref count: %zu\n", ds_refcount(original));
    printf("  All point to same memory: %s\n", (shared[0] == shared[1] && shared[1] == shared[2]) ? "YES" : "NO");

    // Create a new string from one of the shared references
    ds_string modified = ds_append(shared[2], " + addition");
    printf("  Modified: '%s' (refs: %zu)\n", modified, ds_refcount(modified));
    printf("  Original unchanged: '%s' (refs: %zu)\n", original, ds_refcount(original));

    // Clean up
    ds_release(&original);
    for (int i = 0; i < 5; i++) {
        ds_release(&shared[i]);
    }
    ds_release(&modified);
}

void demonstrate_functional_style() {
    printf("\n=== Functional Style Demo ===\n");

    // Chain operations naturally
    ds_string result = ds_prepend(ds_append(ds_append(ds("Hello"), " beautiful"), " world"), ">> ");

    printf("Chained operations: '%s'\n", result);

    // String building with join
    ds_string words[] = {ds("The"), ds("quick"), ds("brown"), ds("fox")};

    ds_string sentence = ds_join(words, 4, " ");
    printf("Joined: '%s'\n", sentence);

    // Substrings
    ds_string quick = ds_substring(sentence, 4, 5);
    ds_string brown = ds_substring(sentence, 10, 5);

    printf("Substrings: '%s' and '%s'\n", quick, brown);

    // Clean up
    ds_release(&result);
    for (int i = 0; i < 4; i++) {
        ds_release(&words[i]);
    }
    ds_release(&sentence);
    ds_release(&quick);
    ds_release(&brown);
}

void demonstrate_memory_efficiency() {
    printf("\n=== Memory Efficiency Demo ===\n");

    ds_string base = ds("Base string for efficiency test");

    printf("Memory layout (single allocation per string):\n");
    printf("  String data stored inline with metadata\n");
    printf("  No separate buffer allocation\n");
    printf("  Better cache locality\n");

    // Show that empty operations return shared instances when possible
    ds_string empty1 = ds_append(base, ""); // Appending nothing
    printf("  Appending empty string shares reference: %s\n", (empty1 == base) ? "YES" : "NO");

    ds_string empty2 = ds_substring(base, 0, 0); // Empty substring
    printf("  Empty substring: '%s' (length: %zu)\n", empty2, ds_length(empty2));

    // String comparison is optimized for shared strings
    ds_string shared = ds_retain(base);
    printf("  Comparing shared strings (fast path): %s\n", (ds_compare(base, shared) == 0) ? "Equal" : "Not equal");

    ds_release(&base);
    ds_release(&empty1);
    ds_release(&empty2);
    ds_release(&shared);
}

void demonstrate_stringbuilder() {
    printf("\n=== StringBuilder Demo ===\n");

    // Create a StringBuilder for efficient building
    ds_builder sb = ds_builder_create();
    printf("Created StringBuilder (capacity: %zu)\n", ds_builder_capacity(&sb));

    // Build a string efficiently
    ds_builder_append(&sb, "Building");
    ds_builder_append(&sb, " a");
    ds_builder_append(&sb, " string");
    ds_builder_append(&sb, " efficiently");

    printf("After building: '%s' (length: %zu, capacity: %zu)\n", ds_builder_cstr(&sb), ds_builder_length(&sb),
           ds_builder_capacity(&sb));

    // THE MAGIC: Convert to immutable string
    ds_string result = ds_builder_to_string(&sb);

    printf("Converted to ds_string: '%s' (refs: %zu)\n", result, ds_refcount(result));
    printf("StringBuilder after conversion (capacity: %zu)\n", ds_builder_capacity(&sb));
    printf("StringBuilder and string share data: %s\n", (sb.data == result) ? "YES" : "NO");

    // Continue using StringBuilder - this triggers copy-on-write!
    printf("\nContinuing to use StringBuilder (should trigger COW)...\n");
    ds_builder_append(&sb, " + more text");

    printf("StringBuilder: '%s' (refs: %zu)\n", ds_builder_cstr(&sb), ds_refcount((ds_string){sb.data}));
    printf("Original string: '%s' (refs: %zu)\n", result, ds_refcount(result));
    printf("Now they're separate: %s\n", (sb.data != result) ? "YES" : "NO");

    // Clean up
    ds_release(&result);
    ds_builder_release(&sb);
}

void demonstrate_builder_efficiency() {
    printf("\n=== Builder Efficiency Demo ===\n");

    // Build a large string efficiently
    ds_builder sb = ds_builder_create_with_capacity(1000);

    printf("Building large string with capacity: %zu\n", ds_builder_capacity(&sb));

    // Add many parts
    for (int i = 0; i < 100; i++) {
        ds_builder_append(&sb, "Part ");
        ds_builder_append_char(&sb, '0' + (i % 10));
        ds_builder_append(&sb, " ");
    }

    printf("Built string with %zu characters\n", ds_builder_length(&sb));
    printf("Final capacity: %zu (growth happened automatically)\n", ds_builder_capacity(&sb));

    // Convert to immutable string - no copying needed!
    ds_string final = ds_builder_to_string(&sb);
    printf("Converted to immutable string (ref count: %zu)\n", ds_refcount(final));

    // The string was built in-place and is now ready to use
    printf("First 50 chars: '%.50s...'\n", final);

    ds_release(&final);
    ds_builder_release(&sb);
}

void demonstrate_unicode_iteration() {
    printf("\n=== Unicode Codepoint Iteration Demo ===\n");

    // Create a string with mixed ASCII and Unicode
    ds_string unicode_str = ds("Hello 🌍 World! 你好 🚀");

    printf("String: '%s'\n", unicode_str);
    printf("Byte length: %zu\n", ds_length(unicode_str));
    printf("Codepoint length: %zu\n", ds_codepoint_length(unicode_str));

    // Iterate through all codepoints (Rust-style)
    printf("\nIterating through codepoints:\n");
    ds_codepoint_iter iter = ds_codepoints(unicode_str);
    size_t index = 0;
    uint32_t codepoint;

    while ((codepoint = ds_iter_next(&iter)) != 0) {
        if (codepoint <= 0x7F) {
            printf("  [%zu] U+%04X '%c' (ASCII)\n", index, codepoint, (char)codepoint);
        } else {
            printf("  [%zu] U+%04X (Unicode)\n", index, codepoint);
        }
        index++;
    }

    // Access specific codepoints by index
    printf("\nAccessing specific codepoints:\n");
    printf("  Codepoint at index 6: U+%04X\n", ds_codepoint_at(unicode_str, 6)); // Should be 🌍
    printf("  Codepoint at index 15: U+%04X\n", ds_codepoint_at(unicode_str, 15)); // Should be 你

    // Demonstrate emoji detection
    printf("\nEmoji detection:\n");
    ds_codepoint_iter emoji_iter = ds_codepoints(unicode_str);
    index = 0;
    while ((codepoint = ds_iter_next(&emoji_iter)) != 0) {
        // Simple emoji range check (this is just an example - real emoji detection is more complex)
        if ((codepoint >= 0x1F600 && codepoint <= 0x1F64F) || // Emoticons
            (codepoint >= 0x1F300 && codepoint <= 0x1F5FF) || // Misc Symbols
            (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) || // Transport
            (codepoint >= 0x2600 && codepoint <= 0x26FF)) { // Misc symbols
            printf("  Found emoji at index %zu: U+%04X\n", index, codepoint);
        }
        index++;
    }

    ds_release(&unicode_str);
}

void demonstrate_unicode_vs_bytes() {
    printf("\n=== Unicode vs Byte Operations Demo ===\n");

    ds_string str1 = ds("ASCII");
    ds_string str2 = ds("🚀🌍🎉");
    ds_string str3 = ds("Mixed: A🚀B🌍C🎉");

    printf("String comparisons:\n");
    printf("'%s': %zu bytes, %zu codepoints\n", str1, ds_length(str1), ds_codepoint_length(str1));
    printf("'%s': %zu bytes, %zu codepoints\n", str2, ds_length(str2), ds_codepoint_length(str2));
    printf("'%s': %zu bytes, %zu codepoints\n", str3, ds_length(str3), ds_codepoint_length(str3));

    // Show that byte indexing vs codepoint indexing are different
    printf("\nByte vs Codepoint indexing in '%s':\n", str3);
    printf("Codepoint[0]: U+%04X\n", ds_codepoint_at(str3, 0)); // 'M'
    printf("Codepoint[8]: U+%04X\n", ds_codepoint_at(str3, 8)); // 'A'
    printf("Codepoint[9]: U+%04X\n", ds_codepoint_at(str3, 9)); // 🚀
    printf("Codepoint[10]: U+%04X\n", ds_codepoint_at(str3, 10)); // 'B'

    ds_release(&str1);
    ds_release(&str2);
    ds_release(&str3);
}

int main() {
    demonstrate_immutability();
    demonstrate_sharing();
    demonstrate_functional_style();
    demonstrate_memory_efficiency();
    demonstrate_stringbuilder();
    demonstrate_builder_efficiency();
    demonstrate_unicode_iteration();
    demonstrate_unicode_vs_bytes();

    printf("\n=== Summary ===\n");
    printf("✓ Immutable strings with reference counting\n");
    printf("✓ Efficient StringBuilder for construction\n");
    printf("✓ In-place building with realloc optimization\n");
    printf("✓ Copy-on-write when builder is reused\n");
    printf("✓ Unicode-aware codepoint iteration (Rust-style)\n");
    printf("✓ UTF-8 storage with proper Unicode handling\n");
    printf("✓ Single allocation per string (metadata + data together)\n");
    printf("✓ Functional programming style supported\n");
    printf("✓ Memory safe with automatic cleanup\n");
    printf("✓ Direct C compatibility - no ds_cstr() needed!\n");

    return 0;
}
