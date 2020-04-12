#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <OneWire.h> // Temperature sensor
#include <DallasTemperature.h> // Temperature sensor
#include "config.h"
#include <ArduinoJson.h> //parse server response
#include <math.h> // wind speed calculations
#include <DoubleResetDetect.h>
#define WindSensorPin (D2) // The pin location of the anemometer sensor
#define windVanePin (A0)       // The pin the wind vane sensor is connected to
#define ONE_WIRE_BUS (D1)
#define BODY_FORMAT "{\"id\": \"%s\", \"d\": \"%d\", \"s\": \"%d.%d\", \"g\": \"%d.%d\", \"t\": \"%s\", \"sig\": \"%d\"}"
#define DRD_TIMEOUT 5.0
#define DRD_ADDRESS 0x00
#define DEBUG // comment out if you want to turn off debugging
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define DRD_TIMEOUT 2.0
#define DRD_ADDRESS 0x00
DoubleResetDetect drd(DRD_TIMEOUT, DRD_ADDRESS);


const char *id=API_PASSWORD;  // get this unique ID in order to send data to vetercek.com
const char *webpage="http://vetercek.com/xml/post.php";  // where POST request is made
volatile unsigned long rotations; // cup rotation counter used in interrupt routine
volatile unsigned long contactBounceTime; // Timer to avoid contact bounce in interrupt routine
volatile unsigned long lastPulseMillis; // last anemometer measured time
volatile unsigned long firstPulseMillis; // fisrt anemometer measured time
volatile unsigned long currentMillis;
unsigned int pwrAir=(D3); // power for air sensor
byte firstWindPulse; // ignore 1st anemometer rotation since it didn't make full circle
int windSpeed; // speed
long windAvr=0; //sum of all wind speed between update
int windGust[3] = { 0, 0, 0 }; // top three gusts
int windGustAvg = 0; //wind gust average
int avrDir[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; // array where wind direction are saved and later one with highest value is selected as dominant
int measureCount=0; // count each mesurement
int VaneValue;       // raw analog value from wind vane
int Direction;       // translated 0 - 360 direction
// int vaneOffset=0; // now defined in config file for each station
int CalDirection;    // converted value with offset applied
int windDir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
float temp; //Stores Temperature value
char tmp[6]; // Temperature char value
char response[100];
char body[200];
//const int SleepTime=8000;       // delay between each masurement
int whenSend=15;       // after how many measurements to send data to server
int onOffTmp = 1;   //on/off temperature measure
int windDelay = 4000; // time for each anemometer measurement in seconds

void ICACHE_RAM_ATTR isr_rotation () {  // This is the function that the interrupt calls to increment the rotation count
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

  WiFiManager wifiManager;


void setup() {
  //pinMode(A0, INPUT);
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

      if (drd.detect())
    {
        wifiManager.resetSettings(); // double reset
    }

  //WiFiManagerParameter dir_offset("offset", "Vane offset", 0, 5);
  //wifiManager.addParameter(&dir_offset);
wifiManager.autoConnect("WEATHER STATION");
WiFi.forceSleepBegin(); // Wifi off

  pinMode(pwrAir, OUTPUT);      // sets the digital pin as output
   sensors.begin();
   digitalWrite(pwrAir, LOW);   // turn off power

}

  void loop() {
 
  float actualWindDelay; //time between first and last measured anemometer rotation
  firstWindPulse=1; // dont count first rotation
  contactBounceTime = millis();
  rotations = 0; // Set rotations count to 0 ready for calculations
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); //setup interrupt on anemometer input pin, interrupt will occur whenever falling edge is detected
  delay (windDelay); // Wait x second to average
  detachInterrupt(digitalPinToInterrupt(WindSensorPin));

  if(rotations==0)  {
    windSpeed=0;  
  } 
  else  {   
    actualWindDelay=(lastPulseMillis-firstPulseMillis);
    windSpeed = rotations * (2250 / actualWindDelay) * 0.868976242 * 10 ; // convert to mp/h using the formula V=P(2.25/Time); 
    // 2250 instead of 2.25 because formula is in seconds not millis   & * 0.868976242 to convert in knots   & *10 so we can calculate decimals later
    }  
  ++measureCount; // add +1 to counter
  windAvr += windSpeed; // add to sum of average wind values

  if (windSpeed >= windGust[2]) { // check if > than old gust3 of wind
    windGust[0] = windGust[1];
    windGust[1] = windGust[2];
    windGust[2] = windSpeed;
  }

  else if (windSpeed >= windGust[1]) { // check if > than old gust2 of wind
    windGust[0] = windGust[1];
    windGust[1] = windSpeed;
  }

  else if (windSpeed > windGust[0]) { // check if > than old gust1 of wind
    windGust[0] = windSpeed;
  }
  windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;

  getWindDirection();

#ifdef DEBUG
      Serial.print(" rot:");
      Serial.print(rotations);
      Serial.print(" sec:");
      Serial.print(actualWindDelay);
      Serial.print(" dir:");
      Serial.print(CalDirection);
      Serial.print(" speed:");
      Serial.print(windSpeed);
      Serial.print(" gust:");
      Serial.print(windGustAvg);
      Serial.print(" next:");
      Serial.print(whenSend-measureCount);
      Serial.print(" count:");
      Serial.print(measureCount);
      Serial.println("");
#endif


    if (measureCount >= whenSend) { // check if is time to send data online
WiFi.forceSleepWake(); // Wifi on
wifiManager.autoConnect("WEATHER STATION");
//WiFi.begin();
sendData();
WiFi.forceSleepBegin(); // Wifi off

    }
//  delay (SleepTime); // Wait
}

void getTemp() {
    digitalWrite(pwrAir, HIGH);   // turn on power
  delay(1000);
    sensors.requestTemperatures(); // get Temperature
  delay(1000);
    temp=sensors.getTempCByIndex(0);

  dtostrf(temp, 4, 1, tmp); //float Tmp to char
  if (temp > -100 && temp < 85) { dtostrf(temp, 4, 1, tmp); }   //float Tmp to char
  digitalWrite(pwrAir, LOW);   // turn off power

}

void dominantDirection(){ // get dominant wind direction
 int maxIndex = 0;
 int max = avrDir[maxIndex];
 for (int i=1; i<16; i++){
   if (max<avrDir[i]){
     max = avrDir[i];
     windDir = i*22; //this is just approximate calculation so server can return the right char value
   }
 }
}

void getAvgWInd() {
  wind_speed=windAvr/measureCount; // calculate average wind
}


// Get Wind Direction, and split it in 16 parts and save it to array
void getWindDirection() {
   VaneValue = analogRead(windVanePin);
   Direction = map(VaneValue, 0, 1023, 0, 360);         
   CalDirection = Direction + VaneOffset;

   if(CalDirection > 360)
     CalDirection = CalDirection - 360;

   if(CalDirection < 0)
     CalDirection = CalDirection + 360;

  CalDirection=(CalDirection+11.25)/22.5;
  if(CalDirection < 16) { ++avrDir[CalDirection]; }
  else { ++avrDir[0]; }


}


void sendData() {
  dominantDirection();
  getAvgWInd();
    if (onOffTmp > 0) {
      getTemp();
      delay(500);
  }

long rssi = WiFi.RSSI();  
HTTPClient http;
http.begin(webpage);
http.addHeader("Content-Type", "application/json");

sprintf(body, BODY_FORMAT, id,windDir,wind_speed/10,wind_speed%10,windGustAvg/10,windGustAvg%10,tmp,rssi);

int httpCode = http.POST(body);
#ifdef DEBUG
  Serial.println(body);
#endif

String payload = http.getString();
#ifdef DEBUG
  Serial.println(payload);
#endif

if (httpCode == HTTP_CODE_OK) {
      measureCount=0;
      windAvr=0;
      windGustAvg=0;
      windDir = 0; 
      temp=0;
      memset(avrDir,0,sizeof(avrDir)); // empty direction array
      memset(windGust, 0, sizeof(windGust)); // empty direction array
      memset(tmp, 0, sizeof(tmp));

      StaticJsonDocument<200> doc;
      deserializeJson(doc, payload);
      JsonObject root = doc.as<JsonObject>();
      
    int whenSend2 = root["w"];
    int Offset = root["o"];
    int windDelay2 = root["wd"];
    int tt = root["tt"];

     if (whenSend2 != whenSend && whenSend2 > 0) { // server response to when to do next update
        whenSend = root["w"];
      }

    if (Offset!=VaneOffset && Offset > -999) { // server sends wind wane position
        VaneOffset=Offset;

        }

   if (windDelay2 != windDelay && windDelay2 > 0 ) { // interval for one wind measurement
      windDelay = root["wd"];   
         }
         
      if (tt != onOffTmp && tt > -1) { // on/off tmp sensor
        onOffTmp = root["tt"];
      }
    }
}
