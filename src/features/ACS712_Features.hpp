// file: ACS712_Features.h
#pragma once

#include <ACS712.h>
#include <HardwareSerial.h>

/**********************************************************************************************************************************/
// Define your sensor settings and variables here
#define ACS712_PIN 32
#define RELAY 23

/**********************************************************************************************************************************/
// Define your functions here
void setCurrentSensor();
int getCurrent();
