


/***********************************************
*  Tomatron v1.0
*
*
***********************************************/

#include <Arduino.h>
#include <EEPROM.h>
#include <Time.h>
#include <avr/wdt.h>  // Watchdog-Timer
#include <avr/sleep.h>
#include <avr/power.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "water.h"

//---------------------------------------------------------
// System Setup
//---------------------------------------------------------

#define CHANNEL 6  // Number of water channels

// Giessflag
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

//---------------------------------------------------------
// Display
//---------------------------------------------------------
// Nokia 5110 Adafruit Library
// Software SPI (slower updates, more flexible pin options):
// pin 12 - Serial clock out (SCLK)
// pin 11 - Serial data out (DIN)
// pin 10 - Data/Command select (D/C)
// GND    - LCD chip select (CS)
// pin 9 - LCD reset (RST)

Adafruit_PCD8544 display = Adafruit_PCD8544(12, 11, 10, 9);
#define DISPLAY_UPDATE 250  // Display update rate (milliseconds)

//---------------------------------------------------------
// Buttons
//---------------------------------------------------------
const byte pinUpBtn =     0;   // Up-Button, ATmega pin 2
const byte pinDownBtn =   1;   // Down-Button, ATmega pin 3
const byte pinEnterBtn =  2;   // Enter-Button, Interupt 0, ATmega pin 4

unsigned int btnDelay =   200; // Debounce delay

//---------------------------------------------------------
// Objects
//---------------------------------------------------------
Flowmeter flow(3); // Interupt 1 -> Pin 3 must not be changed! // ATmega pin 5

Magnetvalves valve[CHANNEL] = {  // 8 characters max.
  Magnetvalves(5, "Tom1  "),  // ATmega pin 11
  Magnetvalves(6, "Tom2  "),  // ATmega pin 12
  Magnetvalves(7, "Tom3  "),  // ATmega pin 13
  Magnetvalves(8, "Pap1  "),  // ATmega pin 14
  Magnetvalves(A1,"Pap2  "),  // ATmega pin 24
  Magnetvalves(A2,"Gur1  ")   // ATmega pin 25
};

Pump pump(4); // ATmega pin 6

Thermocontrol thermo(8); // Daytime hour to start temperature averaging

volatile boolean alarmIsrWasCalled = true;

//---------------------------------------------------------
// Function prototypes
//---------------------------------------------------------

int   checkGiessen(void);
void  giessRoutine(void);
void  statusDisplay(int, int);

void  interruptPulse(void);

void  setParameters(void);
void  writeParameters();

void  btnInterruptSleep(void);
void  enterSleep(void);

void  calibration();
void  calibrationDoseDisplay(int);
void  calibrationDisplay(double);


//-------------------------------------------------------------------
// Setup
//-------------------------------------------------------------------
void setup()
{
/*** ONLY FOR INITIAL EEPROM AND RTC PROGRAMMING *******************
  // Giesstime
    setTime(18, 30, 00, 01, 05, 16); // hour, min, sec, day, month, year
    RTC.set(now());
    giess.time = now() + 3600;
    int eepromAdress = 0;
    EEPROM.put(0, giess.time);
    eepromAdress += sizeof(time_t);

  // Flow Calibration Factor
      EEPROM.put(eepromAdress, 480); // Datasheet says 480 Pulses/L
      eepromAdress += sizeof(int);

  // thermoCoeff
      float thermoCoeff = 1.5;
      EEPROM.put(eepromAdress, thermoCoeff);
********************************************************************/

  // Power management
  power_adc_disable();
  power_spi_disable();
  power_timer1_disable();
  power_timer2_disable();

  // Setup the Watchdog Timer
  wdt_enable(WDTO_8S);

  // Setup LCD display
  display.begin(); // init done
  display.setContrast(50);
  display.setRotation(2);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  delay(1000);

  // Set time and RTC options
  while (RTC.get() == 0);
  setTime(RTC.get());   // the function to get the time from the RTC

  /* Set RTC alarm to wake-up microcontroller every minute
  * Alarm pin of RTC is attached to Pin 2 (EnterButton)  */
  RTC.squareWave(SQWAVE_NONE);
  RTC.alarm(ALARM_1);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.setAlarm(ALM2_EVERY_MINUTE, 1, 1, 1, 1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_2, true);

  // Set pin configuration for buttons
  pinMode(pinEnterBtn, INPUT);   //external pull-up resistor required due to interrupt function!!
  pinMode(pinUpBtn, INPUT_PULLUP);
  pinMode(pinDownBtn, INPUT_PULLUP);
  flow.resetFlowMeter();

  // Read giessTime from EEPROM
  int eeAdress = 0;
  EEPROM.get(eeAdress, giess.time);
  eeAdress += sizeof(time_t); // Move write position to calibration factor

  // Initialize GiessFlag
  giess.flag = CTRL_IDLE;

  // Read calibration factor from EEPROM
  int cf;
  EEPROM.get(eeAdress, cf);
  flow.setCalibrationFactor(cf);
  eeAdress += sizeof(int); // Move write position to temperature factor

  // Read temperature factor factor from EEPROM
  float tf;
  EEPROM.get(eeAdress, tf);
  thermo.SetTempCoeff(tf);
  eeAdress += sizeof(float); // Move write position to volume targets

  // Read volume targets from EEPROM
  int vol;
  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, vol);  //
    eeAdress += sizeof(int); // Set position to next vol target
    valve[i].setVolumeTarget(vol);
  }

  // If Enter is pressed at boot, enter calibration mode
  if (digitalRead(pinEnterBtn) == LOW) {
    display.clearDisplay();
    display.display();
    delay(2000);
    calibration();
  }
  statusDisplay(-1, -1);
}


//--------------------------------------------------------------
// Function name: loop()
//--------------------------------------------------------------
void loop()
{
  // If nothing happens
  if(giess.flag == CTRL_IDLE)
  {
    // MCU enter sleep
    statusDisplay(CTRL_SLEEP, -1);
    wdt_disable();
    enterSleep();

    // Continue after wake-up
    wdt_enable(WDTO_8S); // Enable Watchdog-Timer
    setTime(RTC.get()); // Update MCU time from RTC
    if (hour() > (thermo.GetLowerAveragingHour()-1) &&
        (hour() < hour(giess.time) || (hour() == hour(giess.time) &&
          minute() < minute(giess.time))))
    {
      thermo.AddTempReading(RTC.temperature()/float(4.0)); // Temperature measurement
    }
    statusDisplay(CTRL_IDLE, -1);
  }

  // Reset RTC alarm
  if (alarmIsrWasCalled)
  {
    RTC.alarm(ALARM_2);
    alarmIsrWasCalled = false;
  }

  // Check if it is time for giessing */
  if (giess.flag == CTRL_IDLE)
  {
    giess.flag = checkGiessen();
  }

  // Update Display every DISPLAY_UPDATE ms*/
  if ((millis() - giess.lastCall > DISPLAY_UPDATE))
  {
    statusDisplay(giess.flag, -1);
    giess.lastCall = millis();
  }

  // If giessing, then check progress */
  if (giess.flag > CTRL_IDLE) // that is active channel 0, 1, 2, or 3
  {
    valve[giess.flag].setGiessFactor(thermo.GetGiessFactor());
    giessRoutine();
  }

  // On enter btn press, start mode to set time, giessTime and target volumes */
  if (digitalRead(pinEnterBtn) == 0)
  {
    RTC.alarmInterrupt(ALARM_2, false);
    setParameters();
    if (giess.flag == CTRL_IDLE)
    {
      RTC.alarm(ALARM_2);
      RTC.alarmInterrupt(ALARM_2, true);
    }
  }

  wdt_reset();
}


//--------------------------------------------------------------
// Function name: checkGiessen
//
// checks if it is time for giessen
//--------------------------------------------------------------
int checkGiessen()
{
  if ((hour(giess.time) == hour()) && (minute(giess.time) == minute()))
  {
    flow.resetFlowMeter();
    RTC.alarmInterrupt(ALARM_2, false);
    return CTRL_ACT;
  }
  return CTRL_IDLE;
}


//--------------------------------------------------------------
// Function name: giessRoutine
//--------------------------------------------------------------
void giessRoutine()
{
  if (pump.start()) delay(2000); // wait for 2s if pump had to be started

  valve[giess.flag].setCurrentVolume(flow.getVolume());

  if (valve[giess.flag].dosing() == 0)
  {
    flow.resetFlowMeter();
    giess.flag++; delay(100);
  }

  if (giess.flag > CHANNEL-1)
  {
    pump.stop();
    thermo.ResetAverage();
    giess.flag = CTRL_IDLE;
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_2, true);
  }
}


//--------------------------------------------------------------
// Status Display
//--------------------------------------------------------------
void statusDisplay(int gf, int ch)
{
  static byte blink_flag = 1;
  display.clearDisplay();
  display.setCursor(0, 0);

  //---------------------------------------------------
  // Display if nothing is active or parameter set mode

  if (gf < 0)    //  -1 or -2
  {
    if (gf == -2 && ch == -2) display.setTextColor(WHITE, BLACK);

    display.drawFastHLine(0, 9, 85, BLACK);
    display.drawFastHLine(41, 30, 43, BLACK);
    display.drawFastVLine(40, 0, 48, BLACK);

    // Print current time
    if(hour() < 10) display.print(' ');
    display.print(hour());
    display.print(":");
    if(minute() < 10) display.print('0');
    display.print(minute());
    display.setTextColor(BLACK, WHITE);

    // Print giessFactor
    display.setCursor(47, 33);
    if (gf == -2 && ch == -3)
    {
      display.setTextColor(WHITE, BLACK);
      display.print("f");
      display.print(thermo.GetTempCoeff(), 1);
      display.setTextColor(BLACK, WHITE);
    }
    else
    {
      display.print("x");
      display.print(thermo.GetGiessFactor(), 1);
    }

    // Print average temperature
    display.setCursor(47, 41);
    if (thermo.GetTempAverage() > 0)
    {
      display.print(thermo.GetTempAverage(), 1);
    }
    else
    {
      display.print("---");
    }
    display.print("C");

    // Print giess time
    display.setCursor(46, 0);
    if (gf == -3) {
      display.print("G");
    } else {
      display.print(">");
    }
    if (gf == -2 && ch == -1) display.setTextColor(WHITE, BLACK);
    if(hour(giess.time) < 10) display.print(' ');
    display.print(hour(giess.time));
    display.print(":");
    if(minute(giess.time) < 10) display.print('0');
    display.print(minute(giess.time));
    display.setTextColor(BLACK, WHITE);

    // Print channel left column
    for (int i = 0; i < 4; i++)
    {
      display.setCursor(0, (8*i + 13));
      if (gf == -2 && i == ch) display.setTextColor(WHITE, BLACK);
      display.print(valve[i].getPlantName());
      display.setCursor(26, (8*i + 13));
      if (valve[i].readVolumeTarget()<10) display.print(" ");
      display.println(valve[i].readVolumeTarget());
      display.setTextColor(BLACK, WHITE);
    }

    // Print channel right column
    for (int i = 4; i < CHANNEL; i++)
    {
      display.setCursor(44, (8*(i-4) + 13));
      if (gf == -2 && i == ch) display.setTextColor(WHITE, BLACK);
      display.print(valve[i].getPlantName());
      display.setCursor(70, (8 *(i-4) + 13));
      if (valve[i].readVolumeTarget()<10) display.print(" ");
      display.println(valve[i].readVolumeTarget());
      display.setTextColor(BLACK, WHITE);
    }
    display.display(); // show screen
    return;
  }

  //----------------------------------------------------
  // Display during active giessing

  if (gf > -1)
  {
    // Divide display in boxes
    display.drawFastHLine(0, 9, 85, BLACK);
    display.drawFastHLine(41, 30, 43, BLACK);
    display.drawFastVLine(40, 10, 38, BLACK);

    // Blinking Title "Wasser" (in top box)
    display.setTextColor(WHITE, BLACK);
    display.setCursor(17, 0);
    if (blink_flag++ < 4) display.print(" WASSER ");
    blink_flag %= 6;

    // Print current volume (in box bottom right)
    display.setTextColor(BLACK, WHITE);
    display.setCursor(45, 36);
    if (valve[gf].readCurrentVolume() < 10) display.print(" ");
    display.print(valve[gf].readCurrentVolume(), 1);
    display.print(" L");

    // Left column
    for (int i = 0; i < 4; i++)
    {
      if (i == gf) // Display active channel
      {
        display.setTextColor(WHITE, BLACK);
        display.setCursor(0, (8*i+13));
        display.print(valve[i].getPlantName());
        display.setCursor(26, (8*i+13));
        if (round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()) < 10) display.print(" ");
        display.println(round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()));
        display.setTextColor(BLACK, WHITE);
      }
      else // Display idle channel
      {
        display.setCursor(0, (8*i+13));
        display.print(valve[i].getPlantName());
        display.setCursor(26, (8*i+13));
        if (round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()) < 10) display.print(" ");
        display.println(round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()));
      }
    }

    // Right column
    for (int i = 4; i < CHANNEL; i++)
    {
      if (i == gf) // Display active channel
      {
        display.setTextColor(WHITE, BLACK);
        display.setCursor(44, (8*(i-4)+13));
        display.print(valve[i].getPlantName());
        display.setCursor(70, (8*(i-4)+13));
        if (round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()) < 10) display.print(" ");
        display.println(round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()));
        display.setTextColor(BLACK, WHITE);
      }
      else // Display idle channel
      {
        display.setCursor(44, (8*(i-4)+13));
        display.print(valve[i].getPlantName());
        display.setCursor(70, (8*(i-4)+13));
        if (round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()) < 10) display.print(" ");
        display.println(round(valve[i].readVolumeTarget() * thermo.GetGiessFactor()));
      }
    }
    display.display(); // show screen
    return;
  }
}


//--------------------------------------------------------------
// Function name: interuptPulse()
//--------------------------------------------------------------
void interuptPulse()
{
  flow.pulse();
}


//-------------------------------------------------------------
// Parameter Set Routine
//-------------------------------------------------------------
void setParameters()
{

  int channel = 0;
  long lastActivity;

  delay(2 *  btnDelay);
  lastActivity = millis();

  // Set target volumes
  while (channel < CHANNEL)
  {
    if (digitalRead(pinEnterBtn) == 0) // Choose next channel
    {
      channel++;
      delay(btnDelay);
      lastActivity = millis();
      wdt_reset();
    }

    if (digitalRead(pinUpBtn) == 0) // Increase volume
    {
      valve[channel].incVolumeTarget(1);
      statusDisplay(-2, channel);
      delay(btnDelay);
      lastActivity = millis();
      wdt_reset();
    }

    if (digitalRead(pinDownBtn) == 0) // Decrease volume
    {
      valve[channel].incVolumeTarget(-1);
      statusDisplay(-2, channel);
      delay(btnDelay);
      lastActivity = millis();
      wdt_reset();
    }

    statusDisplay(-2, channel);  // Update display

    // If not button was pressed for 5s save changes and return
    if (millis() - lastActivity > 5000)
    {
      writeParameters();
      return;
    }
    wdt_reset();
  }

  delay(btnDelay);
  lastActivity = millis();

  // Set Timer
  while (digitalRead(pinEnterBtn))
  {
    if (digitalRead(pinUpBtn) == 0)
    {
      giess.time += 60;
      lastActivity = millis();
      wdt_reset();
      delay(btnDelay/5);
    }

    if (digitalRead(pinDownBtn) == 0)
    {
      giess.time -= 60;
      lastActivity = millis();
      wdt_reset();
      delay(btnDelay/5);
    }

    statusDisplay(-2, -1);  // Update display

    // If not button was pressed for 5s save changes and return
    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
    wdt_reset();
  }

  delay(btnDelay);
  lastActivity = millis();
  wdt_reset();

  // Set ThermoCoeff
  while (digitalRead(pinEnterBtn))
  {
    if (digitalRead(pinUpBtn) == 0)
    {
      thermo.IncTempCoeff(0.1);
      lastActivity = millis();
      wdt_reset();
      delay(btnDelay/5);
    }

    if (digitalRead(pinDownBtn) == 0)
    {
      thermo.IncTempCoeff(-0.1);
      lastActivity = millis();
      wdt_reset();
      delay(btnDelay/5);
    }

    statusDisplay(-2, -3);   // Update display

    // If not button was pressed for 5s save changes and return
    if (millis() - lastActivity > 5000) {
      writeParameters();
      return;
    }
    wdt_reset();
  }

  delay(btnDelay);
  lastActivity = millis();
  wdt_reset();

  // Set Daytime
  while (digitalRead(pinEnterBtn))
  {
    if (digitalRead(pinUpBtn) == 0)
    {
      adjustTime(60);  // Function of time library adds seconds
      delay(btnDelay/5);
      lastActivity = millis();
      wdt_reset();
    }

    if (digitalRead(pinDownBtn) == 0)
    {
      adjustTime(-60);  // Function of time library adds seconds
      delay(btnDelay/5);
      lastActivity = millis();
      wdt_reset();
    }

    statusDisplay(-2, -2); // Update display

    // If not button was pressed for 5s save changes and return
    if (millis() - lastActivity > 5000)
    {
      writeParameters();
      return;
    }
    wdt_reset();
  }

  // If Enter button was pressed during daytime setting
  delay(2*btnDelay);
  wdt_reset();
  writeParameters();
}

//---------------------------------------------------------------------
// void writeParameters(void)
// Writes parameters from setParameters() function to EEPROM or RTC
//---------------------------------------------------------------------
void writeParameters()
{
  // Write new giessTime to EEPROM
  int eeAdress = 0;
  EEPROM.put(eeAdress, giess.time);
  eeAdress += (sizeof(time_t) + sizeof(int));

  // Write new TempCoeff to EEPROM
  EEPROM.put(eeAdress, thermo.GetTempCoeff());
  eeAdress += sizeof(float);

  // Write new volume targets to EEPROM
  int i;
  for (i = 0; i < CHANNEL; i++)
  {
    EEPROM.put(eeAdress, valve[i].readVolumeTarget());
    eeAdress += sizeof(int);
  }

  // Write new daytime to RTC
  RTC.set(now());

  // Show normal display
  statusDisplay(CTRL_IDLE, -1);
  wdt_reset();
}

//---------------------------------------------------------------------
// void btnInterruptSleep(void)
// Interrupt handler for RTC alarm
//---------------------------------------------------------------------
void btnInterruptSleep(void) // Back from sleep
{
  alarmIsrWasCalled = true;
}

//---------------------------------------------------------------------
// void enterSleep(void)
//
//---------------------------------------------------------------------
void enterSleep(void)
{
  // Detach flow-meter pin from interrupt
  detachInterrupt(digitalPinToInterrupt(flow.getPin()));

  // Set pin2 (connected to Enter Button and RTC alarm) as interrupt
  // and attach handler function btnInterruptSleep()
  attachInterrupt(digitalPinToInterrupt(pinEnterBtn), btnInterruptSleep, FALLING);

  // Set sleep properties
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Let MCU go to sleep
  sleep_mode();

  // ** After interrupt The program will continue from here
  if (alarmIsrWasCalled)
  {
    detachInterrupt(0);
    sleep_disable();
  }
}


//--------------------------------------------------------------
// Function name: calibration()
//--------------------------------------------------------------
void calibration()
{
  int cf; // calibration factor
  double vol = 10.0; // Volume dispensed for calibration
  int eeAdress = sizeof(time_t);

  wdt_disable();

  // Show instructions on display
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Kalibierung");
  display.println("Taste drucken");
  display.println("10L aus V1:");
  display.println("Genaue Menge");
  display.println("auswiegen!");
  display.display();
  flow.resetFlowMeter();

  while (digitalRead(pinEnterBtn)) {};

  // Dispense volume vol
  pump.start();
  while (valve[0].dosing(int (vol)) > 0)
  {
    valve[0].setCurrentVolume(flow.getVolume());
    calibrationDoseDisplay(valve[0].readCurrentVolume());
  }
  pump.stop();
  // Adjust exact volume with Up/Down-Buttons and press Enter
  calibrationDisplay(double(valve[0].readCurrentVolume()));
  vol = double(valve[0].readCurrentVolume());
  while (digitalRead(pinEnterBtn))
  {
    if (digitalRead(pinUpBtn) == 0)
    {
      vol += 0.1;
      calibrationDisplay(vol);
      delay(btnDelay);
    }
    if (digitalRead(pinDownBtn) == 0)
    {
      vol -= 0.1;
      calibrationDisplay(vol);
      delay(btnDelay);
    }
  }
  delay(2 * btnDelay);

  // Calculate calibration factor and write to EEPROM
  cf = flow.getPulseCount() / vol;
  flow.setCalibrationFactor(cf);
  EEPROM.put(eeAdress, cf);

  wdt_enable(WDTO_8S);
}


//--------------------------------------------------------------
// Function name: calibrationDoseDisplay()
//--------------------------------------------------------------
void calibrationDoseDisplay(int vol)
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Dosierung");
  display.println("");
  display.println("");
  display.println();
  display.print(vol, 1); display.println(" kg");
  display.display();
}

void calibrationDisplay(double vol)
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Volumen ein-");
  display.println("stellen und ");
  display.println("bestaetigen");
  display.println();
  display.print(vol, 1); display.println(" kg");
  display.display();
}
