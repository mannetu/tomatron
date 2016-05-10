/***********************************************
*   Tomatron v0.1
***********************************************/

#include <Arduino.h>
#include "5110.h"
#include <EEPROM.h>
//#include <HardwareSerial.h>


#define CHANNEL 3  //Anzahl Gieskanäle
#define BUTTON0 B01000000   // Arduino-Pin 6  an Arduino-Port D
#define BUTTON1 B10000000   // Arduino-Pin 7  an Arduino-Port D
#define BUTTON2
#define BUTTON3

typedef struct {
  int volume_soll;
  int volume_ist;
  int flow_current;
} sWater;
sWater wasser[CHANNEL];

/* Uhrzeit */


/* Calibration factor for flow-meter = (Pulses / Liter) */
int calibration_factor;
int meterPulses;


void setup() {

  /* Setup Display */
  DisplaySetup();

  /* Setup Serial for Debug */
  Serial.begin(9600); Serial.println("Starte Setup");

  /*  Arduino-Pins für Tasten mit Pull-Up Widerstände konfigurieren */
  DDRD = DDRD & ~(BUTTON0 | BUTTON1); // Arduino-Pin 6 (Einstell-Taste) und 7 (Shift-Taste) als Input (Bit auf 0) definieren (und andere Pin unangetastet lassen)
  PORTD = PORTD | (BUTTON0 | BUTTON1); // Pull-Up aktivieren (Bit auf 1)

  /*  Konfigurationsdaten aus EEPROM auslesen                       */
  EepromReadSetup();

  /*  Einstieg in Kalibrierungs-Modus für Flow-Meter wenn Taste X   */
  /*  beim Einschalten gedrückt                                     */

  if (PIND & BUTTON0) {
    void calibration();
  }

}



void loop() {
  /* Interupt wird bei jedem Puls des FlowMeters ausgelöst */
  interrupts();

  /* Einstellung Wassermenge pro Kanal */
  if (PIND & BUTTON0) {  // wenn Arduino-Pin 7 gedrückt (auf LOW)

  }
  /* */



}


/* Ansteuerung Magnetventile */
void Magnetventil(int channel, int menge) {

  int pin;
  switch (channel) {
    case 0: pin = 1; break;
    case 1: pin = 2; break;
    case 2: pin = 3; break;
  }

  digitalWrite(pin, HIGH);
  while ((meterPulses / calibration_factor) < menge) {
    wasser[channel].volume_ist = meterPulses / calibration_factor;
    void Display();
    delay(1000);
  }

  digitalWrite(pin, LOW);

}


/* Ausgabe auf LCD-Display */
void Display() {

  display.clearDisplay();

  display.print("Tomatron v0.1   "); display.println("xx:xx"); //Uhrzeit
  display.println();
  display.print("Tomaten  "); display.print(wasser[1].volume_soll);
  display.print(" "); display.println(wasser[1].volume_ist);
  display.print("Paprika  "); display.print(wasser[2].volume_soll);
  display.print(" "); display.println(wasser[2].volume_ist);
  display.print("Kanal 3  "); display.print(wasser[3].volume_soll);
  display.print(" "); display.println(wasser[3].volume_ist);

  display.display(); // show screen
}


/* Calibration of Flow Meter */
void calibration() {
  display.clearDisplay();
  display.print("Tomatron v0.1   "); display.println("Calibration");
  display.println();
  display.println("Nach Tastendruck laufen");
  display.println("ca. 10L Wasser aus Schlauch 1.");
  display.println("Genaue Menge auswiegen!");
  display.display(); // show screen

  Magnetventil(0, 10);

  float exakteMenge = 10.0;
  DisplayCalibration(exakteMenge);


  /* Einstellung exakteMenge über Drehregler, Bestätigung durch Taste */

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

  calibration_factor = meterPulses / exakteMenge;

  EEPROM.put(0, calibration_factor);
  
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


int EepromSetVolume(int i){
  int eeAdress = 0 + sizeof(calibration_factor) + i*sizeof(int); // "vorspulen"
  EEPROM.put(eeAdress, wasser[i].volume_soll);
  eeAdress = 0;
  return 0;
}


void InteruptRoutine() {

  meterPulses++;

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

void EepromReadSetup() {
  int eeAdress = 0;
  EEPROM.get(eeAdress, calibration_factor);  // Load calibration factor
  eeAdress += eeAdress + sizeof(int);

  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, wasser[i].volume_soll);  //
    eeAdress += sizeof(int);
  }

}
