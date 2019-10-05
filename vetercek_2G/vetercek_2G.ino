#include <avr/wdt.h>
#include <Http.h>
#include <Parser.h>
#include <Sim800.h>
#include <ArduinoJson.h> //parse server response
#include "LowPower.h"
#include <math.h> // wind speed calculations
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS_1 2
#define ONE_WIRE_BUS_2 4
OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sensor_air(&oneWire_in);
DallasTemperature sensor_water(&oneWire_out);
#include "config.h"
#define WindSensorPin (3) // The pin location of the anemometer sensor 
#define WindVanePin (A3)       // The pin the wind vane sensor is connected to
#define ONE_WIRE_BUS 2 //air
#define ONE_WIRE_BUS2 4 // water
#define DEBUG true

// edit this data to suit your needs  ///////////////////////////////////////////////////////
const char *bearer = "iot.1nce.net"; // APN address
const char *id = API_PASSWORD; // get this unique ID in order to send data to vetercek.com
const char *webpage = "vetercek.com/xml/post.php"; // where POST request is made
int wind_delay = 2; // time for each anemometer measurement in seconds
int VaneOffset = 29;      // define the anemometer offset from magnetic north
int onofftmp = 0;   //on/off temperature measure
int WhenSend = 3;     // after how many measurements to send data to server
//////////////////////////////////////////////////////////////////////////////////////////////

unsigned int RX_PIN = 9; //RX pin for sim800
unsigned int TX_PIN = 8; //TX pin for sim800
unsigned int RST_PIN = 12; //RST pin for sim800 - not in use
volatile unsigned long Rotations; // cup rotation counter used in interrupt routine
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine
int WindSpeed; // speed
long WindAvr = 0; //sum of all wind speed between update
int WindGust[3] = { 0, 0, 0 }; // top three gusts
int WindGustAvg = 0; //wind gust average
int avrDir[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // array where wind direction are saved and later one with highest value is selected as dominant
int measure_count = 0; // count each mesurement
float Water; // water Temperature
char wat[6]; // Water char value
float Temp; //Stores Temperature value
char tmp[6]; // Temperature char value
int VaneValue;       // raw analog value from wind vane
int Direction;       // translated 0 - 360 direction
int CalDirection;    // converted value with offset applied
int wind_dir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
unsigned int bat=0; // battery percentage
char response[60];
char body[160];
Result result;


HTTP http(9600, RX_PIN, TX_PIN, RST_PIN);
#define BODY_FORMAT "{\"id\":\"%s\",\"d\":\"%d\",\"s\":\"%d.%d\",\"g\":\"%d.%d\",\"t\":\"%s\",\"w\":\"%s\",\"b\":\"%d\",\"c\":\"%d\" }"


// the setup routine runs once when you press reset:
void setup() {
  MCUSR=0;
  wdt_disable(); // fix watchdog reset loop
    
  if (DEBUG) {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Starting!");
  }
   pinMode(LED_BUILTIN, OUTPUT);     // this part is used when you bypass bootloader to signal when board is starting...
   digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on 
   delay(1000);                       // wait 
   digitalWrite(LED_BUILTIN, LOW);    // turn the LED

  sensor_air.begin();
  sensor_water.begin();

}

// the loop routine runs over and over again forever:
void loop() {
  anemometer();
  getWindDirection();
  LowPower.powerDown(SLEEP_8S, ADC_ON, BOD_ON);

  if (DEBUG) {
    Serial.print("dir:");
    Serial.print(CalDirection);
    Serial.print(" speed:");
    Serial.print(WindSpeed);
    Serial.print(" gust:");
    Serial.print(WindGustAvg);
    Serial.print(" next:");
    Serial.print(WhenSend - measure_count);
    Serial.print(" count:");
    Serial.print(measure_count);
    Serial.println("");
  }


  if (measure_count >= WhenSend) { // check if is time to send data online
    http.wakeUp();
    sendData();
    http.sleep();
  }

}

void anemometer() { //measure wind speed
  ContactBounceTime = millis();
  Rotations = 0; // Set Rotations count to 0 ready for calculations
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); //setup interrupt on anemometer input pin, interrupt will occur whenever falling edge is detected
  delay (wind_delay * 1000); // Wait x second to average
  detachInterrupt(digitalPinToInterrupt(WindSensorPin));

  WindSpeed = Rotations * (2.25 / wind_delay) * 0.868976242 * 10 ; // convert to mp/h using the formula V=P(2.25/Time);    *0.868976242 to get knots
  ++measure_count; // add +1 to counter
  WindAvr += WindSpeed; // add to sum of average wind values

  if (WindSpeed > WindGust[2]) { // check if > than old gust3 of wind
    WindGust[0] = WindGust[1];
    WindGust[1] = WindGust[2];
    WindGust[2] = WindSpeed;
  }

  else if (WindSpeed > WindGust[1]) { // check if > than old gust2 of wind
    WindGust[0] = WindGust[1];
    WindGust[1] = WindSpeed;
  }

  else if (WindSpeed > WindGust[0]) { // check if > than old gust1 of wind
    WindGust[0] = WindSpeed;
  }
  WindGustAvg = (WindGust[0] + WindGust[1] + WindGust[2]) / 3;
}


void isr_rotation () {  // This is the function that the interrupt calls to increment the rotation count
  if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact.
    Rotations++;
    ContactBounceTime = millis();
  }
}


void dominantDirection() { // get dominant wind direction
  int maxIndex = 0;
  int max = avrDir[maxIndex];
  for (int i = 1; i < 16; i++) {
    if (max < avrDir[i]) {
      max = avrDir[i];
      wind_dir = i * 22; //this is just approximate calculation so server can return the right char value
    }
  }
}


void getAir() {
  sensor_air.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;
  Temp = sensor_air.getTempCByIndex(0);
  if (Temp > -100 && Temp < 85) { dtostrf(Temp, 4, 1, tmp); }   //float Tmp to char
}

void getWater() {
  sensor_water.requestTemperatures(); // Send the command to get temperatures
  delay (750) ;  
  Water = sensor_water.getTempCByIndex(0);
  if (Water > -100 && Water < 85) { dtostrf(Water, 4, 1, wat); }  //float Tmp to char
}


void getAvgWInd() {
  wind_speed = WindAvr / measure_count; // calculate average wind
}


// Get Wind Direction, and split it in 16 parts and save it to array
void getWindDirection() {
  VaneValue = analogRead(WindVanePin);
  Direction = map(VaneValue, 0, 1023, 0, 360);
  CalDirection = Direction + VaneOffset;

  if (CalDirection > 360)
    CalDirection = CalDirection - 360;

  if (CalDirection < 0)
    CalDirection = CalDirection + 360;

  CalDirection = (CalDirection + 11.25) / 22.5;
  if (CalDirection < 16) {
    ++avrDir[CalDirection];
  }
  else {
    ++avrDir[0];
  }

}



// send data to server
void sendData() {
  dominantDirection(); if (DEBUG) {Serial.println("direction done");}
  getAvgWInd(); if (DEBUG) {Serial.println("wind done");}
  if (onofftmp > 0) {
    getAir();  if (DEBUG) {Serial.println("air done");}
    getWater();  if (DEBUG) {Serial.println("water done");}
    delay(1000);
  }
  
  bat=http.readVoltagePercentage(); //battery percentage
  if (DEBUG) {Serial.println("battery done ");}
  //signalq=http.readSignalStrength(); //signal quality

  
  http.configureBearer(bearer);
  result = http.connect();
  
 sprintf(body, BODY_FORMAT, id, wind_dir, wind_speed / 10, wind_speed % 10, WindGustAvg / 10, WindGustAvg % 10, tmp, wat, bat,measure_count);

  if (DEBUG) {
    Serial.println(body);
  }

  result = http.post(webpage, body, response);
  if (result == SUCCESS) {

    measure_count = 0;
    WindAvr = 0;
    WindGustAvg = 0;
    Water = 0;
    Temp = 0;
    memset(avrDir, 0, sizeof(avrDir)); // empty direction array
    memset(WindGust, 0, sizeof(WindGust)); // empty direction array
    memset(tmp, 0, sizeof(tmp));
    memset(wat, 0, sizeof(wat));
    
    StaticJsonDocument<200> doc;
    deserializeJson(doc, response);
    JsonObject root = doc.as<JsonObject>();

    int WhenSend2 = root["w"];
    int Offset = root["o"];
    int wind_delay2 = root["wd"];
    int tt = root["tt"];

    
    if (WhenSend2 != WhenSend && WhenSend2 > 0) { // server response to when to do next update
      WhenSend = root["w"];
    }

    if (Offset != VaneOffset && Offset > -999) { // server sends wind wane position
      VaneOffset = root["o"];
    }

    if (wind_delay2 != wind_delay && wind_delay2 > 0 ) { // interval for one wind measurement
      wind_delay = root["wd"];
    }

    if (tt != onofftmp && tt > -1) { // on/off tmp sensor
      onofftmp = root["tt"];
      }
         
  }

  http.disconnect();

}
