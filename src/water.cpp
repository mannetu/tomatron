/* water.cpp */

#ifndef _ArduinoH_
  #include <Arduino.h>
#endif

#ifndef _WaterH_
  #include "Water.h"
#endif

statusFlag dosingFlag = IDLE;
void interuptPulse();

/******* Flowmeter Member Functions *******/

Flowmeter::Flowmeter(byte p) { // Constructor
    pin = p;
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(1, interuptPulse, FALLING);
}

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

byte Flowmeter::getPin() {
  return pin;
}

/******* Pump Member Functions *******/

Pump::Pump(byte p) {
  pin = p;
  pinMode(pin, OUTPUT);
}

void Pump::start(void) {
 digitalWrite(pin, HIGH);
}

void Pump::stop(void) {
 digitalWrite(pin, LOW);
}

/******* Magnetvalves Member Functions *******/

Magnetvalves::Magnetvalves(byte p, const char *name) {

  pin = p;
  /* Set pin configuration for valves */
  pinMode(pin, OUTPUT);
  strncpy(plant, name, 8);

}

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
