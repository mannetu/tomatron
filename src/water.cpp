/* water.cpp */

#ifndef _ArduinoH_
  #include <Arduino.h>
#endif

#ifndef _WaterH_
  #include "Water.h"
#endif

statusFlag dosingFlag = IDLE;

/******* Flowmeter Member Functions *******/

void Flowmeter::setCalibrationFactor(int cf) {
  calibrationFactor = cf;
}

long Flowmeter::getPulseCount() {
  return pulseCount;
}

void Flowmeter::resetFlowMeter() {
  pulseCount = 0;
  dosingFlag = IDLE;
}

byte Flowmeter::getVolume() {
  return (pulseCount / calibrationFactor);
}

void Flowmeter::pulse(void) {
  pulseCount++;
}


void Flowmeter::setPin(byte p) {
    pin = p;
}

byte Flowmeter::getPin() {
  return pin;
}

/******* Pump Member Functions *******/

void Pump::setPin(byte p) {
 pin = p;
}

byte Pump::getPin(void) {
 return pin;
}

void Pump::start(void) {
 digitalWrite(pin, HIGH);
}

void Pump::stop(void) {
 digitalWrite(pin, LOW);
}

/******* Magnetvalves Member Functions *******/

void Magnetvalves::setVolumeTarget(int v) {
  targetV = v;
}

void Magnetvalves::incVolumeTarget(int i) {
  targetV += i;
}

byte Magnetvalves::readVolumeTarget() {
  return targetV;
}

char * Magnetvalves::getPlant() {
  return plant;
}

void Magnetvalves::setPin(byte p) {
    pin = p;
  }

byte Magnetvalves::getPin() {
  return pin;
}

byte Magnetvalves::dosing(void) {
  if (dosingFlag == IDLE) {
    digitalWrite(this->pin, HIGH);
    dosingFlag = BSY;
    return 1;
  }

  if ((dosingFlag == BSY) && (currV < this->targetV-1)) {
    return 1;
  }

  if ((dosingFlag == BSY) && (currV > this->targetV-1)) {
    digitalWrite(this->pin, LOW);
    dosingFlag = IDLE;
    return 0;
  }

  return -1;
}

byte Magnetvalves::dosing(byte vol) {
  if (dosingFlag == IDLE) {
    digitalWrite(this->pin, HIGH);
    dosingFlag = BSY;
    return 1;
  }

  if ((dosingFlag == BSY) && (currV < vol-1)) {
    return 1;
  }

  if ((dosingFlag == BSY) && (currV > vol-1)) {
    digitalWrite(this->pin, LOW);
    dosingFlag = IDLE;
    return 0;
  }

  return -1;
}

void Magnetvalves::setCurrentVolume(int vol) {
  currV = vol;
}

byte Magnetvalves::readCurrentVolume(void) {
  return currV;
}
