/* water.h */

#include <Arduino.h>
#include <DS3232RTC.h>

enum statusFlag {WATER_IDLE, WATER_BSY};

class Flowmeter {
  private:
    int   m_pin;
    int   m_calibrationFactor;  // Calibration factor for flow-meter = (Pulses / Liter)
    long  m_pulseCount;
  public:
    Flowmeter(byte); // Constructor
    void  setCalibrationFactor(int);
    long  getPulseCount();
    void  resetFlowMeter(void);
    float getVolume(void);
    void  pulse(void);
    int  getPin(void);
};

class Pump {
  private:
    int   m_pin;
  public:
    Pump(byte);
    int  start(void);
    void  stop(void);
};


class Magnetvalves {
  private:
    int   m_pin;
    int   m_targetV;
    float m_giessFactor;
    float m_currentVolume;
    char  m_plantName[5];
  public:
    Magnetvalves(byte, const char *); // Constructor
    void  setVolumeTarget(int);
    void  incVolumeTarget(int);
    int  readVolumeTarget(void);
    char  *getPlantName(void);
    int  dosing(void);
    int  dosing(float);
    void  setCurrentVolume(float);
    float readCurrentVolume(void);
    void  setGiessFactor(float);
};


class Thermocontrol
{
  private:
    int   m_numberOfTempReadings;
    float m_tempAddition;
    float m_tempCoeff;
    int   m_lowerAveragingHour;

  public:
    Thermocontrol(int);
    void  AddTempReading(float);
    float GetTempAverage(void);
    void  ResetAverage(void);
    void  SetTempCoeff(float);
    void  IncTempCoeff(float);
    float GetTempCoeff(void);
    float GetGiessFactor(void);
    int   GetLowerAveragingHour(void);
};
