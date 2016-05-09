/***********************************************
*   Tomatron v0.1
***********************************************/

#include <Arduino.h>
#include "5110.h"
#include <EEPROM.h>
//#include <HardwareSerial.h>


#define CHANNEL 3  //Anzahl Gieskanäle
#define SPEZIALBUTTON0 B01000000   // Arduino-Pin 6  an Arduino-Port D
#define SPEZIALBUTTON1 B10000000   // Arduino-Pin 7  an Arduino-Port D

typedef struct {
  int volume_soll[CHANNEL];
  int volume_ist[CHANNEL];
  int flow_current[CHANNEL];
} sWater;
sWater channel[CHANNEL];

/* Uhrzeit */


/* Calibration factor for flow-meter = (Pulses / Liter) */
int calibration_factor;


void setup() {

  /* Setup Display */
  display.begin(); // init done
  display.setContrast(50); // you can change the contrast around to adapt the display for the best viewing!
  display.display(); // show splashscreen
  delay(1000);
  display.clearDisplay();   // clears the screen and buffer
  display.display();

  /* Setup Serial for Debug */
  Serial.begin(9600);
  Serial.println("Starte Setup");

  /* Arduino-Pins für Spezial-Tasten konfigurieren (Konfiguration als Input und ggfs. Pull-Up Widerstände*/
  DDRD = DDRD & ~(SPEZIALBUTTON0 | SPEZIALBUTTON1); // Arduino-Pin 6 (Einstell-Taste) und 7 (Shift-Taste) als Input (Bit auf 0) definieren (und andere Pin unangetastet lassen)
  PORTD = PORTD | (SPEZIALBUTTON0 | SPEZIALBUTTON1); // Pull-Up aktivieren (Bit auf 1)

  /* EEPROM Lesen für Konfigurationsdaten */
  int eeAdress = 0;
  EEPROM.get(eeAdress, calibration_factor);  // Load calibration factor
  eeAdress += eeAdress + sizeof(int);

  for (byte i = 0; i < (CHANNEL); i++) {
    EEPROM.get(eeAdress, channel[i]);  // Load channel structs
    eeAdress += sizeof(sWater);
  }

  /* Einstieg in Kalibrierungs-Modus für Flow-Meter       */
  /* beim Anschalten mit gedrückter Taste                 */
  if (PIND & SPEZIALBUTTON0) {  // wenn Arduino-Pin 7 gedrückt (auf LOW)
    void calibration();
  }

}



void loop() {
  /* Einstellung Wassermenge pro Kanal */



  /* */



}


/* Ansteuerung Magnetventile */
void Magnetventile() {


}


/* Ausgabe auf LCD-Display */
void Display() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK); // 'inverted' text
  display.setCursor(0,0);

  display.println();

  display.setTextColor(BLACK);

  display.println();

  display.display(); // show splashscreen
}


/* Calibration of Flow Meter */
void calibration() {


}

int EepromWriteCalibration(){
  int eeAdress = 0;
  EEPROM.put(eeAdress, calibration_factor);
  eeAdress = 0;
  return 0;
}


int EepromWriteChannelStructs(int i){
  int eeAdress = 0 + sizeof(int) + i*sizeof(sWater); // "vorspulen"
  EEPROM.put(eeAdress, channel[i]);
  eeAdress = 0;
  return 0;
}
