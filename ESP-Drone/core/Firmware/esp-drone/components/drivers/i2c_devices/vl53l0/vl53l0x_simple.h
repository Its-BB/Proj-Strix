/**
 * Simple Working VL53L0X Driver Header
 * Based on your working library approach
 * Uses single measurement polling - no continuous mode issues
 */

#ifndef VL53L0X_SIMPLE_H_
#define VL53L0X_SIMPLE_H_

#include <stdbool.h>
#include <stdint.h>

// Public API functions for altitude logger integration
bool vl53l0x_simple_init(void);
bool vl53l0x_simple_get_distance(uint16_t* distance_mm, bool* valid);
bool vl53l0x_simple_is_available(void);

#endif // VL53L0X_SIMPLE_H_
