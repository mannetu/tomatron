/* water.h */

enum flowStatus {IDLE, BUSY};

class Flowmeter {
  private:
    int calibrationFactor;  // Calibration factor for flow-meter = (Pulses / Liter)
    int pulseCount;
  public:
    void setCalibrationFactor(int);
    int getPulseCount();
    void resetFlowMeter(void);
    int getVolume(void);
    void pulse();
};




class Magnetvalves {
  private:
    int pin;
    int targetV;
    int currV;
    char plant[10];
  public:
    static int flag;
    void setVolumeTarget(int v);
    int readVolumeTarget(void);
    char * getPlant(void);
    int getPin(void);
    int dosing(void);
    int dosing(int);
    void setCurrentVolume(int);
    int readCurrentVolume(void);
};
