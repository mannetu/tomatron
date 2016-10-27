/* water.cpp */

#ifndef _ArduinoH_
  #include <Arduino.h>
#endif

#ifndef _WaterH_
  #include "water.h"
#endif

statusFlag dosingFlag = WATER_IDLE;
void interuptPulse();

//----------------------------------------------------------------
// Flowmeter Member Functions

Flowmeter::Flowmeter(byte pin) // Constructor
{
    m_pin = pin;
    pinMode(m_pin, INPUT);
}

void Flowmeter::setCalibrationFactor(int calibrationFactor)
{
  m_calibrationFactor = calibrationFactor;
}

long Flowmeter::getPulseCount()
{
  return m_pulseCount;
}

void Flowmeter::resetFlowMeter()
{
  attachInterrupt(1, interuptPulse, RISING);
  m_pulseCount = 0;
  dosingFlag = WATER_IDLE;
}

float Flowmeter::getVolume()
{
  return ((float)m_pulseCount / (float)m_calibrationFactor);
}

void Flowmeter::pulse(void)
{
  m_pulseCount++;
}

int Flowmeter::getPin()
{
  return m_pin;
}

//----------------------------------------------------------------
// Pump Class

Pump::Pump(byte pin)
{
  m_pin = pin;
  pinMode(m_pin, OUTPUT);
}

int Pump::start(void)
{
  if (digitalRead(m_pin))
  {
    return 0;
  }
  digitalWrite(m_pin, HIGH);
  return 1;
}

void Pump::stop(void)
{
 digitalWrite(m_pin, LOW);
}

//----------------------------------------------------------------
// Magnetvalves

Magnetvalves::Magnetvalves(byte pin, const char *plantName)
{
  m_pin = pin;
  pinMode(m_pin, OUTPUT); // Set pin configuration for valves
  strncpy(m_plantName, plantName, 8);
}

void Magnetvalves::setVolumeTarget(int targetV)
{
  m_targetV = targetV;
}

void Magnetvalves::incVolumeTarget(int i)
{
  m_targetV += i;
  if (m_targetV == -1) m_targetV = 99;
  if (m_targetV == 100) m_targetV = 0;
}

  int Magnetvalves::readVolumeTarget()
{
  return m_targetV;
}

char * Magnetvalves::getPlantName()
{
  return m_plantName;
}

int Magnetvalves::dosing(void)
{
  if (dosingFlag == WATER_IDLE)
  {
    digitalWrite(m_pin, HIGH);
    dosingFlag = WATER_BSY;
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (m_currentVolume < (m_targetV * m_giessFactor)))
  {
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (m_currentVolume > (m_targetV * m_giessFactor)))
  {
    digitalWrite(m_pin, LOW);
    dosingFlag = WATER_IDLE;
    return 0;
  }

  return -1;
}

int Magnetvalves::dosing(float vol)
{
  if (dosingFlag == WATER_IDLE)
  {
    digitalWrite(m_pin, HIGH);
    dosingFlag = WATER_BSY;
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (m_currentVolume < vol))
  {
    return 1;
  }

  if ((dosingFlag == WATER_BSY) && (m_currentVolume > vol))
  {
    digitalWrite(m_pin, LOW);
    dosingFlag = WATER_IDLE;
    return 0;
  }

  return -1;
}

void Magnetvalves::setCurrentVolume(float newCurrentVolume)
{
  m_currentVolume = newCurrentVolume;
}

float Magnetvalves::readCurrentVolume(void)
{
  return m_currentVolume;
}

void Magnetvalves::setGiessFactor(float giessFactor)
{
  m_giessFactor =  giessFactor;
}



//----------------------------------------------------------------
// Thermocontroler

Thermocontrol::Thermocontrol(int lowerAveragingHour)
{
  m_numberOfTempReadings = 0;
  m_tempAddition = 0;
  m_tempCoeff = 1;
  m_lowerAveragingHour = lowerAveragingHour;
};

void Thermocontrol::AddTempReading(float tempReading)
{
  m_tempAddition += tempReading;
  m_numberOfTempReadings++;
}

float Thermocontrol::GetTempAverage()
{
  if (m_numberOfTempReadings)
  {
    return (m_tempAddition / float(m_numberOfTempReadings));
  }
  else
  {
  return -1;
  }
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
    return pow(m_tempCoeff,
      (((m_tempAddition / m_numberOfTempReadings) / 20) - 1));
  }
  else
  {
  return 1;
  }
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

int Thermocontrol::GetLowerAveragingHour(void)
{
  return m_lowerAveragingHour;
}
