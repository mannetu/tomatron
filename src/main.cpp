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

/* Pins for FlowMeter and Valves */
const byte pinFlowMeter = 3;  // Hall-Sensor @ Interupt 1
const byte pinValve[CHANNEL] = {4, 5, 6};

/* Flag for active watering thread.
 * Case -1 inactive, case 0, 1, and 2 for active channel
 */
int giessen = -1;
// Clock
int giessenHour = 18;
int giessenMinute = 30;


/***** Objects *******/
class Flowmeter flow;
class Magnetvalves valve[CHANNEL];


/******* Function prototypes *******/
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

  /* Set Clock */
  setTime(18, 29, 00, 01, 05, 16); // hour, min, sec, day, month, year

  /* Read calibration factor from EEPROM */
  int eeAdress = 0;   // EEPROM-Adress
  int cf;
  EEPROM.get(eeAdress, cf);
  flow.setCalibrationFactor(cf);

  /* Read volume targets from EEPROM */
  int vol;
  eeAdress = sizeof(int); // Set position to volume targets (after cf)
  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, vol);  //
    valve[i].setVolumeTarget(vol);
    eeAdress += sizeof(int);
  }

  /* Set pin and interrupt configuration for flow meter */
  flow.setPin(pinFlowMeter);
  pinMode(flow.getPin(), INPUT_PULLUP);
  attachInterrupt(flow.getPin(), interuptPulse, FALLING);

  /* Set pin configuration for valves */
  for (byte i = 0; i < CHANNEL; i++) {
    valve[i].setPin(pinValve[i]);
    pinMode(valve[i].getPin(), OUTPUT);
  }

  /* Set pin configuration for buttons */
  pinMode(pinEnterBtn, INPUT_PULLUP);
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);

  /* Setup LCD display */
  display.begin(); // init done
  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.display(); // show splashscreen
  delay(1000);
  statusDisplay(giessen);
}

void loop() {

  /* Check if time for giessen */
  if ((giessenHour == hour()) && (giessenMinute == minute())) {
    giessen = 0;
    flow.resetFlowMeter();
  }

  /* Giessen routine */
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


  /* Set Target Volumes */
  if (digitalRead(pinEnterBtn) == 0) {
    setTargetVolumes();
  }

  /* Set Clock */
  if (digitalRead(pinUpBtn) == 0) {
    adjustTime(60);  // Function of time library. Adds given seconds to time.
    delay(200);
  }
  if (digitalRead(pinDownBtn) == 0) {
    adjustTime(-60); // Function of time library. Adds given seconds to time.
    delay(200);
  }

}

void statusDisplay(int ch) {

  display.clearDisplay();
  display.setCursor(0, 0);

  /* Display if nothing is active */
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

  /* Display during active giessing */
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


void calibration() {
  int cf; // calibration factor
  double vol = 10; // Volume dispensed for calibration

  /* Show instructions on display */
  display.clearDisplay();
  display.setCursor(0,0);
  //Display-Pos    12345678901234
  display.println("Kalibierung:");
  display.println("Taste drücken!");
  display.println("10L aus V1:");
  display.println("Genaue Menge");
  display.println("auswiegen!");
  display.display();

  /* Wait on button press */
  while (digitalRead(pinEnterBtn)) {/* Wait on button press */ }
  flow.resetFlowMeter();

  /* Dispense volume vol */
  while (!valve[0].dosing(int (vol))) {
    valve[0].setCurrentVolume(flow.getVolume());
    calibrationDisplay(valve[0].readCurrentVolume());
  }

  /* Adjust exact volume with Up/Down-Buttons and press Enter */
  calibrationDisplay(valve[0].readCurrentVolume());
  while (digitalRead(pinEnterBtn)) {
    if (digitalRead(pinUpBtn) == 0) {
      vol += 0.1;
      calibrationDisplay(vol);
      delay(200);
    }
    if (digitalRead(pinDownBtn) == 0) {
      vol -= 0.1;
      calibrationDisplay(vol);
      delay(200);
    }
  }
  delay(500);

  /* Calculate calibration factor and write to EEPROM */
  cf = flow.getPulseCount() / vol;
  flow.setCalibrationFactor(cf);
  EEPROM.put(0, cf);
}

void calibrationDisplay(double vol) {
  display.clearDisplay();
  display.setCursor(0,0);
  //Display        12345678901234
  display.println("Volumen ein-");
  display.println("stellen und ");
  display.println("bestätigen");
  display.println();
  display.print(vol, 3); display.println(" kg");
  display.display();
}


void setTargetVolumes() {
  int channel = 0;
  int eeAdress;

  /* Choose channel */
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

  /* Adjust volume */
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

  /* Write new target to EEPROM */
  eeAdress = (channel + 1) * sizeof(int); // +1 due to calibration factor on
  EEPROM.put(eeAdress, valve[channel].readVolumeTarget());

  /* Show status display */
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
