#include <unity.h>
#include "helpers.h"

void test_get_int_from_char_positive() {
    char str[] = "123|";
    char *p = str;
    TEST_ASSERT_EQUAL(123, get_int_from_char(&p, '|'));
    TEST_ASSERT_EQUAL('\0', *p);
}

void test_get_int_from_char_negative() {
    char str[] = "-45,";
    char *p = str;
    TEST_ASSERT_EQUAL(-45, get_int_from_char(&p, ','));
    TEST_ASSERT_EQUAL('\0', *p);
}

void test_parse_hex_color_valid() {
    TEST_ASSERT_EQUAL_UINT32(0xFFA500, parse_hex_color("#FFA500"));
}

void test_parse_hex_color_invalid() {
    TEST_ASSERT_EQUAL_UINT32(0, parse_hex_color("bad"));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_get_int_from_char_positive);
    RUN_TEST(test_get_int_from_char_negative);
    RUN_TEST(test_parse_hex_color_valid);
    RUN_TEST(test_parse_hex_color_invalid);
    return UNITY_END();
}
