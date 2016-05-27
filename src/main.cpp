/***********************************************
 *  Tomatron v0.1
 *
 *
 ***********************************************/

#include <Arduino.h>
#define _ArduinoH_
#include <EEPROM.h>
#include <Time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define _WaterH_
#include "Water.h"

/* Water channels */
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
int btnDelay = 200; // ButtonDelay

/* Pins for FlowMeter and Valves */
const byte pinFlowMeter = 3;  // Hall-Sensor @ Interupt 1
const byte pinValve[CHANNEL] = {4, 5, 6};

/* Flag for system status:  -1 (idle) / 0, 1, 2 (busy chanel) */
int giessFlag = -1;
int giessCallLastTime = 0;

/* Default time for giessen */
int giessenHour = 18;
int giessenMinute = 30;


/******* Objects *******************/
class Flowmeter flow;
class Magnetvalves valve[CHANNEL];


/******* Function prototypes *******/
int checkGiessen(void);
void giessRoutine(void);
void statusDisplay(int);
void interuptPulse(void);

void setTargetVolumes(void);
void setTargetDisplay(int);

void calibration();
void calibrationDoseDisplay(int);
void calibrationDisplay(double);


/****** Functions ******************/
void setup() {

  /* Setup Serial for Debug */
  //  Serial.begin(9600);
  Serial.println("Starte Setup");

  /* Set pin and interrupt configuration for flow meter */
  flow.setPin(pinFlowMeter);
  pinMode(flow.getPin(), INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flow.getPin()), interuptPulse, FALLING);

  /* Set pin configuration for valves */
  for (byte i = 0; i < CHANNEL; i++) {
    valve[i].setPin(pinValve[i]);
    pinMode(valve[i].getPin(), OUTPUT);
  }

  /* Set pin configuration for buttons */
  pinMode(pinEnterBtn, INPUT_PULLUP);
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);

  /* Set Clock */
  setTime(18, 25, 00, 01, 05, 16); // hour, min, sec, day, month, year

  /* Read calibration factor from EEPROM */
  int eeAdress = 0;   // EEPROM-Adress
  int cf;
  EEPROM.get(eeAdress, cf);
  flow.setCalibrationFactor(cf);

  /* Read volume targets from EEPROM */
  int vol;
  for (byte i = 0; i < (CHANNEL); i++) {
    eeAdress = (i+1) * sizeof(int); // Set position to volume targets (after cf)
    EEPROM.get(eeAdress, vol);  //
    valve[i].setVolumeTarget(vol);
  }

  /* Setup LCD display */
  display.begin(); // init done
  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.display(); // show splashscreen
  delay(1000);

  /* If Enter is pressed at boot, enter calibration mode */
  if (digitalRead(pinEnterBtn) == LOW) {
    display.clearDisplay();
    display.display();
    delay(5000);
    calibration();
  }

  statusDisplay(-1);
}

void loop() {

  /* Check if it is time for giessen */
  if (giessFlag == -1) giessFlag = checkGiessen();

  /* If so, then call giessRoutine until giessen is done */
  if (giessFlag > -1) giessRoutine();

  /* Update Display */
  if ((millis() - giessCallLastTime > 500)) {
    statusDisplay(giessFlag);
    giessCallLastTime = millis();
  }

  /* Set Target Volumes on button press */
  if (digitalRead(pinEnterBtn) == 0) {
    setTargetVolumes();
  }

  /* Set Clock on button press */
  if (digitalRead(pinUpBtn) == 0) {
    adjustTime(60);  // Function of time library. Adds given seconds to time.
    delay(btnDelay);
  }
  if (digitalRead(pinDownBtn) == 0) {
    adjustTime(-60); // Function of time library. Adds given seconds to time.
    delay(btnDelay);
  }
}

int checkGiessen() {
  /* Check if time for giessen */
  if ((giessFlag == -1) && (giessenHour == hour()) && (giessenMinute == minute())) {
    flow.resetFlowMeter();
    return 0;
  }
  return -1;
}

void giessRoutine() {
  /* Giessen routines */
    valve[giessFlag].setCurrentVolume(flow.getVolume());
    if (valve[giessFlag].dosing() == 0) {
      flow.resetFlowMeter();
      giessFlag++;
    }
    if (giessFlag > CHANNEL-1) {
      giessFlag = -1;
    }
  }

void statusDisplay(int ch) {

  display.clearDisplay();
  display.setCursor(0, 0);

  /* Display if nothing is active */
  if (ch == -1) {
    display.print("Zeit ");
    display.print(hour()); display.print(":");
    if(minute() < 10) display.print('0');
    display.print(minute()); display.print(":");
    if(second() < 10) display.print('0');
    display.print(second()); display.println();

    display.print("Timer ");
    display.print(giessenHour); display.print(":");
    if(giessenMinute < 10) display.print('0');
    display.println(giessenMinute);

    display.println();
    for (int i = 0; i < CHANNEL; i++) {
      display.setCursor(0, (8*i+20));
      display.print(valve[i].getPlant());
      display.setCursor(25, (8*i+20));
      if (valve[i].readVolumeTarget()<100) display.print(" ");
      if (valve[i].readVolumeTarget()<10) display.print(" ");
      display.println(valve[i].readVolumeTarget());
    }
    display.display(); // show screen
    return;
  }

  /* Display during active giessing */
  display.print("Giessen! "); display.println(ch+1);
  display.print("Pulse: "); display.println(flow.getPulseCount());
  for (int i = 0; i < CHANNEL; i++) {

    if (i < ch+1)
    {
      display.setCursor(0, (8*i+20));
      display.print(valve[i].getPlant());
      display.setCursor(14, (8*i+20));
      if (valve[i].readVolumeTarget()<100) display.print(" ");
      if (valve[i].readVolumeTarget()<10) display.print(" ");
      display.print(valve[i].readVolumeTarget()); display.print(" L");
      display.setCursor(49, (8*i+20));
      if ((100 * valve[i].readCurrentVolume() / valve[i].readVolumeTarget())<100) display.print(" ");
      if ((100 * valve[i].readCurrentVolume() / valve[i].readVolumeTarget())<10) display.print(" ");
      display.print(100 * valve[i].readCurrentVolume() / valve[i].readVolumeTarget());
      display.println("%");
    }
    else
    {
      display.setCursor(0, (8*i+20));
      display.print(valve[i].getPlant());
      display.setCursor(14, (8*i+20));
      if (valve[i].readVolumeTarget()<100) display.print(" ");
      if (valve[i].readVolumeTarget()<10) display.print(" ");
      display.print(valve[i].readVolumeTarget()); display.println(" L");
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
  double vol = 10.0; // Volume dispensed for calibration

  /* Show instructions on display */
  display.clearDisplay();
  display.setCursor(0,0);
  //Display-Pos    12345678901234
  display.println("Kalibierung");
  display.println("Taste drucken");
  display.println("10L aus V1:");
  display.println("Genaue Menge");
  display.println("auswiegen!");
  display.display();
  flow.resetFlowMeter();

  while (digitalRead(pinEnterBtn)) { };

  /* Dispense volume vol */
  while (valve[0].dosing(int (vol)) > 0) {
    valve[0].setCurrentVolume(flow.getVolume());
    calibrationDoseDisplay(valve[0].readCurrentVolume());
  }

  /* Adjust exact volume with Up/Down-Buttons and press Enter */
  calibrationDisplay(double(valve[0].readCurrentVolume()));
  vol = double(valve[0].readCurrentVolume());
  while (digitalRead(pinEnterBtn)) {
    if (digitalRead(pinUpBtn) == 0) {
      vol += 0.1;
      calibrationDisplay(vol);
      delay(btnDelay);
    }
    if (digitalRead(pinDownBtn) == 0) {
      vol -= 0.1;
      calibrationDisplay(vol);
      delay(btnDelay);
    }
  }
  delay(2 * btnDelay);

  /* Calculate calibration factor and write to EEPROM */
  cf = flow.getPulseCount() / vol;
  flow.setCalibrationFactor(cf);
  EEPROM.put(0, cf);
}

void calibrationDoseDisplay(int vol) {
  display.clearDisplay();
  display.setCursor(0,0);
  //Display        12345678901234
  display.println("Dosierung");
  display.println("");
  display.println("");
  display.println();
  display.print(vol, 1); display.println(" kg");
  display.display();
}

void calibrationDisplay(double vol) {
  display.clearDisplay();
  display.setCursor(0,0);
  //Display        12345678901234
  display.println("Volumen ein-");
  display.println("stellen und ");
  display.println("bestaetigen");
  display.println();
  display.print(vol, 1); display.println(" kg");
  display.display();
}


void setTargetVolumes() {
  int channel = 0;
  int eeAdress;
  delay(1000);

  while (channel < CHANNEL) {
    /* Choose channel */
    if (digitalRead(pinEnterBtn) == 0) {
      channel++;
      delay(btnDelay);
    }

    /* Adjust volume */
    if (digitalRead(pinUpBtn) == 0) {
      valve[channel].incVolumeTarget(1);
      setTargetDisplay(channel);
      delay(btnDelay);
    }

    if (digitalRead(pinDownBtn) == 0) {
      valve[channel].incVolumeTarget(-1);
      setTargetDisplay(channel);
      delay(btnDelay);
    }
  setTargetDisplay(channel);
  }

  /* Write new target to EEPROM */
  for (channel = 0; channel < CHANNEL; channel++) {
    eeAdress = (channel+1) * sizeof(int);
    EEPROM.put(eeAdress, valve[channel].readVolumeTarget());
  }

  /* Show status display */
  statusDisplay(-1);
}

void setTargetDisplay(int ch) {

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Set Volumes");
  display.println();

  for (int i = 0; i < CHANNEL; i++) {

    if (i == ch) {
      display.setTextColor(WHITE, BLACK);
      display.print(valve[i].getPlant());
      if (valve[i].readVolumeTarget() < 100) display.print(" ");
      if (valve[i].readVolumeTarget() < 10) display.print(" ");
      display.print(valve[i].readVolumeTarget()); display.println(" L");
      display.setTextColor(BLACK, WHITE);
    } else {
      display.print(valve[i].getPlant());
      if (valve[i].readVolumeTarget() < 100) display.print(" ");
      if (valve[i].readVolumeTarget() < 10) display.print(" ");
      display.print(valve[i].readVolumeTarget()); display.println(" L");
    }
  }

  display.display(); // show screen
}
