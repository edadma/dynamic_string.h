#define DS_IMPLEMENTATION
#include "dynamic_string.h"
#include "unity.h"

// ============================================================================
// SETUP/TEARDOWN
// ============================================================================

void setUp(void) {
    // Unity calls this before each test
}

void tearDown(void) {
    // Unity calls this after each test
}

// ============================================================================
// BASIC FUNCTIONALITY TESTS (at least one test for simple functions)
// ============================================================================

void test_basic_string_creation(void) {
    ds_string str = ds("Hello");
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Hello", str);
    TEST_ASSERT_EQUAL_UINT(5, ds_length(str));
    TEST_ASSERT_EQUAL_UINT(1, ds_refcount(str));
    ds_release(&str);
    TEST_ASSERT_NULL(str);
}

void test_basic_append(void) {
    ds_string str = ds("Hello");
    ds_string result = ds_append(str, " World");
    TEST_ASSERT_EQUAL_STRING("Hello World", result);
    TEST_ASSERT_EQUAL_STRING("Hello", str); // Original unchanged
    ds_release(&str);
    ds_release(&result);
}

void test_basic_compare(void) {
    ds_string a = ds("Hello");
    ds_string b = ds("Hello");
    ds_string c = ds("World");
    TEST_ASSERT_EQUAL_INT(0, ds_compare(a, b));
    TEST_ASSERT_TRUE(ds_compare(a, c) < 0);
    TEST_ASSERT_TRUE(ds_compare(c, a) > 0);

    ds_release(&a);
    ds_release(&b);
    ds_release(&c);
}

// ============================================================================
// REFERENCE COUNTING EDGE CASES (highest priority)
// ============================================================================

void test_multiple_retains_releases(void) {
    ds_string original = ds("Shared string");
    TEST_ASSERT_EQUAL_UINT(1, ds_refcount(original));

    ds_string ref1 = ds_retain(original);
    TEST_ASSERT_EQUAL_UINT(2, ds_refcount(original));
    TEST_ASSERT_EQUAL_UINT(2, ds_refcount(ref1));
    TEST_ASSERT_TRUE(original == ref1); // Same pointer

    ds_string ref2 = ds_retain(original);
    TEST_ASSERT_EQUAL_UINT(3, ds_refcount(original));

    ds_release(&ref1);
    TEST_ASSERT_NULL(ref1);
    TEST_ASSERT_EQUAL_UINT(2, ds_refcount(original));

    ds_release(&ref2);
    TEST_ASSERT_EQUAL_UINT(1, ds_refcount(original));

    ds_release(&original);
    TEST_ASSERT_NULL(original);
}

void test_shared_string_immutability(void) {
    ds_string original = ds("Base");
    ds_string shared = ds_retain(original);

    TEST_ASSERT_TRUE(ds_is_shared(original));
    TEST_ASSERT_TRUE(ds_is_shared(shared));

    // Operations on shared strings should return new strings
    ds_string modified = ds_append(shared, " Modified");
    TEST_ASSERT_EQUAL_STRING("Base", original);
    TEST_ASSERT_EQUAL_STRING("Base", shared);
    TEST_ASSERT_EQUAL_STRING("Base Modified", modified);

    // Reference counts should be correct
    TEST_ASSERT_EQUAL_UINT(2, ds_refcount(original)); // original + shared
    TEST_ASSERT_EQUAL_UINT(1, ds_refcount(modified)); // new string

    ds_release(&original);
    ds_release(&shared);
    ds_release(&modified);
}

void test_release_null_safety(void) {
    ds_string null_str = NULL;
    ds_release(&null_str); // Should not crash
    TEST_ASSERT_NULL(null_str);

    ds_string str = ds("Test");
    ds_release(&str);
    TEST_ASSERT_NULL(str);
    ds_release(&str); // Double release should be safe
    TEST_ASSERT_NULL(str);
}

void test_retain_null_safety(void) {
    ds_string null_str = NULL;
    ds_string result = ds_retain(null_str);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// STRINGBUILDER STATE TRANSITIONS (second highest priority)
// ============================================================================

void test_stringbuilder_basic_usage(void) {
    ds_stringbuilder sb = ds_sb_create();
    TEST_ASSERT_NOT_NULL(sb.data);
    TEST_ASSERT_TRUE(ds_sb_capacity(&sb) > 0);
    TEST_ASSERT_EQUAL_UINT(0, ds_sb_length(&sb));

    TEST_ASSERT_TRUE(ds_sb_append(&sb, "Hello"));
    TEST_ASSERT_EQUAL_STRING("Hello", ds_sb_cstr(&sb));
    TEST_ASSERT_EQUAL_UINT(5, ds_sb_length(&sb));

    ds_sb_destroy(&sb);
    TEST_ASSERT_NULL(sb.data);
    TEST_ASSERT_EQUAL_UINT(0, ds_sb_capacity(&sb));
}

void test_stringbuilder_to_string_consumption(void) {
    ds_stringbuilder sb = ds_sb_create();
    ds_sb_append(&sb, "Test content");

    size_t original_capacity = ds_sb_capacity(&sb);
    TEST_ASSERT_TRUE(original_capacity > 0);

    // Convert to string - this should consume the StringBuilder
    ds_string result = ds_sb_to_string(&sb);
    TEST_ASSERT_EQUAL_STRING("Test content", result);
    TEST_ASSERT_EQUAL_UINT(1, ds_refcount(result));

    // StringBuilder should be consumed (empty)
    TEST_ASSERT_NULL(sb.data);
    TEST_ASSERT_EQUAL_UINT(0, ds_sb_capacity(&sb));

    // Should be safe to destroy consumed StringBuilder
    ds_sb_destroy(&sb);

    ds_release(&result);
}

void test_stringbuilder_capacity_growth(void) {
    ds_stringbuilder sb = ds_sb_create_with_capacity(8);
    TEST_ASSERT_TRUE(ds_sb_capacity(&sb) >= 8);

    // Fill beyond initial capacity
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_TRUE(ds_sb_append(&sb, "X"));
    }

    TEST_ASSERT_EQUAL_UINT(10, ds_sb_length(&sb));
    TEST_ASSERT_TRUE(ds_sb_capacity(&sb) >= 10);

    ds_sb_destroy(&sb);
}

void test_stringbuilder_ensure_unique_behavior(void) {
    // Create a string and share it with StringBuilder
    ds_string original = ds("Original content");
    ds_string shared = ds_retain(original);

    // Create StringBuilder that shares the data
    ds_stringbuilder sb;
    sb.data = shared;
    sb.capacity = ds_length(shared) + 1;

    TEST_ASSERT_EQUAL_UINT(2, ds_refcount(sb.data)); // original + sb.data

    // Modifying StringBuilder should trigger copy-on-write
    TEST_ASSERT_TRUE(ds_sb_append(&sb, " + more"));

    TEST_ASSERT_EQUAL_STRING("Original content", original);
    TEST_ASSERT_EQUAL_STRING("Original content + more", ds_sb_cstr(&sb));
    TEST_ASSERT_EQUAL_UINT(1, ds_refcount(original)); // Only original now

    ds_release(&original);
    ds_sb_destroy(&sb);
}

// ============================================================================
// UNICODE BOUNDARY CONDITIONS (third priority)
// ============================================================================

void test_unicode_codepoint_iteration(void) {
    ds_string unicode_str = ds("A🌍B");

    // Check lengths
    TEST_ASSERT_EQUAL_UINT(6, ds_length(unicode_str)); // 1 + 4 + 1 bytes
    TEST_ASSERT_EQUAL_UINT(3, ds_codepoint_length(unicode_str)); // 3 codepoints

    // Test iteration
    ds_codepoint_iter iter = ds_codepoints(unicode_str);
    TEST_ASSERT_EQUAL_UINT32(0x41, ds_iter_next(&iter)); // 'A'
    TEST_ASSERT_EQUAL_UINT32(0x1F30D, ds_iter_next(&iter)); // 🌍
    TEST_ASSERT_EQUAL_UINT32(0x42, ds_iter_next(&iter)); // 'B'
    TEST_ASSERT_EQUAL_UINT32(0, ds_iter_next(&iter)); // End

    ds_release(&unicode_str);
}

void test_unicode_codepoint_access(void) {
    ds_string unicode_str = ds("🚀🌍🎉");

    TEST_ASSERT_EQUAL_UINT32(0x1F680, ds_codepoint_at(unicode_str, 0)); // 🚀
    TEST_ASSERT_EQUAL_UINT32(0x1F30D, ds_codepoint_at(unicode_str, 1)); // 🌍
    TEST_ASSERT_EQUAL_UINT32(0x1F389, ds_codepoint_at(unicode_str, 2)); // 🎉
    TEST_ASSERT_EQUAL_UINT32(0, ds_codepoint_at(unicode_str, 3)); // Out of bounds

    ds_release(&unicode_str);
}

void test_unicode_append_char(void) {
    ds_string str = ds("Hello ");
    ds_string result = ds_append_char(str, 0x1F30D); // 🌍

    TEST_ASSERT_EQUAL_STRING("Hello 🌍", result);
    TEST_ASSERT_EQUAL_UINT(10, ds_length(result)); // 6 + 4 bytes
    TEST_ASSERT_EQUAL_UINT(7, ds_codepoint_length(result)); // 7 codepoints

    ds_release(&str);
    ds_release(&result);
}

void test_unicode_invalid_codepoint(void) {
    ds_string str = ds("Test");
    ds_string result = ds_append_char(str, 0x110000); // Invalid codepoint

    // Should append replacement character (U+FFFD = EF BF BD in UTF-8)
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(ds_length(result) > ds_length(str));

    ds_release(&str);
    ds_release(&result);
}

void test_unicode_empty_iteration(void) {
    ds_string empty_str = ds("");

    TEST_ASSERT_EQUAL_UINT(0, ds_codepoint_length(empty_str));

    ds_codepoint_iter iter = ds_codepoints(empty_str);
    TEST_ASSERT_EQUAL_UINT32(0, ds_iter_next(&iter));
    TEST_ASSERT_FALSE(ds_iter_has_next(&iter));

    ds_release(&empty_str);
}

// ============================================================================
// NULL/EMPTY INPUT HANDLING (fourth priority)
// ============================================================================

void test_null_input_safety(void) {
    // ds() with NULL
    ds_string null_result = ds(NULL);
    TEST_ASSERT_NULL(null_result);

    // Operations with NULL strings
    ds_string str = ds("Test");
    ds_string append_null = ds_append(str, NULL);
    TEST_ASSERT_TRUE(append_null == str); // Should return retained original
    ds_release(&str);
    ds_release(&append_null);

    // Append to NULL
    ds_string append_to_null = ds_append(NULL, "Test");
    TEST_ASSERT_EQUAL_STRING("Test", append_to_null);
    ds_release(&append_to_null);

    // Other operations with NULL
    TEST_ASSERT_EQUAL_UINT(0, ds_length(NULL));
    TEST_ASSERT_EQUAL_UINT(0, ds_refcount(NULL));
    TEST_ASSERT_TRUE(ds_is_empty(NULL));
    TEST_ASSERT_FALSE(ds_is_shared(NULL));
    TEST_ASSERT_EQUAL_INT(0, ds_compare(NULL, NULL));
}

void test_empty_string_operations(void) {
    ds_string empty = ds("");

    TEST_ASSERT_NOT_NULL(empty);
    TEST_ASSERT_EQUAL_UINT(0, ds_length(empty));
    TEST_ASSERT_TRUE(ds_is_empty(empty));
    TEST_ASSERT_EQUAL_STRING("", empty);

    // Operations on empty strings
    ds_string append_to_empty = ds_append(empty, "Hello");
    TEST_ASSERT_EQUAL_STRING("Hello", append_to_empty);

    ds_string prepend_to_empty = ds_prepend(empty, "Hello");
    TEST_ASSERT_EQUAL_STRING("Hello", prepend_to_empty);

    ds_string substr = ds_substring(empty, 0, 5);
    TEST_ASSERT_TRUE(ds_is_empty(substr));

    ds_release(&empty);
    ds_release(&append_to_empty);
    ds_release(&prepend_to_empty);
    ds_release(&substr);
}

void test_boundary_conditions(void) {
    ds_string str = ds("Hello");

    // Substring at boundaries
    ds_string substr_start = ds_substring(str, 0, 2);
    TEST_ASSERT_EQUAL_STRING("He", substr_start);

    ds_string substr_end = ds_substring(str, 3, 2);
    TEST_ASSERT_EQUAL_STRING("lo", substr_end);

    ds_string substr_beyond = ds_substring(str, 10, 5);
    TEST_ASSERT_TRUE(ds_is_empty(substr_beyond));

    // Insert at boundaries
    ds_string insert_start = ds_insert(str, 0, ">> ");
    TEST_ASSERT_EQUAL_STRING(">> Hello", insert_start);

    ds_string insert_end = ds_insert(str, ds_length(str), " <<");
    TEST_ASSERT_EQUAL_STRING("Hello <<", insert_end);

    ds_string insert_beyond = ds_insert(str, 100, "Bad");
    TEST_ASSERT_TRUE(insert_beyond == str); // Should return retained original

    ds_release(&str);
    ds_release(&substr_start);
    ds_release(&substr_end);
    ds_release(&substr_beyond);
    ds_release(&insert_start);
    ds_release(&insert_end);
    ds_release(&insert_beyond);
}

// ============================================================================
// COMPLEX INTEGRATION TESTS
// ============================================================================

void test_functional_chaining(void) {
    ds_string result = ds_prepend(ds_append(ds_append(ds("Hello"), " beautiful"), " world"), ">> ");

    TEST_ASSERT_EQUAL_STRING(">> Hello beautiful world", result);
    ds_release(&result);
}

void test_string_join(void) {
    ds_string words[] = {ds("The"), ds("quick"), ds("brown"), ds("fox")};

    ds_string sentence = ds_join(words, 4, " ");
    TEST_ASSERT_EQUAL_STRING("The quick brown fox", sentence);

    // Test with NULL separator
    ds_string no_sep = ds_join(words, 4, NULL);
    TEST_ASSERT_EQUAL_STRING("Thequickbrownfox", no_sep);

    // Test with empty array
    ds_string empty_join = ds_join(NULL, 0, " ");
    TEST_ASSERT_TRUE(ds_is_empty(empty_join));

    for (int i = 0; i < 4; i++) {
        ds_release(&words[i]);
    }
    ds_release(&sentence);
    ds_release(&no_sep);
    ds_release(&empty_join);
}

// ============================================================================
// MAIN TEST FUNCTION
// ============================================================================

void test(void) {
    UNITY_BEGIN();

    // Basic functionality (simple functions)
    RUN_TEST(test_basic_string_creation);
    RUN_TEST(test_basic_append);
    RUN_TEST(test_basic_compare);

    // Reference counting edge cases (highest priority)
    RUN_TEST(test_multiple_retains_releases);
    RUN_TEST(test_shared_string_immutability);
    RUN_TEST(test_release_null_safety);
    RUN_TEST(test_retain_null_safety);

    // StringBuilder state transitions (second priority)
    RUN_TEST(test_stringbuilder_basic_usage);
    RUN_TEST(test_stringbuilder_to_string_consumption);
    RUN_TEST(test_stringbuilder_capacity_growth);
    RUN_TEST(test_stringbuilder_ensure_unique_behavior);

    // Unicode boundary conditions (third priority)
    RUN_TEST(test_unicode_codepoint_iteration);
    RUN_TEST(test_unicode_codepoint_access);
    RUN_TEST(test_unicode_append_char);
    RUN_TEST(test_unicode_invalid_codepoint);
    RUN_TEST(test_unicode_empty_iteration);

    // NULL/empty input handling (fourth priority)
    RUN_TEST(test_null_input_safety);
    RUN_TEST(test_empty_string_operations);
    RUN_TEST(test_boundary_conditions);

    // Complex integration tests
    RUN_TEST(test_functional_chaining);
    RUN_TEST(test_string_join);

    UNITY_END();
}

int main(void) {
    test();
    return 0;
}
