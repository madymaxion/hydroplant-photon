// This #include statement was automatically added by the Spark IDE.
#include "Tinker.h"

// This #include statement was automatically added by the Spark IDE.
#include "elapsedMillis.h"

#include "flashee-eeprom.h"
 using namespace Flashee;


elapsedMillis timeElapsed; //declare global if you don't want it reset every time loop runs


//Initial values & storage address for eeprom saving Tresholds
FlashDevice* flash;
int eepromW = 0;
int eepromH = 10;
int threshW = 0;
int threshH = 0;


//Analog Pins
int led = D7; //blue default led
int soilPin = A0;
int waterPin = A1;




//values to store analog sensor input, variables are avaible via particle cloud
int waterLevel = 0;
int humidityLevel = 0;


// state of the LED = LOW is off, HIGH is on
//delay in milliseconds between blinks of the LED
unsigned int interval = 1000;
boolean ledState = LOW;

SYSTEM_MODE(SEMI_AUTOMATIC);

int myConnect = 0;      // means neither connected or disconnected yet
int i = 0;

void setup() {
  //store variables
  flash = Devices::createDefaultStore();

  pinMode(led, OUTPUT);
  pinMode(A0, INPUT);

	//Register all the Tinker functions
	Spark.function("digitalread", tinkerDigitalRead);
	Spark.function("digitalwrite", tinkerDigitalWrite);
	Spark.function("analogread", tinkerAnalogRead);
	Spark.function("analogwrite", tinkerAnalogWrite);

  //connect with particle cloud
  Spark.connect();

  //public accesible variable
  Spark.variable("waterLevel", &waterLevel, INT);
  Spark.variable("humidityLevel", &humidityLevel, INT);

  //events testing
  Serial.begin(9600);
  Spark.subscribe("cWater", waterHandler);
  Spark.subscribe("cHumidity", humidityHandler);

  delay(5000);

  //inital values from eeprom
  flash->read(threshW, eepromW);
  Serial.print("Water Treshold: ");
  Serial.println(threshW);
  flash->read(threshH, eepromH);
  Serial.print("Humidity Treshold: ");
  Serial.println(threshH, eepromH);

}

void setRGBColor(const int r=255, const int g=255, const int b=255, const int brightness=255) {
  RGB.control(true);
  RGB.color(r,g,b);
  RGB.brightness(brightness);
}


void loop() {
    //if (!WiFi.listening()) {
      /*
      if (timeElapsed > interval)
        {
            ledState = !ledState;         // toggle the state from HIGH to LOW to HIGH to LOW ...
            digitalWrite(led, ledState);
            timeElapsed = 0;              // reset the counter to 0 so the counting starts over...
        }
      */
    //}


    bool connected = Spark.connected();

    if (!connected)
    {
        // only put code here that is fine with no wifi
        setRGBColor(255,0,255);

        if (!WiFi.hasCredentials()) { //if there is no SSID set
           if (!WiFi.listening()) { //if not yet in listening mode
                WiFi.listen(); //enter config mode
                RGB.control(false); //default blue blinking of particle photon user mode
            }
        }

        setRGBColor(255,0,255,0);
        delay(1000);

    }


    //read values from eeprom every second, causes a delay but saves flash
    if (timeElapsed > interval)
    {
      flash->read(threshW, eepromW);
      Serial.print("Water Treshold: ");
      Serial.println(threshW);
      flash->read(threshH, eepromH);
      Serial.print("Humidity Treshold: ");
      Serial.println(threshH, eepromH);
      timeElapsed = 0;              // reset the counter to 0 so the counting starts over...
    }


    //read analog sensors
    waterLevel = analogRead(waterPin);
    humidityLevel = analogRead(soilPin);

    //check if there is water
    bool waterSupply = false;
    if (waterLevel > 20 ) {
      waterSupply = true;
    }
    else {

    }
    /*
    if (threshH > senseH) {
      //plant wants water

    }
    */

}




void waterHandler(const char *event, const char *data)
{
  if (data){
    writeEEPROM(0, atoi(data));
  }
  else {
    Serial.println("NULL");
  }
}

void humidityHandler(const char *event, const char *data)
{
  if (data){
    writeEEPROM(1, atoi(data));
  }
  else {
    Serial.println("NULL");
  }
}


bool writeEEPROM(const int mode, const int data)
{
  bool success;
  int storedVal;
  RGB.control(true);
  Serial.println(data);
  switch(mode)
  {
    case 0:
      flash->read(storedVal, eepromW);
      if (data != storedVal) {
        RGB.color(255,0,0);
        success = flash->write(data, eepromW);
        Serial.println("Write Mode 0");
        break;
      }
    case 1:
      flash->read(storedVal, eepromH);
      if (data != storedVal) {
        RGB.color(255,0,0);
        Serial.println("Write Mode 1");
        success = flash->write(data, eepromH);
        break;
      }
  }
  delay(2000);
  RGB.control(false);
  return success;
}
