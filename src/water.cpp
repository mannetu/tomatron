/* water.cpp */

#include <Arduino.h>
#include "water.h"
#include <EEPROM.h>

int dosingFlag = IDLE;

/******* Flowmeter Member Functions *******/

void Flowmeter::setCalibrationFactor(int cf) {
  calibrationFactor = cf;
  EEPROM.put(0, calibrationFactor);
}

int Flowmeter::getPulseCount() {
  return pulseCount;
}

void Flowmeter::resetFlowMeter() {
  pulseCount = 0;
  dosingFlag = IDLE;
}

int Flowmeter::getVolume() {
  return (pulseCount / calibrationFactor);
}

void Flowmeter::pulse() {
  pulseCount++;
}


/******* Magnetvalves Member Functions *******/

void Magnetvalves::setVolumeTarget(int v) {
  targetV = v;
}

int Magnetvalves::readVolumeTarget() {
  return targetV;
}

char * Magnetvalves::getPlant() {
  return plant;
}

int Magnetvalves::getPin() {
  return pin;
}

int Magnetvalves::dosing(void) {
  if (dosingFlag == IDLE) {
    digitalWrite(this->pin, HIGH);
    dosingFlag = BUSY;
    return 1;
  } else {
    return 2;
  }

  if (currV < this->targetV) {
    return 0;
  } else
  {
    digitalWrite(this->pin, LOW);
    dosingFlag = IDLE;
    return 1;
  }
}

int Magnetvalves::dosing(int vol) {
  if (dosingFlag == IDLE) {
    digitalWrite(this->pin, HIGH);
    dosingFlag = BUSY;
    return 0;
  } else {
    return 2;
  }

  if (currV < vol) {
    return 0;
  } else
  {
    digitalWrite(this->pin, LOW);
    dosingFlag = IDLE;
    return 1;
  }
}

void Magnetvalves::setCurrentVolume(int vol) {
  currV = vol;
}

int Magnetvalves::readCurrentVolume(void) {
  return currV;
}
