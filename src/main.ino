/***********************************************
*   Tomatron v0.1
***********************************************/

#include <Arduino.h>
##include "5110.h"

#define CHANNEL 3  //Anzahl Gieskanäle

typedef struct {


  int volume_soll[CHANNEL];
  int volume_ist[CHANNEL];
  int flow_current[CHANNEL];
} sWater;

/* Uhrzeit */


/* Calibration factor for flow-meter = (Pulses / Liter) */
int calibration_factor;


void setup() {

  if (/* Reset button */) {
    void calibration;
  }

  /* Read EEPROM values
    /   - Calibration factor
    /   - Volume_soll[CHANNEL]
    */





  /******* Setup Display  *******/

  display.begin(); // init done

  // you can change the contrast around to adapt the display for the best viewing!
  display.setContrast(50);

  display.display(); // show splashscreen
  delay(1000);
  display.clearDisplay();   // clears the screen and buffer
  display.display(); // show splashscreen

}


void loop() {
  /* Einstellung Wassermenge pro Kanal */



  /* */



}


/* Ansteuerung Magnetventile */


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
