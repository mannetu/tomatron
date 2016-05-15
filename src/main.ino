/***********************************************
*   Tomatron v0.1
***********************************************/

#include <Arduino.h>
#include "5110.h"
#include <EEPROM.h>
#include <WIRE.h>       // I2C-Library für RTC-Uhr (z.B. D3231)

#define CHANNEL 3  //Anzahl Wasserkanäle

const byte pulsePin = 2;

int giessen = -1;

class Flowmeter {
  private:
    int calibrationFactor;  // Calibration factor for flow-meter = (Pulses / Liter)
    int pulseCount;
  public:
    static int flag;
    void setCalibrationFactor(int);
    int getPulseCount();
    void resetFlowMeter(void);
    int getVolume(void);
    void pulse();
} flow;

enum flowStatus {IDLE, BUSY};
int Flowmeter::flag = IDLE;

void Flowmeter::setCalibrationFactor(int cf) {
  calibrationFactor = cf;
  EEPROM.put(0, calibrationFactor);
}

int Flowmeter::getPulseCount() {
  return pulseCount;
}


void Flowmeter::resetFlowMeter() {
  pulseCount = 0;
  flag = IDLE;
}

int Flowmeter::getVolume() {
  return (pulseCount / calibrationFactor);
}

void Flowmeter::pulse() {
  pulseCount++;
}

class Magnetvalves {
  private:
    int pin;
    int targetV;
    int currV;
    char plant[10];
  public:
    void setVolumeTarget(int v);
    int readVolumeTarget(void);
    char * getPlant(void);
    int getPin(void);
    int dosing(void);
    int getCurrentVolume(void);
}  valve[CHANNEL];

void Magnetvalves::setVolumeTarget(int v) {
  targetV = v;
}

int Magnetvalves::readVolumeTarget() {
  return targetV;
}

char * Magnetvalves::getPlant() {
  return plant;
}

int Magnetvalves::getPin() {
  return pin;
}

int Magnetvalves::dosing(void) {
  if (flow.flag == IDLE) {
    flow.resetFlowMeter();
    digitalWrite(this->pin, HIGH);
    flow.flag = BUSY;
    return 0;
  } else {
    return 2;
  }

  if (flow.getVolume() < this->targetV) {
    return 0;
  } else
  {
    digitalWrite(this->pin, LOW);
    flow.flag = IDLE;
    return 1;
  }
}

int Magnetvalves::getCurrentVolume() {
  return flow.getVolume();
}


void setup() {

  /* Setup Display */
  DisplaySetup();

  /* Setup Serial for Debug */
  Serial.begin(9600);
  Serial.println("Starte Setup");


  /*  Konfigurationsdaten aus EEPROM auslesen                       */
  readEeprom();

  /* Pin for flow meter pulse and interrupt */
  pinMode(pulsePin, INPUT_PULLUP);
  attachInterrupt(pulsePin, interuptPulse, FALLING);

}


void loop() {

  if (/* es ist Zeit zum giessen */1) {
    giessen = 0;
  }

  /* Check ob gegossen werden muss */
  if (giessen > -1) {
    if (valve[giessen].dosing() == 1) {
      giessen++;
    }
    statusDisplay();
  }

  /* Check ob alles gegossen */
  if (giessen == CHANNEL+1) {
    giessen = -1;
  }



}


void DisplaySetup() {
  display.begin(); // init done
  display.setContrast(50); // you can change the contrast around to adapt the display for the best viewing!
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.display(); // show splashscreen
  delay(1000);
  display.clearDisplay();   // clears the screen and buffer
  display.display();
}

int readEeprom() {
  int vol;
  int cf;
  int eeAdress = 0;

  EEPROM.get(eeAdress, cf);  // Load calibration factor
  flow.setCalibrationFactor(cf);

  eeAdress = sizeof(int);
  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, vol);  //
    valve[i].setVolumeTarget(vol);
    eeAdress += sizeof(int);
  }
  return 0;
}

int writeEeprom(void){
  int eeAdress = 0 + sizeof(int); // "vorspulen" nach calibration factor
  for (int i = 0; i < CHANNEL; i++) {
    EEPROM.put(eeAdress, valve[i].readVolumeTarget());
    eeAdress += sizeof(int);
  }
  eeAdress = 0;
  return 0;
}


/***********  Flow-Meter Calibration  **********

void calibration() {
  float cf;
  float exactAmount = 10;

  display.clearDisplay();
  display.print("Tomatron v0.1   "); display.println("Calibration");
  display.println();
  display.println("Nach Tastendruck laufen");
  display.println("ca. 10L Wasser aus Schlauch 1.");
  display.println("Genaue Menge auswiegen!");
  display.display(); // show screen


  valve[0].setVolumeTarget(10);
  while (!valve[0].dosing()) {
  //  calibrationDisplay(valve[0].getCurrentVolume());
  }


  // Einstellung exakteMenge über Up/Down-Taster, Bestätigung durch Enter
  calibrationDisplay(exactAmount);
  while (digitalRead(4)) {

    if (digitalRead(5)) {	// wenn Arduino-Pin 6 auf LOW -> Einstell-Taste gedrückt
      exactAmount += 0.1;
      calibrationDisplay(exactAmount);
      delay(200);
    }

    if (digitalRead(6)) {
      exactAmount -= 0.1;
      calibrationDisplay(exactAmount);
      delay(200);
    }
  }

  // Berechnung Kalibrierfaktor und speichern im EEPROM *
  cf = flow.getPulseCount() / exactAmount;
  flow.setCalibrationFactor(cf);
}

void calibrationDisplay(float menge) {

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Mit Up/Down-Tasten");
  display.println("exakte Menge einstellen");
  display.println("");
  display.setTextSize(2);
  display.print(menge); display.println(" L");
  display.setTextSize(1);
  display.display();

}


/***********  Loop Routines  **********/
void statusDisplay() {

  display.clearDisplay();

  display.print("Tomatron v0.1   "); display.println("xx:xx"); //Uhrzeit
  display.println();

  for (int i = 0; i < CHANNEL; i++) {
    display.print(valve[i].getPlant()); display.print(valve[i].readVolumeTarget());
    display.print(" "); display.println(valve[i].getCurrentVolume());
  }

  display.display(); // show screen
}


void interuptPulse() {
  flow.pulse();
}
