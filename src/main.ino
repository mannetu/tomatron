/***********************************************
*   Tomatron v0.1
***********************************************/

#include <Arduino.h>
#include "5110.h"
#include <EEPROM.h>
#include <WIRE.h>       // I2C-Library für RTC-Uhr (z.B. D3231)

#define CHANNEL 3  //Anzahl Wasserkanäle

class Flowmeter {
private:
  int calibrationFactor;  // Calibration factor for flow-meter = (Pulses / Liter)
  int pulseCount;
public:
  void setCalibrationFactor(int);
  void resetFlowMeter(void);
  int getVolume(void);
  void pulse();
} flow;

void Flowmeter::setCalibrationFactor(int cf) {
  calibrationFactor = cf;
}

void Flowmeter::resetFlowMeter() {
  pulseCount = 0;
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

  static int flagOn;
public:
  void setVolumeTarget(int v);
  int readVolumeTarget(void);
  char * getPlant(void);
  int getPin(void);
  void dosing(void);
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

void Magnetvalves::dosing(void) {
  if (flagOn == 0) {
    flow.resetFlowMeter();
    digitalWrite(this->pin, HIGH);
    this->flagOn = 1;
  }

  if (flow.getVolume() < this->targetV) {
    void Display();
  } else
  {
    digitalWrite(this->pin, LOW);
    this->flagOn = 0;
  }
}

int Magnetvalves::getCurrentVolume() {
  return flow.getVolume();
}



/* Uhrzeit aus RTC */

void setup() {

  /* Setup Display */
  DisplaySetup();

  /* Setup Serial for Debug */
  Serial.begin(9600);
  Serial.println("Starte Setup");

  /*  Arduino-Pins für Tasten mit Pull-Up Widerstände konfigurieren */

  /*  Konfigurationsdaten aus EEPROM auslesen                       */
  readEeprom();

  /*  Einstieg in Kalibrierungs-Modus für Flow-Meter wenn Taste X   */
  /*  beim Einschalten gedrückt                                     */

}


void loop() {

  /* Uhrzeit aus RTC ziehen  */

  /* Interupt-Pin 1 wird bei jedem Puls des FlowMeters ausgelöst */

  /* Interupt pin 2 führt zur zum Menu - Einstellung */

  }


/***********  Setup Routines  **********/
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

void readEeprom() {
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

}

/***********  Set Volume Routines  **********
int EepromWrite(int i){
  int eeAdress = 0 + sizeof(float) + i*sizeof(int); // "vorspulen"
  EEPROM.put(eeAdress, valve[i].getVolumeTarget());
  eeAdress = 0;
  return 0;
}


***********  Flow-Meter Calibration  **********
void calibration() {
  display.clearDisplay();
  display.print("Tomatron v0.1   "); display.println("Calibration");
  display.println();
  display.println("Nach Tastendruck laufen");
  display.println("ca. 10L Wasser aus Schlauch 1.");
  display.println("Genaue Menge auswiegen!");
  display.display(); // show screen

  float exakteMenge = 10.0;
  float cf;

  DisplayCalibration(exakteMenge);

  // Einstellung exakteMenge über Up/Down-Taster, Bestätigung durch Enter

  while ((PIND & BUTTON0)) { // solange Arduino-Pin 7 auf HIGH ->Shift-Taste nicht gedrückt

    if (!(PIND & BUTTON1)) {	// wenn Arduino-Pin 6 auf LOW -> Einstell-Taste gedrückt
      exakteMenge += 0.1;
      DisplayCalibration(exakteMenge);
      delay(200);
    }

    if (!(PIND & BUTTON1)) {
      exakteMenge -= 0.1;
      DisplayCalibration(exakteMenge);
      delay(200);
    }
  }

  // Berechnung Kalibrierfaktor und speichern im EEPROM *
  cf = meterPulses / exakteMenge;
  flow.setCalibrationFactor(cf);
  EEPROM.put(0, cf);

  meterPulses = 0;
}

void DisplayCalibration(float menge) {

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
*/

/***********  Loop Routines  **********/
void Display() {

  display.clearDisplay();

  display.print("Tomatron v0.1   "); display.println("xx:xx"); //Uhrzeit
  display.println();

  for (int i = 0; i < CHANNEL; i++) {
    display.print(valve[i].getPlant()); display.print(valve[i].readVolumeTarget());
    display.print(" "); display.println(valve[i].getCurrentVolume());
  }

  display.display(); // show screen
}


void InteruptRoutine() {
  flow.pulse();
}
