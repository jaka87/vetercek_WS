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
#define WindSensorPin (D2) // The pin location of the anemometer sensor 
#define WindVanePin A0       // The pin the wind vane sensor is connected to
#define ONE_WIRE_BUS (D4)
#define BODY_FORMAT "{\"id\": \"%s\", \"d\": \"%d\", \"s\": \"%d.%d\", \"g\": \"%d.%d\", \"t\": \"%s\"}"
#define DEBUG true
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


const char *id=API_PASSWORD;  // get this unique ID in order to send data to vetercek.com
const char *webpage="http://vetercek.com/xml/post.php";  // where POST request is made
volatile unsigned long Rotations; // cup rotation counter used in interrupt routine 
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine 
int WindSpeed; // speed  
long WindAvr=0; //sum of all wind speed between update
int WindGust=0;
int avrDir[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; // array where wind direction are saved and later one with highest value is selected as dominant
int measure_count=0; // count each mesurement
int VaneValue;       // raw analog value from wind vane
int Direction;       // translated 0 - 360 direction
int VaneOffset=0;        // define the anemometer offset from magnetic north
int CalDirection;    // converted value with offset applied
int wind_dir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
float Temp; //Stores Temperature value
char tmp[6]; // Temperature char value
char response[100];
char body[200]; 
const int SleepTime=10000;       // delay between each masurement
int WhenSend=50;       // after how many measurements to send data to server

void ICACHE_RAM_ATTR isr_rotation () {  // This is the function that the interrupt calls to increment the rotation count 
  if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact. 
  Rotations++; 
  ContactBounceTime = millis(); 
  } 
}

void setup() {
  pinMode(A0, INPUT);
  Serial.begin(115200);
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  //WiFiManagerParameter dir_offset("offset", "Vane offset", 0, 5);
  //wifiManager.addParameter(&dir_offset);
  wifiManager.autoConnect("WEATHER STATION");
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); // interupt for anemometer
}

  void loop() {
  Rotations = 0; // Set Rotations count to 0 ready for calculations 
  sei(); // Enables interrupts 
  delay (2000); // Wait 2 second to average 
  cli(); // Disable interrupts 
  //delay (5000); // Wait 2 second to average 

  WindSpeed = Rotations * 1.125 * 0.868976242 * 10 ;  // convert to mp/h using the formula V=P(2.25/Time);    *0.868976242 to get knots 
  ++measure_count; // add +1 to counter
  WindAvr += WindSpeed; // add to sum of average wind values
  getWindDirection();
  
    if (DEBUG){
      Serial.print("dir:"); 
      Serial.print(CalDirection); 
      Serial.print(" speed:"); 
      Serial.print(WindSpeed); 
      Serial.print(" gust:"); 
      Serial.print(WindGust);  
      Serial.print(" next:");  
      Serial.print(WhenSend-measure_count); 
      Serial.print(" count:");  
      Serial.print(measure_count); 
      Serial.println(""); 
    }   
    if (measure_count >= WhenSend) { // check if is time to send data online
      sendData();
    } 
}

void getTemp() {
    sensors.requestTemperatures(); // get Temperature
    Temp=sensors.getTempCByIndex(0);
  dtostrf(Temp, 4, 1, tmp); //float Tmp to char

}

void dominantDirection(){ // get dominant wind direction
 int maxIndex = 0;
 int max = avrDir[maxIndex];
 for (int i=1; i<16; i++){
   if (max<avrDir[i]){
     max = avrDir[i];
     wind_dir = i*22; //this is just approximate calculation so server can return the right char value
   }
 }
}

void getAvgWInd() {
  wind_speed=WindAvr/measure_count; // calculate average wind
}


// Get Wind Direction, and split it in 16 parts and save it to array
void getWindDirection() {
   VaneValue = analogRead(WindVanePin);
   Direction = map(VaneValue, 0, 1023, 0, 360);
   CalDirection = Direction + VaneOffset;
   
   if(CalDirection > 360)
     CalDirection = CalDirection - 360;
     
   if(CalDirection < 0)
     CalDirection = CalDirection + 360;

  CalDirection=(CalDirection+11.25)/22.5;
  if(CalDirection < 16) { ++avrDir[CalDirection]; }
  else { ++avrDir[0]; }

    if (WindSpeed > WindGust) { // check if > than old gust of wind
      WindGust=WindSpeed;
     } 
}


void sendData() {
  dominantDirection();
  getAvgWInd();
  getTemp();

HTTPClient http;
http.begin(webpage);
http.addHeader("Content-Type", "application/json");

sprintf(body, BODY_FORMAT, id,wind_dir,wind_speed/10,wind_speed%10,WindGust/10,WindGust%10,tmp);

int httpCode = http.POST(body);
Serial.println(body);

String payload = http.getString();
Serial.print(payload);


if (httpCode == HTTP_CODE_OK) {
      measure_count=0;
      WindAvr=0; 
      WindGust=0;
      Temp=0;
      memset(avrDir,0,sizeof(avrDir)); // empty direction array

      StaticJsonDocument<200> doc;
      deserializeJson(doc, payload);
      JsonObject root = doc.as<JsonObject>();
      const char* idd = root["id"];
      int WhenSend2 = root["whensend"];
      int Offset = root["offset"];

      if (Offset > -999){  // server response to when to do next update 
        VaneOffset=Offset;
        }
      }
}
