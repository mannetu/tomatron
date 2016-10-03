/* water.h */

#include <Arduino.h>
#include <DS3232RTC.h>

enum statusFlag {WATER_IDLE, WATER_BSY};

class Flowmeter {
  private:
    byte pin;
    int calibrationFactor;  // Calibration factor for flow-meter = (Pulses / Liter)
    long pulseCount;
  public:
    Flowmeter(byte); // Constructor
    void setCalibrationFactor(int);
    long  getPulseCount();
    void resetFlowMeter(void);
    float  getVolume(void);
    void pulse(void);
    byte  getPin(void);
};

class Pump {
  private:
    byte pin;
  public:
    Pump(byte);
    byte start(void);
    void stop(void);
};


class Magnetvalves {
  private:
    byte pin;
    byte targetV;
    float m_giessFactor;
    float currV;
    char plant[9];
  public:
    Magnetvalves(byte, const char *); // Constructor
    //static byte flag;
    void setVolumeTarget(int);
    void incVolumeTarget(int);
    byte  readVolumeTarget(void);
    char * getPlant(void);
    byte  dosing(void);
    byte  dosing(float);
    void setCurrentVolume(float);
    float  readCurrentVolume(void);
    void setGiessFactor(float);
};


class Thermocontrol
{
  private:
  int m_numberOfTempReadings;
  float m_tempAddition;
  float m_tempCoeff;

  public:
  Thermocontrol(float);
  void AddTempReading(float);
  int GetTempAverage(void);
  void ResetAverage(void);
  void SetTempCoeff(float);
  void IncTempCoeff(float);
  float GetTempCoeff(void);
  float GetGiessFactor(void);
};
