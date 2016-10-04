/* water.cpp */

#ifndef _ArduinoH_
  #include <Arduino.h>
#endif

#ifndef _WaterH_
  #include "water.h"
#endif

statusFlag dosingFlag = WATER_IDLE;
void interuptPulse();

/******* Flowmeter Member Functions *******/

Flowmeter::Flowmeter(byte p) // Constructor
{
    pin = p;
    pinMode(pin, INPUT);
}

void Flowmeter::setCalibrationFactor(int cf)
{
  calibrationFactor = cf;
}

long Flowmeter::getPulseCount()
{
  return pulseCount;
}

void Flowmeter::resetFlowMeter()
{
  attachInterrupt(1, interuptPulse, RISING);
  pulseCount = 0;
  dosingFlag = WATER_IDLE;
}

float Flowmeter::getVolume()
{
  float vol = (float)pulseCount / (float)calibrationFactor;
  return vol;
}

void Flowmeter::pulse(void)
{
  pulseCount++;
}

byte Flowmeter::getPin()
{
  return pin;
}

/******* Pump Member Functions *******/

Pump::Pump(byte p)
{
  pin = p;
  pinMode(pin, OUTPUT);
}

byte Pump::start(void)
{
  if (digitalRead(pin)) return 0;
  digitalWrite(pin, HIGH);
  return 1;
}

void Pump::stop(void)
{
 digitalWrite(pin, LOW);
}

/******* Magnetvalves Member Functions *******/

Magnetvalves::Magnetvalves(byte p, const char *name)
{
  pin = p;
  /* Set pin configuration for valves */
  pinMode(pin, OUTPUT);
  strncpy(plant, name, 8);
}

void Magnetvalves::setVolumeTarget(int v)
{
  targetV = v;
}

void Magnetvalves::incVolumeTarget(int i)
{
  targetV += i;
  if (targetV == 100) targetV = 0;
  if (targetV == 255) targetV = 99;
}

byte Magnetvalves::readVolumeTarget()
{
  return targetV;
}

char * Magnetvalves::getPlant()
{
  return plant;
}

byte Magnetvalves::dosing(void)
{
  if (dosingFlag == WATER_IDLE)
  {
    digitalWrite(this->pin, HIGH);
    dosingFlag = WATER_BSY;
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (currV < (this->targetV * this->m_giessFactor)))
  {
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (currV > (this->targetV * this->m_giessFactor)))
  {
    digitalWrite(this->pin, LOW);
    dosingFlag = WATER_IDLE;
    return 0;
  }

  return -1;
}

byte Magnetvalves::dosing(float vol)
{
  if (dosingFlag == WATER_IDLE)
  {
    digitalWrite(this->pin, HIGH);
    dosingFlag = WATER_BSY;
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (currV < vol))
  {
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (currV > vol))
  {
    digitalWrite(this->pin, LOW);
    dosingFlag = WATER_IDLE;
    return 0;
  }

  return -1;
}

void Magnetvalves::setCurrentVolume(float vol)
{
  currV = vol;
}

float Magnetvalves::readCurrentVolume(void)
{
  return currV;
}

void Magnetvalves::setGiessFactor(float giessFactor)
{
  m_giessFactor =  giessFactor;
}

//----------------------------------------------------------------
// Thermocontrol class implementation

Thermocontrol::Thermocontrol(int lowerAveragingHour)
{
  m_numberOfTempReadings = 0;
  m_tempAddition = 0;
  m_tempCoeff = 1;
  m_lowerAveragingHour = lowerAveragingHour;
};

void Thermocontrol::AddTempReading(float tempReading, int upperAveragingHour)
{
  if (hour() > m_lowerAveragingHour - 1 && hour() < m_upperAveragingHour + 1)
  {
    m_tempAddition += tempReading;
    m_numberOfTempReadings++;
  }
}

float Thermocontrol::GetTempAverage()
{
  if (m_numberOfTempReadings)
  {
    return (m_tempAddition / float(m_numberOfTempReadings));
  }
  else
  return -1;
};

void Thermocontrol::ResetAverage()
{
  m_tempAddition = 0;
  m_numberOfTempReadings = 0;
};

float Thermocontrol::GetGiessFactor()
{
  if (m_numberOfTempReadings)
  {
    return
    pow(m_tempCoeff, (((m_tempAddition / m_numberOfTempReadings) / 20) - 1));
  }
  else
  return 1;
}

void Thermocontrol::SetTempCoeff(float tempCoeff)
{
  m_tempCoeff = tempCoeff;
}

void Thermocontrol::IncTempCoeff(float inc)
{
  m_tempCoeff += inc;
}

float Thermocontrol::GetTempCoeff(void)
{
  return m_tempCoeff;
}
