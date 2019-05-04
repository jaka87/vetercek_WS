#include <Http.h> // send data
#include <ArduinoJson.h> //parse server response
#include <Sleep_n0m1.h> // put arduino to sleep
#include <math.h> // wind speed calculations
#include <OneWire.h> // water Temperature sensor
#include <DallasTemperature.h> // water Temperature sensor
//#include <DHT.h> // Temperature sensor
//#include <DHT_U.h>
#include "config.h"
#define WindSensorPin (3) // The pin location of the anemometer sensor 
#define WindVanePin (A3)       // The pin the wind vane sensor is connected to
//#define DHTPIN 4     // what pin we're connected to
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define ONE_WIRE_BUS 2
#define DEBUG false

Sleep sleep;
//DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor
OneWire oneWire(ONE_WIRE_BUS); // water temperature
DallasTemperature sensors(&oneWire);

uint8_t sensor1[8] = { 0x28, 0x40, 0x17, 0x45, 0x92, 0x12, 0x02, 0x81 }; // inside sensor
uint8_t sensor2[8] = { 0x28, 0xAA, 0x52, 0x0C, 0x38, 0x14, 0x01, 0xFB }; // outside sensor


const char *bearer="internet.ht.hr"; // APN address
const char *id=API_PASSWORD;  // get this unique ID in order to send data to vetercek.com
const char *webpage="vetercek.com/xml/post.php";  // where POST request is made
unsigned int RX_PIN = 9; //RX pin for sim800
unsigned int TX_PIN = 8; //TX pin for sim800
unsigned int RST_PIN = 12; //RST pin for sim800 - not in use
volatile unsigned long Rotations; // cup rotation counter used in interrupt routine 
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine 
int WindSpeed; // speed  
long WindAvr=0; //sum of all wind speed between update
int WindGust=0;
int avrDir[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; // array where wind direction are saved and later one with highest value is selected as dominant
int measure_count=0; // count each mesurement
float Water; // water Temperature
char wat[6]; // Water char value
float Temp; //Stores Temperature value
char tmp[6]; // Temperature char value
int VaneValue;       // raw analog value from wind vane
int Direction;       // translated 0 - 360 direction
int VaneOffset=0;        // define the anemometer offset from magnetic north
int CalDirection;    // converted value with offset applied
int wind_dir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
char voltage[3]; //battery percentage
char gps[20]; //gps location
char response[35];
char body[160]; 
const int SleepTime=10000;       // delay between each masurement
int WhenSend=1;       // after how many measurements to send data to server
Result result;
//int ram=0;

HTTP http(9600, RX_PIN, TX_PIN, RST_PIN);
#define BODY_FORMAT "{\"id\":\"%s\",\"d\":\"%d\",\"s\":\"%d.%d\",\"g\":\"%d.%d\",\"t\":\"%s\",\"w\":\"%s\",\"b\":\"%s\",\"l\":\"%s\",\"c\":\"%d\" }"


// the setup routine runs once when you press reset:
void setup() {
    if (DEBUG){
    Serial.begin(9600);
    while(!Serial);
      Serial.println("Starting!");
  }
  //dht.begin();
  sensors.begin();
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); // interupt for anemometer
    
}

// the loop routine runs over and over again forever:
  void loop() {
 
  Rotations = 0; // Set Rotations count to 0 ready for calculations 
  sei(); // Enables interrupts 
  delay (2000); // Wait 2 second to average 
  cli(); // Disable interrupts 
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(10000); //sleep for: sleepTime

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
      Serial.print(" temp:");       
      Serial.print(Temp); 
      Serial.print(" w:");       
      Serial.print(Water); 
      Serial.print(" next:");  
      Serial.print(WhenSend-measure_count); 
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


void isr_rotation () {  // This is the function that the interrupt calls to increment the rotation count 
if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact. 
Rotations++; 
ContactBounceTime = millis(); 
} 
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

// Get temperature
//void getTemperature() {
//  Temp= dht.readTemperature(); //read temperature...
//  dtostrf(Temp, 4, 1, tmp); //float Tmp to char
//}

void getTemp() {
    sensors.requestTemperatures(); // get water Temperature
    //Water=sensors.getTempCByIndex(0); when there is only one sensor
    Water=sensors.getTempC(sensor1); //when there are two sensors
    Temp=sensors.getTempC(sensor2); //when there are two sensors
  dtostrf(Water, 4, 1, wat); //water to char
  dtostrf(Temp, 4, 1, tmp); //float Tmp to char

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



// send data to server
void sendData(){
  dominantDirection();
  getAvgWInd();
  //getTemperature();
  getTemp();
         
  http.configureBearer(bearer);
  result = http.connect();
  http.readVoltagePercentage(voltage); //battery percentage  
  http.readGpsLocation(gps); // GPS location


    sprintf(body, BODY_FORMAT, id,wind_dir,wind_speed/10,wind_speed%10,WindGust/10,WindGust%10,tmp,wat,voltage,gps,measure_count);

    if (DEBUG){
      Serial.println(body);
    }  

    result = http.post(webpage, body, response);
    if (result == SUCCESS) {
  
        measure_count=0;
        WindAvr=0; 
        WindGust=0;
        Water=0;
        Temp=0;
        memset(avrDir,0,sizeof(avrDir)); // empty direction array
  
        StaticJsonDocument<200> doc;
        deserializeJson(doc, response);
        JsonObject root = doc.as<JsonObject>();
        int WhenSend2 = root["whensend"];
        int Offset = root["offset"];
  
        if (WhenSend2> 0){  // server response to when to do next update 
         WhenSend=WhenSend2;
        }
  
        if (Offset > -999){  // server response to when to do next update 
         VaneOffset=Offset;
        }
   }

 http.disconnect();
//   ram = availableMemory();

}

//int availableMemory()
//{
// int size = 1024;
// byte *buf;
// while ((buf = (byte *) malloc(--size)) == NULL);
// free(buf);
// return size;
//}
