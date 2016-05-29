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
#define CHANNEL 4

/* Nokia 5110 Display
// Software SPI (slower updates, more flexible pin options):
// pin 13 - Serial clock out (SCLK)
// pin 12 - Serial data out (DIN)
// pin 11 - Data/Command select (D/C)
// GND    - LCD chip select (CS)
// pin 10 - LCD reset (RST)
*/
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 12, 11, 10);

/* Pins for Buttons  */
const byte pinUpBtn = 0;      // Up/Increase-Button
const byte pinDownBtn = 1;    // Down/Decrease-Button
const byte pinEnterBtn = 2;   // Enter-Button @ Interupt 0
const byte pinManualBtn = 3;  // Button for Manual Mode
const byte btnDelay = 200; // ButtonDelay

/* Pins for FlowMeter and Valves */
const byte pinFlowMeter = 3;  // Hall-Sensor @ Interupt 1 (must be Pin 3 !!)
const byte pinValve[CHANNEL] = {5, 6, 7, 8};
const byte pinPump = 9;

/* Flag for system status:
*
*  -2 (setParameters)
*  -1 (idle)
*   0, 1, 2 (busy channel)
*/
int giessFlag = -1;
long giessCallLastTime = 0;
time_t giessTime;

/******* Objects *******************/
class Flowmeter flow;
class Pump pump;
class Magnetvalves valve[CHANNEL];


/******* Function prototypes *******/
int checkGiessen(void);
void giessRoutine(void);
void statusDisplay(int, int);
void interuptPulse(void);

void setParameters(void);
void writeParameters();

void calibration();
void calibrationDoseDisplay(int);
void calibrationDisplay(double);


/****** Functions ******************/
void setup() {

  /* Set clock */
  setTime(18, 30, 00, 01, 05, 16); // hour, min, sec, day, month, year

  /*** ONLY FOR INITIAL EEPROM PROGRAMMING ****************************
  giessTime = now() + 3600;
  EEPROM.put(0, giessTime);
  EEPROM.put(sizeof(time_t), 1); // Default value for calibration factor
  *********************************************************************/

  /* Set pin and interrupt configuration for flow meter */
  flow.setPin(pinFlowMeter);
  pinMode(flow.getPin(), INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flow.getPin()), interuptPulse, FALLING);

  /* Set pin for pump */
  pump.setPin(pinPump);
  pinMode(pump.getPin(), OUTPUT);

  /* Set pin configuration for valves */
  for (byte i = 0; i < CHANNEL; i++) {
    valve[i].setPin(pinValve[i]);
    pinMode(valve[i].getPin(), OUTPUT);
  }

  /* Set pin configuration for buttons */
  pinMode(pinEnterBtn, INPUT_PULLUP);
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);

  /* Read giessTime from EEPROM */
  int eeAdress = 0;   // EEPROM-Adress
  EEPROM.get(eeAdress, giessTime);
  eeAdress += sizeof(time_t); // Set position to calibration factor

  /* Read calibration factor from EEPROM */
  int cf;
  EEPROM.get(eeAdress, cf);
  eeAdress += sizeof(int); // Set position to volume targets (after cf)
  flow.setCalibrationFactor(cf);

  /* Read plant name */


  /* Read volume targets from EEPROM */
  int vol;
  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, vol);  //
    eeAdress += sizeof(int); // Set position to next vol target
    valve[i].setVolumeTarget(vol);
  }

  /* Setup LCD display */
  display.begin(); // init done
  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.display(); // show splashscreen
  delay(500);

  /* If Enter is pressed at boot, enter calibration mode */
  if (digitalRead(pinEnterBtn) == LOW) {
    display.clearDisplay();
    display.display();
    delay(5000);
    calibration();
  }

  statusDisplay(-1, -1);
}

void loop() {

  /* Check if it is time for giessen */
  if (giessFlag == -1) giessFlag = checkGiessen();

  /* If so, then call giessRoutine until giessen is done */
  if (giessFlag > -1) giessRoutine();

  /* Update Display */
  if ((millis() - giessCallLastTime > 500)) {
    statusDisplay(giessFlag, -1);
    giessCallLastTime = millis();
  }

  /* Set Target Volumes on button press */
  if (digitalRead(pinEnterBtn) == 0) {
    setParameters();
  }
}

int checkGiessen() {
  /* Check if time for giessen */
  if ((giessFlag == -1) && (hour(giessTime) == hour()) && (minute(giessTime) == minute())) {
    flow.resetFlowMeter();
    return 0;
  }
  return -1;
}

void giessRoutine() {
  /* Giessen routines */
  pump.start();
  valve[giessFlag].setCurrentVolume(flow.getVolume());
  if (valve[giessFlag].dosing() == 0) {
    flow.resetFlowMeter();
    giessFlag++;
  }
  if (giessFlag > CHANNEL-1) {
    pump.stop();
    giessFlag = -1;
  }
}

void statusDisplay(int gf, int ch) {

  display.clearDisplay();
  display.setCursor(0, 0);

  /* Display if nothing is active or parameter set mode */
  if (gf < 0) {   // -1 or -2
    if (gf == -2 && ch == -2) display.setTextColor(WHITE, BLACK);
    display.print(hour());
    display.print(":");
    if(minute() < 10) display.print('0');
    display.print(minute());
    display.setTextColor(BLACK, WHITE);

    display.print("  >");

    if (gf == -2 && ch == -1) display.setTextColor(WHITE, BLACK);
    display.print(hour(giessTime)); display.print(":");
    if(minute(giessTime) < 10) display.print('0');
    display.println(minute(giessTime));
    display.setTextColor(BLACK, WHITE);

    display.println();

    for (int i = 0; i < CHANNEL; i++) {
      display.setCursor(0, (8*i+15));
      display.print(valve[i].getPlant());
      display.setCursor(50, (8*i+15));
      if (gf == -2 && i == ch) display.setTextColor(WHITE, BLACK);
      if (valve[i].readVolumeTarget()<100) display.print(" ");
      if (valve[i].readVolumeTarget()<10) display.print(" ");
      display.print(valve[i].readVolumeTarget());
      display.println(" L");
      display.setTextColor(BLACK, WHITE);
    }
    display.display(); // show screen
    return;
  }

  /* Display during active giessing */
  if (gf > -1) {

    display.print(hour());
    display.print(":");
    if(minute() < 10) display.print('0');
    display.print(minute());
    display.print("  ");
    display.setTextColor(WHITE, BLACK);
    display.println("WASSER");
    display.setTextColor(BLACK, WHITE);
    display.println(flow.getPulseCount());
    for (int i = 0; i < CHANNEL; i++) {

      if (i == gf)
      {
        display.setTextColor(WHITE, BLACK);
        display.setCursor(0, (8*i+15));
        display.print(valve[i].getPlant());
        display.setCursor(50, (8*i+15));
        if (valve[i].readVolumeTarget()<100) display.print(" ");
        if (valve[i].readVolumeTarget()<10) display.print(" ");
        display.print(valve[i].readVolumeTarget());
        display.println(" L");
        display.setTextColor(BLACK, WHITE);
        /*
        display.setCursor(55, (8*i+15));
        if ((100 * valve[i].readCurrentVolume() / valve[i].readVolumeTarget())<100) display.print(" ");
        if ((100 * valve[i].readCurrentVolume() / valve[i].readVolumeTarget())<10) display.print(" ");
        display.print(100 * valve[i].readCurrentVolume() / valve[i].readVolumeTarget());
        display.println("%");
        */
      }
      else
      {
        display.setCursor(0, (8*i+15));
        display.print(valve[i].getPlant());
        display.setCursor(50, (8*i+15));
        if (valve[i].readVolumeTarget()<100) display.print(" ");
        if (valve[i].readVolumeTarget()<10) display.print(" ");
        display.print(valve[i].readVolumeTarget());
        display.println(" L");

      }
    }
    display.display(); // show screen
    return;
  }
}

void interuptPulse() {
  flow.pulse();
}


void calibration() {
  int cf; // calibration factor
  double vol = 10.0; // Volume dispensed for calibration
  int eeAdress = sizeof(time_t);

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
  pump.start();
  while (valve[0].dosing(int (vol)) > 0) {
    valve[0].setCurrentVolume(flow.getVolume());
    calibrationDoseDisplay(valve[0].readCurrentVolume());
  }
  pump.stop();
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
  EEPROM.put(eeAdress, cf);
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

void setParameters() {

  int channel = 0;
  long lastActivity;

  delay(2 *  btnDelay);
  lastActivity = millis();

  /* Set Clock */
  while (digitalRead(pinEnterBtn) == HIGH) {

    if (digitalRead(pinUpBtn) == 0) {
      adjustTime(60);  // Function of time library. Adds given seconds to time.
      lastActivity = millis();
      delay(btnDelay);
    }

    if (digitalRead(pinDownBtn) == 0) {
      adjustTime(-60);  // Function of time library. Adds given seconds to time.
      lastActivity = millis();
      delay(btnDelay);
    }

    statusDisplay(-2, -2);

    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
  }
  delay(2 * btnDelay);
  lastActivity = millis();

  /* Set Timer */
  while (digitalRead(pinEnterBtn) == HIGH) {

    if (digitalRead(pinUpBtn) == 0) {
      giessTime += 60;
      lastActivity = millis();
      delay(btnDelay);
    }

    if (digitalRead(pinDownBtn) == 0) {
      giessTime -= 60;
      lastActivity = millis();
      delay(btnDelay);
    }

    statusDisplay(-2, -1);

    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
  }
  delay(2 * btnDelay);
  lastActivity = millis();

  /* Set target volumes */
  while (channel < CHANNEL) {
    /* Choose channel */
    if (digitalRead(pinEnterBtn) == 0) {
      channel++;
      lastActivity = millis();
      delay(btnDelay);
    }
    /* Adjust volume */
    if (digitalRead(pinUpBtn) == 0) {
      valve[channel].incVolumeTarget(1);
      statusDisplay(-2, channel);
      lastActivity = millis();
      delay(btnDelay);
    }
    if (digitalRead(pinDownBtn) == 0) {
      valve[channel].incVolumeTarget(-1);
      statusDisplay(-2, channel);
      lastActivity = millis();
      delay(btnDelay);
    }

    statusDisplay(-2, channel);

    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
  }

  writeParameters();
  return;
}

void writeParameters() {
  int eeAdress = 0;
  int i;

  /* Write new giessTime to EEPROM */
  EEPROM.put(eeAdress, giessTime);
  eeAdress += (sizeof(time_t) + sizeof(int));

  /* Write new volume targets to EEPROM */
  for (i = 0; i < CHANNEL; i++) {
    EEPROM.put(eeAdress, valve[i].readVolumeTarget());
    eeAdress += sizeof(int);
  }
  /* Show normal display */
  statusDisplay(-1, -1);
}
