#include <avr/wdt.h> //watchdog
#include <Http.h> //gprs
#include <Parser.h> //gprs
#include <Sim800.h> //gprs
#include <ArduinoJson.h> //parse server response
#include "LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include <OneWire.h> //tmp sensor
#include <DallasTemperature.h> //tmp sensor
#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define windVanePin (A3)       // The pin the wind vane sensor is connected to
OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sensor_air(&oneWire_in);
DallasTemperature sensor_water(&oneWire_out);
unsigned int RX_Pin = 9; //RX pin for sim800
unsigned int TX_Pin = 8; //TX pin for sim800
unsigned int RST_Pin = 7; //RST pin for sim800
unsigned int pwrAir=11; // power for air sensor
unsigned int pwrWater=12; // power for water sensor
byte resetReason = MCUSR;
// edit this data to suit your needs  ///////////////////////////////////////////////////////
#include "config.h"
//#define DEBUG // comment out if you want to turn offvdebugging
const char *bearer = "iot.1nce.net"; // APN address
const char *id = apiPassword; // get this unique ID in order to send data to vetercek.com
const char *webpage = "vetercek.com/xml/post.php"; // where POST request is made
int windDelay = 2; // time for each anemometer measurement in seconds
int onOffTmp = 0;   //on/off temperature measure
int whenSend = 3;     // after how many measurements to send data to server
// int vaneOffset=0; // now defined in config file for each station
//////////////////////////////////////////////////////////////////////////////////////////////

volatile unsigned long rotations; // cup rotation counter used in interrupt routine
volatile unsigned long contactBounceTime; // Timer to avoid contact bounce in interrupt routine
volatile unsigned long lastPulseMillis; // last anemometer measured time
volatile unsigned long firstPulseMillis; // fisrt anemometer measured time
volatile unsigned long currentMillis;
byte firstWindPulse; // ignore 1st anemometer rotation since it didn't make full circle
int windSpeed; // speed
long windAvr = 0; //sum of all wind speed between update
int windGust[3] = { 0, 0, 0 }; // top three gusts
int windGustAvg = 0; //wind gust average
int avrDir[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // array where wind direction are saved and later one with highest value is selected as dominant
int measureCount = 0; // count each mesurement
float water; // water Temperature
char wat[6]; // water char value
float temp; //Stores Temperature value
char tmp[6]; // Temperature char value
int vaneValue;       // raw analog value from wind vane
int direction;       // translated 0 - 360 direction
int calDirection;    // converted value with offset applied
int windDir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
unsigned int bat=0; // battery percentage
char response[60];
char body[160];
Result result;

HTTP http(9600, RX_Pin, TX_Pin, RST_Pin);
#define BODY_FORMAT "{\"id\":\"%s\",\"d\":\"%d\",\"s\":\"%d.%d\",\"g\":\"%d.%d\",\"t\":\"%s\",\"w\":\"%s\",\"b\":\"%d\",\"c\":\"%d\",\"r\":\"%d\" }"


// the setup routine runs once when you press reset:
void setup() {
  MCUSR = 0; // clear reset flags
  wdt_disable();
#ifdef DEBUG
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Starting!");
#endif

   pinMode(LED_BUILTIN, OUTPUT);     // this part is used when you bypass bootloader to signal when board is starting...
   digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
   delay(1000);                       // wait
   digitalWrite(LED_BUILTIN, LOW);    // turn the LED

   pinMode(pwrAir, OUTPUT);      // sets the digital pin as output
   pinMode(pwrWater, OUTPUT);      // sets the digital pin as output
   sensor_air.begin();
   sensor_water.begin();
   digitalWrite(pwrAir, LOW);   // turn off power
   digitalWrite(pwrWater, LOW);   // turn off power

}

// the loop routine runs over and over again forever:
void loop() {
  Anemometer();
  GetWindDirection();
  LowPower.powerDown(SLEEP_8S, ADC_ON, BOD_ON);

#ifdef DEBUG
    Serial.print("dir:");
    Serial.print(calDirection);
    Serial.print(" speed:");
    Serial.print(windSpeed);
    Serial.print(" gust:");
    Serial.print(windGustAvg);
    Serial.print(" next:");
    Serial.print(whenSend - measureCount);
    Serial.print(" count:");
    Serial.print(measureCount);
    Serial.println("");
#endif

  if (measureCount >= whenSend) { // check if is time to send data online
    SendData();
  }

}

void Anemometer() { //measure wind speed
  int actualWindDelay; //time between first and last measured anemometer rotation
  firstWindPulse=1; // dont count first rotation
  contactBounceTime = millis();
  rotations = 0; // Set rotations count to 0 ready for calculations
  EIFR = (1 << INTF0); // clear interrupt flag
  attachInterrupt(digitalPinToInterrupt(windSensorPin), ISRrotation, FALLING); //setup interrupt on anemometer input pin, interrupt will occur whenever falling edge is detected
  delay (windDelay * 1000); // Wait x second to average
  detachInterrupt(digitalPinToInterrupt(windSensorPin));

  if(rotations==0)  {
    windSpeed=0;  
  } 
  else  {   
    actualWindDelay=(lastPulseMillis-firstPulseMillis;
    windSpeed = rotations * (2250 / actualWindDelay) * 0.868976242 * 10 ; // convert to mp/h using the formula V=P(2.25/Time); 
    // 2250 instead of 2.25 because we need time in seconds not ms & * 0.868976242 to convert in knots  & *10 so we can calculate decimals later
    }  
  ++measureCount; // add +1 to counter
  windAvr += windSpeed; // add to sum of average wind values

  if (windSpeed > windGust[2]) { // check if > than old gust3 of wind
    windGust[0] = windGust[1];
    windGust[1] = windGust[2];
    windGust[2] = windSpeed;
  }

  else if (windSpeed > windGust[1]) { // check if > than old gust2 of wind
    windGust[0] = windGust[1];
    windGust[1] = windSpeed;
  }

  else if (windSpeed > windGust[0]) { // check if > than old gust1 of wind
    windGust[0] = windSpeed;
  }
  windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;
}

void ISRrotation () {  // This is the function that the interrupt calls to increment the rotation count
  currentMillis=millis(); //we have to read millis at the same position in ISR each time to get the most accurate readings
  if(firstWindPulse==1) { //discard first pulse as we don't know exactly when it happened
    contactBounceTime=currentMillis;
    firstWindPulse=0;
    firstPulseMillis=currentMillis;
  }
    else if ((currentMillis - contactBounceTime) > 15 ) { // debounce the switch contact.
      rotations++;
      contactBounceTime = currentMillis;
      lastPulseMillis=currentMillis;
    }
}




void DominantDirection() { // get dominant wind direction
  int maxIndex = 0;
  int max = avrDir[maxIndex];
  for (int i = 1; i < 16; i++) {
    if (max < avrDir[i]) {
      max = avrDir[i];
      windDir = i * 22; //this is just approximate calculation so server can return the right char value
    }
  }
}


void GetAir() {
  digitalWrite(pwrAir, HIGH);   // turn on power
  delay(500);
  sensor_air.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;
  temp = sensor_air.getTempCByIndex(0);
  if (temp > -100 && temp < 85) { dtostrf(temp, 4, 1, tmp); }   //float Tmp to char
  digitalWrite(pwrAir, LOW);   // turn off power
}

void GetWater() {
  digitalWrite(pwrWater, HIGH);   // turn on power
  delay(500);
  sensor_water.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;
  water = sensor_water.getTempCByIndex(0);
  if (water > -100 && water < 85) { dtostrf(water, 4, 1, wat); }  //float Tmp to char
  digitalWrite(pwrWater, LOW);   // turn off power
}


void GetAvgWInd() {
  wind_speed = windAvr / measureCount; // calculate average wind
}


// Get Wind direction, and split it in 16 parts and save it to array
void GetWindDirection() {
  vaneValue = analogRead(windVanePin);
  direction = map(vaneValue, 0, 1023, 0, 360);
  calDirection = direction + vaneOffset;

  if (calDirection > 360)
    calDirection = calDirection - 360;

  if (calDirection < 0)
    calDirection = calDirection + 360;

  calDirection = (calDirection + 11.25) / 22.5;
  if (calDirection < 16) {
    ++avrDir[calDirection];
  }
  else {
    ++avrDir[0];
  }

}



// send data to server
void SendData() {
  DominantDirection();
    #ifdef DEBUG
      Serial.println("direction done");
    #endif
  GetAvgWInd();
    #ifdef DEBUG
      Serial.println("wind done");
    #endif
  if (onOffTmp > 0) {
    GetAir();
    #ifdef DEBUG
      Serial.println("air done");
    #endif
    GetWater();
    #ifdef DEBUG
      Serial.println("water done");
    #endif
  delay(1000);
  }

  http.wakeUp();
  bat=http.readVoltagePercentage(); //battery percentage
    #ifdef DEBUG
      Serial.println(bat);
      Serial.println("battery done");
    #endif
  //signalq=http.readSignalStrength(); //signal quality


  http.configureBearer(bearer);
  result = http.connect();

 sprintf(body, BODY_FORMAT, id, windDir, wind_speed / 10, wind_speed % 10, windGustAvg / 10, windGustAvg % 10, tmp, wat, bat,measureCount,resetReason);

  #ifdef DEBUG
    Serial.println(body);
  #endif


  result = http.post(webpage, body, response);
  http.disconnect();
  http.sleep();
  delay(500);

  if (result == SUCCESS) {

    measureCount = 0;
    windAvr = 0;
    windGustAvg = 0;
    water = 0;
    temp = 0;
    memset(avrDir, 0, sizeof(avrDir)); // empty direction array
    memset(windGust, 0, sizeof(windGust)); // empty direction array
    memset(tmp, 0, sizeof(tmp));
    memset(wat, 0, sizeof(wat));

    StaticJsonDocument<200> doc;
    deserializeJson(doc, response);
    JsonObject root = doc.as<JsonObject>();

    int whenSend2 = root["w"];
    int offset = root["o"];
    int windDelay2 = root["wd"];
    int tt = root["tt"];


    if (whenSend2 != whenSend && whenSend2 > 0) { // server response to when to do next update
      whenSend = root["w"];
    }

    if (offset != vaneOffset && offset > -999) { // server sends wind wane position
      vaneOffset = root["o"];
    }

    if (windDelay2 != windDelay && windDelay2 > 0 ) { // interval for one wind measurement
      windDelay = root["wd"];
    }

    if (tt != onOffTmp && tt > -1) { // on/off tmp sensor
      onOffTmp = root["tt"];
      }
  }

}
