#include <unity.h>
#include <clock_utils.h>

void setUp(void) {
    // Run before each test
}

void tearDown(void) {
    // Run after each test
}

// Test vertical position calculations
void test_vertical_position_top(void) {
    // LEDs 0 and 31 are at the top (position 0)
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, getVerticalPosition(0));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, getVerticalPosition(31));
}

void test_vertical_position_bottom(void) {
    // LEDs 15 and 16 are at the bottom (position 1)
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, getVerticalPosition(15));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 1.0, getVerticalPosition(16));
}

void test_vertical_position_middle(void) {
    // LEDs 7/8 and 23/24 are at middle positions
    float pos7 = getVerticalPosition(7);
    float pos24 = getVerticalPosition(24);
    TEST_ASSERT_FLOAT_WITHIN(0.01, pos7, pos24);  // Same height
    TEST_ASSERT_TRUE(pos7 > 0.0 && pos7 < 1.0);   // In between
}

void test_vertical_position_symmetry(void) {
    // Opposite LEDs should have same vertical position
    for (int i = 0; i < 16; i++) {
        float pos1 = getVerticalPosition(i);
        float pos2 = getVerticalPosition(31 - i);
        TEST_ASSERT_FLOAT_WITHIN(0.01, pos1, pos2);
    }
}

// Test HSV to RGB conversion
void test_hsv_red(void) {
    uint8_t r, g, b;
    hsvToRgb(0, 255, 255, &r, &g, &b);  // Hue 0 = Red
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(0, b);
}

void test_hsv_green(void) {
    uint8_t r, g, b;
    hsvToRgb(85, 255, 255, &r, &g, &b);  // Hue ~85 = Green
    TEST_ASSERT_EQUAL_UINT8(255, g);
}

void test_hsv_blue(void) {
    uint8_t r, g, b;
    hsvToRgb(170, 255, 255, &r, &g, &b);  // Hue ~170 = Blue
    TEST_ASSERT_EQUAL_UINT8(255, b);
}

void test_hsv_brightness(void) {
    uint8_t r1, g1, b1, r2, g2, b2;
    hsvToRgb(0, 255, 255, &r1, &g1, &b1);  // Full brightness
    hsvToRgb(0, 255, 128, &r2, &g2, &b2);  // Half brightness
    TEST_ASSERT_TRUE(r1 > r2);  // Full brightness should be brighter
}

// Test color correction
void test_color_correction_reduces_blue(void) {
    uint8_t r = 100, g = 100, b = 100;
    applyColorCorrection(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(100, r);  // Red unchanged
    TEST_ASSERT_EQUAL_UINT8(85, g);   // Green reduced to 85%
    TEST_ASSERT_EQUAL_UINT8(40, b);   // Blue reduced to 40%
}

void test_color_correction_preserves_black(void) {
    uint8_t r = 0, g = 0, b = 0;
    applyColorCorrection(&r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Vertical position tests
    RUN_TEST(test_vertical_position_top);
    RUN_TEST(test_vertical_position_bottom);
    RUN_TEST(test_vertical_position_middle);
    RUN_TEST(test_vertical_position_symmetry);

    // HSV to RGB tests
    RUN_TEST(test_hsv_red);
    RUN_TEST(test_hsv_green);
    RUN_TEST(test_hsv_blue);
    RUN_TEST(test_hsv_brightness);

    // Color correction tests
    RUN_TEST(test_color_correction_reduces_blue);
    RUN_TEST(test_color_correction_preserves_black);

    return UNITY_END();
}
