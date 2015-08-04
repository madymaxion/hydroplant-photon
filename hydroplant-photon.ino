// This #include statement was automatically added by the Spark IDE.
#include "Tinker.h"

// This #include statement was automatically added by the Spark IDE.
#include "elapsedMillis.h"

#include "flashee-eeprom.h"
 using namespace Flashee;


 //DEBUG SETTINGS
 //#define DEBUG //enable/disable serial debug output

 #ifdef DEBUG
   #define DEBUG_PRINT(x)     Serial.print(x)
   #define DEBUG_PRINTDEC(x)     Serial.print(x, DEC)
   #define DEBUG_PRINTLN(x)  Serial.println(x)
 #else
   #define DEBUG_PRINT(x)
   #define DEBUG_PRINTDEC(x)
   #define DEBUG_PRINTLN(x)
 #endif



elapsedMillis timeElapsed; //declare global if you don't want it reset every time loop runs
elapsedMillis eepromTimeElapsed; //cycle for offline eeprom read
elapsedMillis notificationElapsed; //cycle for offline eeprom read
unsigned long notificationInterval = 60000; //1min

int led = D7;
bool ledState = LOW;

//Initial values & storage address for eeprom saving Tresholds
FlashDevice* flash;
int eepromW = 0;
int eepromH = 10;
int pumpDuration = 0; //seconds
unsigned int pumpDurationMs = 0;
int threshH = 0;
int eepromReadInterval = 5000;

//Analog Pins
int soilPin = A0;
int waterPin = A1;




//values to store analog sensor input, variables are avaible via particle cloud, set with defaults, overwritten by stored values from eeprom
int waterLevel = 0;
int humidityLevel = 0;


bool waterSupply = false; //is there water?
unsigned currentlyPumping=0;

// state of the LED = LOW is off, HIGH is on
//delay in milliseconds between blinks of the LED
unsigned int blinkinterval = 250;
unsigned int pulseinterval = 2000;
int pulseBrightness = 0;
int pulseDirection = 1;

//start photon in semi-automatic mode, used for first wireless setup by user
SYSTEM_MODE(SEMI_AUTOMATIC);


//controll via function (ios/android)
int remoteControl(String command);

void setup() {
  //Serial Output for debug
  #ifdef DEBUG
    Serial.begin(9600); //serial debug
  #endif

  //store variables
  flash = Devices::createDefaultStore();
  //Devices::userFlash().eraseAll();  //delete user flash stored values


  pinMode(led, OUTPUT);
  pinMode(A0, INPUT);

	//Register all the Tinker functions
	//Spark.function("digitalread", tinkerDigitalRead);
	//Spark.function("digitalwrite", tinkerDigitalWrite);
	//Spark.function("analogread", tinkerAnalogRead);
	//Spark.function("analogwrite", tinkerAnalogWrite);




  //connect with particle cloud
  Spark.connect();
  delay(5000); //wait for connection
  if (Spark.connected()){
    Spark.publish("hydroplant-alert", "Hydroplant connected & booting up", 60, PRIVATE);
  }

  //callable function, remote controling tresholds
  Spark.function("rControl", remoteControl);

  //public accesible variable
  Spark.variable("waterLevel", &waterLevel, INT);
  Spark.variable("humidityLevel", &humidityLevel, INT);

  //inital values from eeprom, if none yet set store defaults
  flash->read(pumpDuration, eepromW);
  flash->read(threshH, eepromH);

  if (pumpDuration <= 0 || pumpDuration > 1024) {
    pumpDuration = 20; //seconds
    //store in eeprom
    writeEEPROM(0, pumpDuration);  //in case of no internet

  }
  if (threshH <= 0 || threshH > 1024) {
    threshH = 512;
    //store in eeprom
    writeEEPROM(1, threshH); //in case of no internet
  }

  DEBUG_PRINT("Water Pump Duration (s): ");
  DEBUG_PRINTLN(pumpDuration);
  DEBUG_PRINT("Humidity Treshold: ");
  DEBUG_PRINTLN(threshH);

}

void setRGBColor(const int r=255, const int g=255, const int b=255, const int brightness=255) {
  RGB.control(true);
  RGB.color(r,g,b);
  RGB.brightness(brightness);
}

void pulsateLed(int r=0, int g=0, int b=0, int minBrighness=20, int interval=1000)
{
    if (timeElapsed > interval)
  	{
      ledState =!ledState;		 // toggle the state from HIGH to LOW to HIGH to LOW ...
  		if (ledState)
      {
        pulseDirection = 1;
      }
      else {
        pulseDirection = -1;
      }
  		timeElapsed = 0;			 // reset the counter to 0 so the counting starts over...
  	}
    pulseBrightness = pulseBrightness+pulseDirection;
    setRGBColor(r,g,b,map(pulseBrightness, 0, interval, minBrighness, 255));
}

void blinkLed(int r=0, int g=0, int b=0, int interval=1000)
{

  if (timeElapsed > interval)
	{
		ledState =!ledState;		 // toggle the state from HIGH to LOW to HIGH to LOW ...
		if (ledState)
    {
      setRGBColor(r,g,b,255);
    }
    else {
      setRGBColor(r,g,b,0);
    }
    //digitalWrite(led, ledState);
		timeElapsed = 0;			 // reset the counter to 0 so the counting starts over...
	}

}



//main program
void loop() {

    bool connected = Spark.connected();

    if (!connected)
    {
        // only put code here that is fine with no wifi
        blinkLed(255,0,0,500);

        if (!WiFi.hasCredentials()) { //if there is no SSID set
           if (!WiFi.listening()) { //if not yet in listening mode
                WiFi.listen(); //enter config mode
                RGB.control(false); //default blue blinking of particle photon user mode
            }
        }


        //read previous tresholds from eeprom
        if (eepromTimeElapsed > eepromReadInterval)
        {
          flash->read(pumpDuration, eepromW);
          flash->read(threshH, eepromH);
          eepromTimeElapsed = 0;			 // reset the counter to 0 so the counting starts over...
        }
    }
    else {
      RGB.control(false);
    }


    //read analog sensors
    waterLevel = analogRead(waterPin);
    humidityLevel = analogRead(soilPin);

    //check if there is water

    if (waterLevel > 20 ) { //if there is water

      if(humidityLevel<threshH){ //check if water is needed

        pumpDurationMs = pumpDuration*1000; //in ms
        while(currentlyPumping < pumpDurationMs) { //pump water as long there is water
          waterLevel = analogRead(waterPin);
          if (waterLevel>0){
            pump(1); //run pump
          }
          else {
            pump(0); //stop pump
            break;
          }
          currentlyPumping++;
        }
        currentlyPumping=0;
      }
      else {
        //gotosleep
      }

    }
    else { //NO WATER!!!
      //send notification
    }






    //output to console

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("CUSTOM SENSOR VALUES------------------");
    DEBUG_PRINT("Soil humidity threshold: ");
    DEBUG_PRINTDEC(threshH);
    DEBUG_PRINTLN("");
    DEBUG_PRINT("Soil Sensor Value: ");
    DEBUG_PRINTDEC(humidityLevel);
    DEBUG_PRINTLN("");
    DEBUG_PRINT("Water Sensor Value: ");
    DEBUG_PRINTDEC(waterLevel);
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("");



    //delay(1000); //debug

}


void pump(int mode)
{
  switch(mode)
  {
    case 0:
      RGB.control(false);
      break;
    case 1:
      pulsateLed(0,0,255,10,100);
      break;
    default:
      RGB.control(false);
      break;
  }
}





//general function to convert strings to array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0,-1};
  int maxIndex = data.length()-1;
  for (int i=0; i<=maxIndex && found<=index; i++)
  {
    if(data.charAt(i)==separator || i==maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//controll via function (ios/android)
int remoteControl(String command)
{

  int rControls[2];
  rControls[0] = atoi(getValue(command, ',', 0));
  rControls[1] = atoi(getValue(command, ',', 1));

  //store in global variable
  pumpDuration = rControls[0];
  threshH = rControls[1];

  //store in eeprom
  writeEEPROM(0, rControls[0]);
  writeEEPROM(1, rControls[1]);


  return 1;
}


//write conroll data to eeprom, in case of no internet connection
bool writeEEPROM(const int mode, const int data)
{
  bool success;
  int storedVal;

  DEBUG_PRINT("data to write: ");
  DEBUG_PRINTLN(data);

  switch(mode)
    {
      case 0:
        flash->read(storedVal, eepromW);
        if (data != storedVal) {
          setRGBColor(0,255,255,255);
          success = flash->write(data, eepromW);
          if (success){
            DEBUG_PRINTLN("Write Mode 0 = OK!");
          }
        }
        break;
      case 1:
        flash->read(storedVal, eepromH);
        if (data != storedVal) {
          setRGBColor(0,255,255,255);
          success = flash->write(data, eepromH);
          if (success){
            DEBUG_PRINTLN("Write Mode 1 = OK!");
          }
        }
        break;
    }

  delay(500);
  RGB.control(false);
  return success;
}
