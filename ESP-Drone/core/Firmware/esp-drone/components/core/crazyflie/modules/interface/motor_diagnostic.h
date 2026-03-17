/**
 * Motor Diagnostic Tool for ESP-Drone
 * 
 * This module provides functions to test and identify individual motors
 * to help diagnose motor mapping and direction issues.
 * 
 * Copyright 2019-2020 Espressif Systems (Shanghai)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 */

#ifndef __MOTOR_DIAGNOSTIC_H__
#define __MOTOR_DIAGNOSTIC_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize the motor diagnostic system
 */
void motorDiagnosticInit(void);

/**
 * Update motor diagnostic system - call this regularly from main loop
 * @param tick Current system tick in milliseconds
 */
void motorDiagnosticUpdate(uint32_t tick);

/**
 * Set the physical position mapping for a motor
 * @param motorId Motor ID (0-3)
 * @param position Physical position (-1=unknown, 0=front-right, 1=rear-right, 2=rear-left, 3=front-left)
 */
void motorDiagnosticSetPosition(uint8_t motorId, int8_t position);

/**
 * Set the balance/trim factor for a motor
 * @param motorId Motor ID (0-3)
 * @param balance Balance factor (1.0=normal, <1.0=reduce power, >1.0=increase power)
 */
void motorDiagnosticSetBalance(uint8_t motorId, float balance);

/**
 * Run a test on all motors in sequence
 */
void motorDiagnosticTestAll(void);

/**
 * Stop all motor tests immediately
 */
void motorDiagnosticStopTest(void);

/**
 * Get the motor balance factor for power distribution
 * @param motorId Motor ID (0-3)
 * @return Balance factor
 */
float motorDiagnosticGetBalance(uint8_t motorId);

#endif /* __MOTOR_DIAGNOSTIC_H__ */
