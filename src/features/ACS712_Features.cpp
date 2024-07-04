// file: ACS712_Features.cpp
#include "ACS712_Features.hpp"


ACS712  acs(ACS712_PIN, 3.3, 4095, 100);
unsigned int current_val;

/*****************************************************Get Current Sensor Data*****************************************************/
// get current Sensor Data
int getCurrent() {
  digitalWrite(RELAY, HIGH);
  delay(400);
  current_val = acs.mA_AC_sampling();
  delay(400);
  //current = current_val;
  Serial.print("mA: ");
  Serial.println(current_val);
  delay(400);
  return current_val;
}

/**********************************************************************************************************************************/
// Define your sensor settings and variables here
void setCurrentSensor()
{
  pinMode(ACS712_PIN, INPUT);
  pinMode(RELAY, OUTPUT);
  acs.autoMidPoint();
  Serial.print("MidPoint: ");
  Serial.print(acs.getMidPoint());
  Serial.print(". Noise mV: ");
  Serial.println(acs.getNoisemV());
  getCurrent();
}



