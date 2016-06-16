/* water.h */

#include <Arduino.h>

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
};
