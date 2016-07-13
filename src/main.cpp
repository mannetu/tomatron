/***********************************************
*  Tomatron v0.1
*
*
***********************************************/

#include <Arduino.h>
#define _ArduinoH_
#include <EEPROM.h>
#include <Time.h>
#include <avr/wdt.h>  // Watchdog-Timer
#include <avr/sleep.h>
#include <avr/power.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define _WaterH_
#include "water.h"

/* Water channels */
#define CHANNEL 4
#define DISPLAY_UPDATE 250

/* Giessflag */
enum giessFlag {
  CTRL_SLEEP = -3,
  CTRL_SET = -2,
  CTRL_IDLE = -1,
  CTRL_ACT = 0  // 1, 2, 3
};

struct s_giess {
  int flag = -1;
  long lastCall = 0;
  time_t time;
} giess;

/*** Nokia 5110 Display  ******
 Software SPI (slower updates, more flexible pin options):
 pin 13 - Serial clock out (SCLK)
 pin 12 - Serial data out (DIN)
 pin 11 - Data/Command select (D/C)
 GND    - LCD chip select (CS)
 pin 10 - LCD reset (RST)
*/
Adafruit_PCD8544 display = Adafruit_PCD8544(13, 12, 11, 10);

/* Buttons  */
const byte pinUpBtn =     0;   // Pin Up/Increase-Button
const byte pinDownBtn =   1;   // Pin Down/Decrease-Button
const byte pinEnterBtn =  2;   // Pin Enter-Button @ Interupt 0

unsigned int btnDelay =   200; // Debounce delay

/******* Objects *******************/
Flowmeter flow = Flowmeter(3); // Interupt 1 -> Pin 3 must not be changed!

Magnetvalves valve[CHANNEL] = {
  Magnetvalves(5, "Tomaten"),
  Magnetvalves(6, "Gurken"),
  Magnetvalves(7, "Paprika"),
  Magnetvalves(8, "Bohnen")
};

Pump pump = Pump(4);

volatile boolean alarmIsrWasCalled = true;

/******* Function prototypes *******/
int checkGiessen(void);
void giessRoutine(void);
void statusDisplay(int, int);

void interruptPulse(void);

void calibration();
void calibrationDoseDisplay(int);
void calibrationDisplay(double);

void setParameters(void);
void writeParameters();

void btnInterruptSleep(void);
void enterSleep(void);


/****** Functions ******************/
void setup() {

  /*** ONLY FOR INITIAL EEPROM AND RTC PROGRAMMING ********************
  setTime(18, 30, 00, 01, 05, 16); // hour, min, sec, day, month, year
  RTC.set(now());
  giess.time = now() + 3600;
  EEPROM.put(0, giess.time);
  EEPROM.put(sizeof(time_t), 480); // 480 Pulse/L calculated from datasheet
  *********************************************************************/

  /* Power management */
  power_adc_disable();
  power_spi_disable();
  power_timer1_disable();
  power_timer2_disable();

  /* Setup the Watchdog Timer */
  wdt_enable(WDTO_8S);

  /* Setup LCD display */
  display.begin(); // init done
  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  delay(1000);

  /* Set time and RTC options */
  while (RTC.get() == 0);
  setTime(RTC.get());   // the function to get the time from the RTC

  /* Set RTC alarm to wake-up microcontroller every minute
   * Alarm pin of RTC is attached to Pin 3 (Flowmeter)  */
  RTC.squareWave(SQWAVE_NONE);

  RTC.alarm(ALARM_1);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.setAlarm(ALM2_EVERY_MINUTE, 1, 1, 1, 1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_2, true);


  /* Set pin configuration for buttons */
  pinMode(pinEnterBtn, INPUT);   //external pull-up resistor required due to interrupt function!!
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);
  flow.resetFlowMeter();

  /* Read giessTime from EEPROM */
  int eeAdress = 0;   // EEPROM-Adress
  EEPROM.get(eeAdress, giess.time);
  eeAdress += sizeof(time_t); // Set position to calibration factor

  /* Initialize GiessFlag */
  giess.flag = CTRL_IDLE;

  /* Read calibration factor from EEPROM */
  int cf;
  EEPROM.get(eeAdress, cf);
  eeAdress += sizeof(int); // Set position to volume targets (after cf)
  flow.setCalibrationFactor(cf);

  /* Read volume targets from EEPROM */
  int vol;
  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, vol);  //
    eeAdress += sizeof(int); // Set position to next vol target
    valve[i].setVolumeTarget(vol);
  }


  /* If Enter is pressed at boot, enter calibration mode */
  if (digitalRead(pinEnterBtn) == LOW) {
    display.clearDisplay();
    display.display();
    delay(2000);
    calibration();
  }

  statusDisplay(-1, -1);
  }

void loop() {

  /* Enter sleep mode */
  if(giess.flag == CTRL_IDLE) {
    statusDisplay(CTRL_SLEEP, -1);
    wdt_disable();
    enterSleep();
    wdt_enable(WDTO_8S);
    setTime(RTC.get());
    statusDisplay(CTRL_IDLE, -1);
  }

  if (alarmIsrWasCalled) {
    RTC.alarm(ALARM_2);
    alarmIsrWasCalled = false;
  }

  /* Check if it is time for giessing */
  if (giess.flag == CTRL_IDLE) {
    giess.flag = checkGiessen();
  }

  /* Update Display every DISPLAY_UPDATE ms*/
  if ((millis() - giess.lastCall > DISPLAY_UPDATE)) {
    statusDisplay(giess.flag, -1);
    giess.lastCall = millis();
  }

  /* If giessing, then check progress */
  if (giess.flag > CTRL_IDLE) {   // that is active channel 0, 1, 2, or 3
    giessRoutine();
  }

  /* On enter btn press, start mode to set time, giessTime and target volumes */
  if (digitalRead(pinEnterBtn) == 0) {
    setParameters();
  }

  wdt_reset();
}

int checkGiessen() {
  /* Check if time for giessen */
  if ((hour(giess.time) == hour()) && (minute(giess.time) == minute())) {
    flow.resetFlowMeter();
    RTC.alarmInterrupt(ALARM_2, false);
    return CTRL_ACT;
  }
  return CTRL_IDLE;
}

void giessRoutine() {
  /* Giessen routines */
  if (pump.start()) delay(2000); // wait for 2s if pump had to be started

  valve[giess.flag].setCurrentVolume(flow.getVolume());

  if (valve[giess.flag].dosing() == 0) {
    flow.resetFlowMeter();
    giess.flag++; delay(100);
  }

  if (giess.flag > CHANNEL-1) {
    pump.stop();
    giess.flag = CTRL_IDLE;
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_2, true);
  }
}

void statusDisplay(int gf, int ch) {

  static byte blink_flag = 1;
  display.clearDisplay();
  display.setCursor(0, 0);

  /* Display if nothing is active or parameter set mode */
  if (gf < 0) {   // -1 or -2
    if (gf == -2 && ch == -2) display.setTextColor(WHITE, BLACK);

    /* Print current time */
    display.print(hour());
    display.print(":");
    if(minute() < 10) display.print('0');
    display.print(minute());
    display.setTextColor(BLACK, WHITE);

    /* Print giess time */
    display.setCursor(40, 0);
    if (gf == -3) {
      display.print("S>");
      } else {
        display.print(">>");
      }
    if (gf == -2 && ch == -1) display.setTextColor(WHITE, BLACK);
    display.print(hour(giess.time)); display.print(":");
    if(minute(giess.time) < 10) display.print('0');
    display.print(minute(giess.time));
    display.setTextColor(BLACK, WHITE);

    /* Print channel information */
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

    display.setTextColor(WHITE, BLACK);

    /* Blink Wasser */
    if (blink_flag++ < 4) display.print(" WASSER ");
    blink_flag %= 6;
    display.setTextColor(BLACK, WHITE);

    /* Print pulse count */
    //display.println(flow.getPulseCount());

    display.setCursor(50, 0);
    if (valve[gf].readCurrentVolume() < 100) display.print(" ");
    if (valve[gf].readCurrentVolume() < 10) display.print(" ");
    display.print(valve[gf].readCurrentVolume());
    display.print(" L");



    for (int i = 0; i < CHANNEL; i++) {
    /* Display active channel */
      if (i == gf)
      {
        display.setTextColor(WHITE, BLACK);
        display.setCursor(0, (8*i+15));
        display.print(valve[i].getPlant());
        display.print("   ");
        display.setCursor(50, (8*i+15));
        if (valve[i].readVolumeTarget()<100) display.print(" ");
        if (valve[i].readVolumeTarget()<10) display.print(" ");
        display.print(valve[i].readVolumeTarget());
        display.println(" L");
        display.setTextColor(BLACK, WHITE);

      }
      else
      /* Display idle channel */
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

  wdt_disable();

  /* Show instructions on display */
  display.clearDisplay();
  display.setCursor(0,0);
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

  wdt_enable(WDTO_8S);
}

void calibrationDoseDisplay(int vol) {
  display.clearDisplay();
  display.setCursor(0,0);
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
  display.println("Volumen ein-");
  display.println("stellen und ");
  display.println("bestaetigen");
  display.println();
  display.print(vol, 1); display.println(" kg");
  display.display();
}

void setParameters() {

  RTC.alarmInterrupt(ALARM_2, false);

  int channel = 0;
  long lastActivity;

  delay(2 *  btnDelay);
  lastActivity = millis();

  /* Set target volumes */
  while (channel < CHANNEL) {
    /* Choose channel */
    if (digitalRead(pinEnterBtn) == 0) {
      channel++;
      delay(btnDelay);
      lastActivity = millis();
      wdt_reset();
    }
    /* Adjust volume */
    if (digitalRead(pinUpBtn) == 0) {
      valve[channel].incVolumeTarget(1);
      statusDisplay(-2, channel);
      delay(btnDelay);
      lastActivity = millis();
      wdt_reset();
    }
    if (digitalRead(pinDownBtn) == 0) {
      valve[channel].incVolumeTarget(-1);
      statusDisplay(-2, channel);
      delay(btnDelay);
      lastActivity = millis();
      wdt_reset();
    }

    statusDisplay(-2, channel);

    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }

    wdt_reset();
  }

  delay(btnDelay);
  lastActivity = millis();

  /* Set Clock */
  while (digitalRead(pinEnterBtn)) {

    if (digitalRead(pinUpBtn) == 0) {
      adjustTime(60);  // Function of time library. Adds given seconds to time.
      delay(btnDelay/4);
      lastActivity = millis();
      wdt_reset();
    }

    if (digitalRead(pinDownBtn) == 0) {
      adjustTime(-60);  // Function of time library. Adds given seconds to time.
      delay(btnDelay/4);
      lastActivity = millis();
      wdt_reset();
    }

    statusDisplay(-2, -2);

    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
  }

  delay(btnDelay);
  lastActivity = millis();
  wdt_reset();

  /* Set Timer */
  while (digitalRead(pinEnterBtn)) {

    if (digitalRead(pinUpBtn) == 0) {
      giess.time += 60;
      lastActivity = millis();
      wdt_reset();
      delay(btnDelay/2);
    }

    if (digitalRead(pinDownBtn) == 0) {
      giess.time -= 60;
      lastActivity = millis();
      wdt_reset();
      delay(btnDelay/2);
    }

    statusDisplay(-2, -1);

    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
  }

  delay(btnDelay);
  wdt_reset();
  writeParameters();

}

void writeParameters() {

  /* Write new giessTime to EEPROM */
  int eeAdress = 0;
  EEPROM.put(eeAdress, giess.time);

  /* Write new volume targets to EEPROM */
  eeAdress += (sizeof(time_t) + sizeof(int));
  int i;
  for (i = 0; i < CHANNEL; i++) {
    EEPROM.put(eeAdress, valve[i].readVolumeTarget());
    eeAdress += sizeof(int);
  }

  /* Update RTC */
  RTC.set(now());

  /* Show normal display */
  statusDisplay(CTRL_IDLE, -1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_2, true);
  wdt_reset();

}

void btnInterruptSleep(void) {
  /* This will bring us back from sleep. */
    alarmIsrWasCalled = true;
}

void enterSleep(void) {
  /* Set-up pin2 as an interrupt and attach handler. */
  detachInterrupt(digitalPinToInterrupt(flow.getPin()));  // detach flow-meter interrupt
  attachInterrupt(digitalPinToInterrupt(pinEnterBtn), btnInterruptSleep, FALLING);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  sleep_mode();
  /* The program will continue from here. */

  if (alarmIsrWasCalled) {
    detachInterrupt(0);
    sleep_disable();
  }

}
