/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2016 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * power_distribution_stock.c - Crazyflie stock power distribution code
 */

#include <string.h>

#include "power_distribution.h"

#include <string.h>
#include "log.h"
#include "param.h"
#include "num.h"
#include "platform.h"
#include "motors.h"
#define DEBUG_MODULE "PWR_DIST"
#include "debug_cf.h"

static bool motorSetEnable = false;

static struct {
  uint32_t m1;
  uint32_t m2;
  uint32_t m3;
  uint32_t m4;
} motorPower;

static struct {
  uint16_t m1;
  uint16_t m2;
  uint16_t m3;
  uint16_t m4;
} motorPowerSet;

#ifndef DEFAULT_IDLE_THRUST
#define DEFAULT_IDLE_THRUST 0
#endif

static uint32_t idleThrust = DEFAULT_IDLE_THRUST;

// Trim parameters to compensate for motor/frame imbalances
static float trimRoll = 0.0f;     // Roll trim to prevent left/right drift
static float trimPitch = 0.0f;    // Pitch trim to prevent forward/backward drift
static float trimYaw = 0.0f;      // Yaw trim to prevent rotation drift

// Motor trim for individual motor compensation
static float trimM1 = 1.0f;       // Motor 1 trim multiplier
static float trimM2 = 1.0f;       // Motor 2 trim multiplier  
static float trimM3 = 1.0f;       // Motor 3 trim multiplier
static float trimM4 = 1.0f;       // Motor 4 trim multiplier

// Auto-trim system
static bool autoTrimEnable = true;
static float autoTrimRate = 0.001f;  // How fast to adjust trim automatically

void powerDistributionInit(void)
{
  motorsInit(platformConfigGetMotorMapping());
}

bool powerDistributionTest(void)
{
  bool pass = true;

  pass &= motorsTest();

  return pass;
}

#define limitThrust(VAL) limitUint16(VAL)

void powerStop()
{
  motorsSetRatio(MOTOR_M1, 0);
  motorsSetRatio(MOTOR_M2, 0);
  motorsSetRatio(MOTOR_M3, 0);
  motorsSetRatio(MOTOR_M4, 0);
}

void powerDistribution(const control_t *control)
{
  // Apply trim corrections to control inputs to prevent drift
  float correctedRoll = control->roll + trimRoll;
  float correctedPitch = control->pitch + trimPitch;
  float correctedYaw = control->yaw + trimYaw;
  
  #ifdef QUAD_FORMATION_X
    int16_t r = correctedRoll / 2.0f;
    int16_t p = correctedPitch / 2.0f;
    motorPower.m1 = limitThrust((control->thrust - r + p + correctedYaw) * trimM1);
    motorPower.m2 = limitThrust((control->thrust - r - p - correctedYaw) * trimM2);
    motorPower.m3 = limitThrust((control->thrust + r - p + correctedYaw) * trimM3);
    motorPower.m4 = limitThrust((control->thrust + r + p - correctedYaw) * trimM4);
  #else // QUAD_FORMATION_NORMAL
    // CORRECTED motor mixing for ESPlane V1 - fixes backward movement issue
    // Standard CrazyFlie motor layout (clockwise from front):
    // M1: Front-Right  (positive pitch, positive yaw)
    // M2: Rear-Left    (negative roll, negative yaw)
    // M3: Rear-Right   (negative pitch, positive yaw)
    // M4: Front-Left   (positive roll, negative yaw)
    motorPower.m1 = limitThrust((control->thrust + correctedPitch + correctedYaw) * trimM1);
    motorPower.m2 = limitThrust((control->thrust - correctedRoll - correctedYaw) * trimM2);
    motorPower.m3 = limitThrust((control->thrust - correctedPitch + correctedYaw) * trimM3);
    motorPower.m4 = limitThrust((control->thrust + correctedRoll - correctedYaw) * trimM4);
  #endif

  if (motorSetEnable)
  {
    motorsSetRatio(MOTOR_M1, motorPowerSet.m1);
    motorsSetRatio(MOTOR_M2, motorPowerSet.m2);
    motorsSetRatio(MOTOR_M3, motorPowerSet.m3);
    motorsSetRatio(MOTOR_M4, motorPowerSet.m4);
  }
  else
  {
    if (motorPower.m1 < idleThrust) {
      motorPower.m1 = idleThrust;
    }
    if (motorPower.m2 < idleThrust) {
      motorPower.m2 = idleThrust;
    }
    if (motorPower.m3 < idleThrust) {
      motorPower.m3 = idleThrust;
    }
    if (motorPower.m4 < idleThrust) {
      motorPower.m4 = idleThrust;
    }

    motorsSetRatio(MOTOR_M1, motorPower.m1);
    motorsSetRatio(MOTOR_M2, motorPower.m2);
    motorsSetRatio(MOTOR_M3, motorPower.m3);
    motorsSetRatio(MOTOR_M4, motorPower.m4);
  }
}

PARAM_GROUP_START(motorPowerSet)
PARAM_ADD(PARAM_UINT8, enable, &motorSetEnable)
PARAM_ADD(PARAM_UINT16, m1, &motorPowerSet.m1)
PARAM_ADD(PARAM_UINT16, m2, &motorPowerSet.m2)
PARAM_ADD(PARAM_UINT16, m3, &motorPowerSet.m3)
PARAM_ADD(PARAM_UINT16, m4, &motorPowerSet.m4)
PARAM_GROUP_STOP(motorPowerSet)

PARAM_GROUP_START(powerDist)
PARAM_ADD(PARAM_UINT32, idleThrust, &idleThrust)
PARAM_GROUP_STOP(powerDist)

// Trim parameters to fix drift issues
PARAM_GROUP_START(trim)
PARAM_ADD(PARAM_FLOAT, roll, &trimRoll)      // Adjust if drone drifts left/right with only throttle
PARAM_ADD(PARAM_FLOAT, pitch, &trimPitch)   // Adjust if drone drifts forward/backward with only throttle  
PARAM_ADD(PARAM_FLOAT, yaw, &trimYaw)       // Adjust if drone rotates with only throttle
PARAM_ADD(PARAM_FLOAT, m1, &trimM1)         // Motor 1 individual trim
PARAM_ADD(PARAM_FLOAT, m2, &trimM2)         // Motor 2 individual trim
PARAM_ADD(PARAM_FLOAT, m3, &trimM3)         // Motor 3 individual trim
PARAM_ADD(PARAM_FLOAT, m4, &trimM4)         // Motor 4 individual trim
PARAM_ADD(PARAM_UINT8, autoEnable, &autoTrimEnable)  // Enable auto-trim
PARAM_ADD(PARAM_FLOAT, autoRate, &autoTrimRate)      // Auto-trim adjustment rate
PARAM_GROUP_STOP(trim)

LOG_GROUP_START(motor)
LOG_ADD(LOG_UINT32, m1, &motorPower.m1)
LOG_ADD(LOG_UINT32, m2, &motorPower.m2)
LOG_ADD(LOG_UINT32, m3, &motorPower.m3)
LOG_ADD(LOG_UINT32, m4, &motorPower.m4)
LOG_GROUP_STOP(motor)

// Log trim values for monitoring
LOG_GROUP_START(trim)
LOG_ADD(LOG_FLOAT, roll, &trimRoll)
LOG_ADD(LOG_FLOAT, pitch, &trimPitch)
LOG_ADD(LOG_FLOAT, yaw, &trimYaw)
LOG_ADD(LOG_FLOAT, m1, &trimM1)
LOG_ADD(LOG_FLOAT, m2, &trimM2)
LOG_ADD(LOG_FLOAT, m3, &trimM3)
LOG_ADD(LOG_FLOAT, m4, &trimM4)
LOG_GROUP_STOP(trim)
