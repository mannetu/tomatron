/* water.h */

#include <Arduino.h>

enum statusFlag {IDLE, BSY};

class Watercontroller {
  private:
    int gf;
  public:
    int timecheck(int);
    int control(int);
};

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


class Magnetvalves {
  private:
    byte pin;
    byte targetV;
    byte currV;
    char plant[10];
  public:
    static byte flag;
    void setVolumeTarget(int);
    void incVolumeTarget(int);
    byte  readVolumeTarget(void);
    char * getPlant(void);
    void setPin(byte);
    byte  getPin(void);
    byte  dosing(void);
    byte  dosing(byte);
    void setCurrentVolume(int);
    byte  readCurrentVolume(void);
};
