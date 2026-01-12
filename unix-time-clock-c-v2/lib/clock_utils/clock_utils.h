#ifndef CLOCK_UTILS_H
#define CLOCK_UTILS_H

#include <stdint.h>

// Vertical rank: 0 = top, 15 = bottom
// LEDs on opposite sides of the circle at the same height have the same rank
const int verticalRank[32] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // LEDs 0-15
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0   // LEDs 16-31
};

// Get vertical position (0.0 = top, 1.0 = bottom)
inline float getVerticalPosition(int ledIndex) {
  return verticalRank[ledIndex] / 15.0f;
}

// HSV to RGB conversion for rainbow effect
inline void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;

  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:  *r = v; *g = t; *b = p; break;
    case 1:  *r = q; *g = v; *b = p; break;
    case 2:  *r = p; *g = v; *b = t; break;
    case 3:  *r = p; *g = q; *b = v; break;
    case 4:  *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
  }
}

// Apply color correction for warmer rainbow
inline void applyColorCorrection(uint8_t *r, uint8_t *g, uint8_t *b) {
  *g = (*g * 85) / 100;  // 85% green
  *b = (*b * 40) / 100;  // 40% blue
}

#endif // CLOCK_UTILS_H
