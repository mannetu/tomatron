/* water.h */

#include <Arduino.h>

enum statusFlag {IDLE, BSY};


class Flowmeter {
  private:
    byte pin;
    int calibrationFactor;  // Calibration factor for flow-meter = (Pulses / Liter)
    long pulseCount;
  public:
    void setCalibrationFactor(int);
    long  getPulseCount();
    void resetFlowMeter(void);
    byte  getVolume(void);
    void pulse(void);
    void setPin(byte);
    byte  getPin(void);
};

class Pump {
  private:
    byte pin;
  public:
    void setPin(byte);
    byte getPin(void);
    void start(void);
    void stop(void);
};


class Magnetvalves {
  private:
    byte pin;
    byte targetV;
    byte currV;
    char plant[9];
  public:
    static byte flag;
    void setVolumeTarget(int);
    void incVolumeTarget(int);
    byte  readVolumeTarget(void);
    void setPlant(const char *);
    char * getPlant(void);
    void setPin(byte);
    byte  getPin(void);
    byte  dosing(void);
    byte  dosing(byte);
    void setCurrentVolume(int);
    byte  readCurrentVolume(void);
};
