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

#include "motor_diagnostic.h"
#include "motors.h"
#include "power_distribution.h"
#include "log.h"
#include "param.h"
#include "debug_cf.h"
#undef DEBUG_MODULE
#define DEBUG_MODULE "MOTOR_DIAG"

// Test parameters
static uint16_t testMotorId = 0;        // Which motor to test (0-3)
static uint16_t testThrust = 20000;     // Test thrust level
static uint32_t testDuration = 1000;   // Test duration in ms
static bool motorTestEnabled = false;  // Enable motor testing
static bool runMotorSequence = false;  // Run sequential motor test

// Motor identification results
static int16_t motorPositions[4] = {-1, -1, -1, -1}; // Physical position mapping
static float motorBalance[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // Balance factors

// Test state
static uint32_t testStartTime = 0;
static uint32_t sequenceStartTime = 0;
static int currentSequenceMotor = 0;
static bool testActive = false;

void motorDiagnosticInit(void)
{
    DEBUG_PRINT("Motor diagnostic system initialized\n");
}

void motorDiagnosticUpdate(uint32_t tick)
{
    uint32_t currentTime = tick; // Assuming tick is in milliseconds
    
    // Single motor test
    if (motorTestEnabled && !testActive) {
        DEBUG_PRINT("Starting motor %d test at thrust %d for %d ms\n", 
                   testMotorId, testThrust, testDuration);
        
        // Stop all motors first
        powerStop();
        
        // Start the selected motor
        motorsSetRatio(testMotorId, testThrust);
        
        testStartTime = currentTime;
        testActive = true;
    }
    
    // Stop single motor test after duration
    if (testActive && (currentTime - testStartTime) >= testDuration) {
        motorsSetRatio(testMotorId, 0);
        testActive = false;
        motorTestEnabled = false;
        DEBUG_PRINT("Motor %d test completed\n", testMotorId);
    }
    
    // Sequential motor test
    if (runMotorSequence) {
        if (currentSequenceMotor == 0 && sequenceStartTime == 0) {
            sequenceStartTime = currentTime;
            DEBUG_PRINT("Starting sequential motor test...\n");
        }
        
        uint32_t sequenceTime = currentTime - sequenceStartTime;
        uint32_t motorPhase = sequenceTime / (testDuration + 500); // Test + pause
        uint32_t phaseTime = sequenceTime % (testDuration + 500);
        
        if (motorPhase < 4) {
            currentSequenceMotor = motorPhase;
            
            if (phaseTime == 0) {
                // Start this motor
                powerStop();
                motorsSetRatio(currentSequenceMotor, testThrust);
                DEBUG_PRINT("Testing motor M%d (sequence %d/4)\n", 
                           currentSequenceMotor + 1, motorPhase + 1);
            } else if (phaseTime >= testDuration) {
                // Stop this motor (pause phase)
                motorsSetRatio(currentSequenceMotor, 0);
            }
        } else {
            // Sequence complete
            powerStop();
            runMotorSequence = false;
            sequenceStartTime = 0;
            currentSequenceMotor = 0;
            DEBUG_PRINT("Sequential motor test completed\n");
        }
    }
}

void motorDiagnosticSetPosition(uint8_t motorId, int8_t position)
{
    if (motorId < 4) {
        motorPositions[motorId] = position;
        DEBUG_PRINT("Motor M%d mapped to position %d\n", motorId + 1, position);
    }
}

void motorDiagnosticSetBalance(uint8_t motorId, float balance)
{
    if (motorId < 4) {
        motorBalance[motorId] = balance;
        DEBUG_PRINT("Motor M%d balance set to %.3f\n", motorId + 1, balance);
    }
}

void motorDiagnosticTestAll(void)
{
    runMotorSequence = true;
    sequenceStartTime = 0;
    currentSequenceMotor = 0;
}

void motorDiagnosticStopTest(void)
{
    motorTestEnabled = false;
    runMotorSequence = false;
    testActive = false;
    powerStop();
    DEBUG_PRINT("All motor tests stopped\n");
}

// Get motor balance factors for power distribution
float motorDiagnosticGetBalance(uint8_t motorId)
{
    if (motorId < 4) {
        return motorBalance[motorId];
    }
    return 1.0f;
}

// Parameter interface for cfclient control
PARAM_GROUP_START(motorDiag)
PARAM_ADD(PARAM_UINT16, testMotor, &testMotorId)        // Motor to test (0-3)
PARAM_ADD(PARAM_UINT16, testThrust, &testThrust)       // Thrust level for test
PARAM_ADD(PARAM_UINT32, testDuration, &testDuration)   // Test duration in ms
PARAM_ADD(PARAM_UINT8, startTest, &motorTestEnabled)   // Start single motor test
PARAM_ADD(PARAM_UINT8, startSequence, &runMotorSequence) // Start sequential test

PARAM_ADD(PARAM_FLOAT, balanceM1, &motorBalance[0])     // Motor balance factors
PARAM_ADD(PARAM_FLOAT, balanceM2, &motorBalance[1])
PARAM_ADD(PARAM_FLOAT, balanceM3, &motorBalance[2])
PARAM_ADD(PARAM_FLOAT, balanceM4, &motorBalance[3])
PARAM_GROUP_STOP(motorDiag)

// Logging for motor monitoring
LOG_GROUP_START(motorDiag)
LOG_ADD(LOG_UINT8, testActive, &testActive)
LOG_ADD(LOG_UINT8, currentMotor, &currentSequenceMotor)
LOG_ADD(LOG_INT16, posM1, &motorPositions[0])
LOG_ADD(LOG_INT16, posM2, &motorPositions[1])
LOG_ADD(LOG_INT16, posM3, &motorPositions[2])
LOG_ADD(LOG_INT16, posM4, &motorPositions[3])
LOG_ADD(LOG_FLOAT, balM1, &motorBalance[0])
LOG_ADD(LOG_FLOAT, balM2, &motorBalance[1])
LOG_ADD(LOG_FLOAT, balM3, &motorBalance[2])
LOG_ADD(LOG_FLOAT, balM4, &motorBalance[3])
LOG_GROUP_STOP(motorDiag)
