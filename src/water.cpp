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

void Magnetvalves::incVolumeTarget(int i) {
  targetV += i;
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
  }

  if ((dosingFlag == BUSY) && (currV < this->targetV)) {
    return 1;
  }

  if ((dosingFlag == BUSY) && (currV > this->targetV)) {
    digitalWrite(this->pin, LOW);
    dosingFlag = IDLE;
    return 0;
  }

  return -1;
}

int Magnetvalves::dosing(int vol) {
  if (dosingFlag == IDLE) {
    digitalWrite(this->pin, HIGH);
    dosingFlag = BUSY;
    return 1;
  }

  if ((dosingFlag == BUSY) && (currV < vol)) {
    return 1;
  }

  if ((dosingFlag == BUSY) && (currV > vol)) {
    digitalWrite(this->pin, LOW);
    dosingFlag = IDLE;
    return 0;
  }

  return -1;
}

void Magnetvalves::setCurrentVolume(int vol) {
  currV = vol;
}

int Magnetvalves::readCurrentVolume(void) {
  return currV;
}
