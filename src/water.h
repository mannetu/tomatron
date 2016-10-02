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
    byte  getVolume(void);
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
    byte currV;
    char plant[9];
  public:
    Magnetvalves(byte, const char *); // Constructor
    static byte flag;
    void setVolumeTarget(int);
    void incVolumeTarget(int);
    byte  readVolumeTarget(void);
    char * getPlant(void);
    byte  dosing(void);
    byte  dosing(byte);
    void setCurrentVolume(int);
    byte  readCurrentVolume(void);
    void setGiessFactor(float);
    float getGiessFactor(void);
};


class Thermocontrol
{
  private:
  int m_numberOfTempReadings;
  float m_tempAddition;
  int m_tempCoeff;

  public:
  Thermocontrol(int);
  int AddTempReading(float);
  int GetTempAverage(void);
  void ResetAverage(void);
  float GetGiessFactor(void);
};
