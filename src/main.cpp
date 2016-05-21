/***********************************************
 *  Tomatron v0.1
 *
 *
 ***********************************************/

#include <Arduino.h>
#define _ArduinoH_
#include <EEPROM.h>
#include <Time.h>
//#include <DS3232RTC.h>
#include <WIRE.h>       // I2C-Library für RTC-Uhr (z.B. D3231)
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define _WaterH_
#include "Water.h"

// Water channels
#define CHANNEL 3

/* Nokia 5110 Display
 * Hardware SPI (faster, but must use certain hardware pins):
 * SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
 * MOSI is LCD DIN - this is pin 11 on an Arduino Uno
 * pin 9 - Data/Command select (D/C)
 * pin 8 - LCD chip select (CS)
 * pin 7 - LCD reset (RST)
 */
Adafruit_PCD8544 display = Adafruit_PCD8544(9, 8, 7);

/* Pins for Buttons  */
const byte pinUpBtn = 0;      // Up/Increase-Button
const byte pinDownBtn = 1;    // Down/Decrease-Button
const byte pinEnterBtn = 2;   // Enter-Button @ Interupt 0

// Pins for FlowMeter and MagneticValves
const byte pinFlowMeter = 3;  // Hall-Sensor @ Interupt 1
const byte pinValve[CHANNEL] = {4, 5, 6};

/* Flag for active watering thread.
 * Case -1 inactive, case 0, 1, and 2 for active channel    */
int giessen = -1;
// Clock
int giessenHour = 18;
int giessenMinute = 30;


/***** Objects *******/
class Flowmeter flow;
class Magnetvalves valve[CHANNEL];


/******* Function prototypes *******/
void displaySetup(void);
int  readEeprom(void);

void statusDisplay(int);
void interuptPulse(void);

void setTargetVolumes(void);
void setTargetDisplay(int);

void calibration();
void calibrationDisplay(float);


/****** Functions *******/
void setup() {

  /* Setup Serial for Debug */
  Serial.begin(9600);
  Serial.println("Starte Setup");

  /* Setup Clock */
  setTime(12,00,00,01,05,16);

  /* Setup Display */
  displaySetup();

  /* Read config data from EEPROM */
  readEeprom();

  /* Pin configuration for flow meter pulse and interrupt */
  flow.setPin(pinFlowMeter);
  pinMode(flow.getPin(), INPUT_PULLUP);
  attachInterrupt(flow.getPin(), interuptPulse, FALLING);

  /* Pin configuration for valves */
  for (int i = 0; i < CHANNEL; i++) {
    valve[i].setPin(pinValve[i]);
    pinMode(valve[i].getPin(), OUTPUT);
  }

  /* Pin configuration buttons */
  pinMode(pinEnterBtn, INPUT_PULLUP);
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);
}

void loop() {

  /* Giess-Routine */
  if ((giessenHour == hour()) && (giessenMinute == minute())) {
    giessen = 0;
    flow.resetFlowMeter();
  }

  /* Giessen-Loop */
  if (giessen > -1) {

    valve[giessen].setCurrentVolume(flow.getVolume());

    if (valve[giessen].dosing() == 0) {
      giessen++;
    }

    if (giessen > CHANNEL-1) {
      giessen = -1;
    }

    statusDisplay(giessen);
  }


  /* Einstellung Giessmengen */
  if (digitalRead(pinEnterBtn) == 0) {
    setTargetVolumes();
  }

  /* Einstellung Uhr */
  if (digitalRead(pinUpBtn) == 0) {
    adjustTime(60);
    delay(200);
  }

  if (digitalRead(pinDownBtn) == 0) {
    adjustTime(-60);
    delay(200);
  }

}

void displaySetup() {
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
  int volTarget;
  int cf;
  int eeAdress = 0;

  EEPROM.get(eeAdress, cf);  // Load calibration factor
  flow.setCalibrationFactor(cf);

  eeAdress = sizeof(int);
  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, volTarget);  //
    valve[i].setVolumeTarget(volTarget);
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

void calibration() {
  float cf;
  double calVolume = 10;

  display.clearDisplay();
  display.setCursor(0,0);
  //Display        12345678901234
  display.println("Kalibierung:");
  display.println("Taste drücken!");
  display.println("10L aus V1:");
  display.println("Genaue Menge");
  display.println("auswiegen!");
  display.display(); // show screen

  while (digitalRead(pinEnterBtn)) {/* Wait on button */ }
  flow.resetFlowMeter();

  while (!valve[0].dosing((int)calVolume)) {
    valve[0].setCurrentVolume(flow.getVolume());
    calibrationDisplay(valve[0].readCurrentVolume());
  }

  // Einstellung exakteMenge über Up/Down-Taster, Bestätigung durch Enter

  calibrationDisplay(valve[0].readCurrentVolume());

  while (digitalRead(pinEnterBtn)) {

    if (digitalRead(pinUpBtn)) {	// wenn Arduino-Pin 6 auf LOW -> Einstell-Taste gedrückt
      calVolume += 0.1;
      calibrationDisplay(calVolume);
      delay(200);
    }

    if (digitalRead(pinDownBtn)) {
      calVolume -= 0.1;
      calibrationDisplay(calVolume);
      delay(200);
    }
  }

  // Berechnung Kalibrierfaktor und speichern im EEPROM *
  cf = flow.getPulseCount() / calVolume;
  flow.setCalibrationFactor(cf);
  EEPROM.put(0, cf);
}

void calibrationDisplay(double menge) {
  display.clearDisplay();
  display.setCursor(0,0);
  //Display        12345678901234
  display.println("Mit +/- Tasten");
  display.println("10 * Menge");
  display.println("Enter");
  display.println();
  display.setTextSize(1);
  display.print(menge, 3); display.println(" kg");
  display.setTextSize(1);
  display.display();
}

void statusDisplay(int ch) {

  display.clearDisplay();
  display.setCursor(0, 0);

  if (ch == -1) {
    display.print("INAKTIV  ");
    // digital clock display of the time
    display.print(hour()); display.print(":");
    if(minute() < 10) display.print('0');
    display.println(minute());

    display.println();
    for (int i = 0; i < CHANNEL; i++) {
      display.print(valve[i].getPlant());
      display.setCursor(i, 8);
      display.println(valve[i].readVolumeTarget());
    }
    display.display(); // show screen
    return;
  }

  display.print("AKTIV: KANAL "); display.println(ch+1);
  display.println();
  for (int i = 0; i < CHANNEL; i++) {
    if (i < ch+1) {
      display.print(valve[i].getPlant());
      display.setCursor(i, 8);
      display.print(valve[i].readVolumeTarget()); display.setCursor(i, 11);
      display.print(valve[i].readCurrentVolume()/valve[i].readVolumeTarget());
      display.println(" %%");
      } else {
        display.print(valve[i].getPlant()); display.setCursor(i, 9);
        display.println(valve[i].readVolumeTarget());
      }
    }
  display.display(); // show screen
  return;
}

void interuptPulse() {
  flow.pulse();
}

void setTargetVolumes() {
  int channel = 0;

  while (digitalRead(pinEnterBtn)) {
    if (digitalRead(pinDownBtn) == 0) {
      channel++;
      setTargetDisplay(channel);
      delay(200);
    }
    if (digitalRead(pinUpBtn) == 0) {
      channel--;
      setTargetDisplay(channel);
      delay(200);
    }
  }

  while (digitalRead(pinEnterBtn)) {
    if (digitalRead(pinDownBtn) == 0) {
      valve[channel].incVolumeTarget(1);
      setTargetDisplay(channel);
      delay(200);
    }
    if (digitalRead(pinUpBtn) == 0) {
      valve[channel].incVolumeTarget(-1);
      setTargetDisplay(channel);
      delay(200);
    }
  }
  statusDisplay(-1);
}

void setTargetDisplay(int ch) {

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Zielvolumen:   "); display.println("xx:xx"); //Uhrzeit
  display.println();

  for (int i = 0; i < CHANNEL; i++) {
    if (i == ch) {
      display.setTextColor(WHITE, BLACK);
      display.print(valve[i].getPlant()); display.print(valve[i].readVolumeTarget());
      display.setTextColor(BLACK, WHITE);
    } else {
      display.print(valve[i].getPlant()); display.print(valve[i].readVolumeTarget());
    }
  }

  display.display(); // show screen
}
