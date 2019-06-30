#include <Http.h> // send data
#include <ArduinoJson.h> //parse server response
#include "LowPower.h"
#include <math.h> // wind speed calculations
#include <OneWire.h> // water Temperature sensor
#include <DallasTemperature.h> // water Temperature sensor
#include "config.h"
#define WindSensorPin (3) // The pin location of the anemometer sensor 
#define WindVanePin (A3)       // The pin the wind vane sensor is connected to
#define ONE_WIRE_BUS 2
#define DEBUG true

OneWire oneWire(ONE_WIRE_BUS); // water temperature
DallasTemperature sensors(&oneWire);

const char *bearer="iot.1nce.net"; // APN address
const char *id=API_PASSWORD;  // get this unique ID in order to send data to vetercek.com
const char *webpage="vetercek.com/xml/post.php";  // where POST request is made
unsigned int RX_PIN = 9; //RX pin for sim800
unsigned int TX_PIN = 8; //TX pin for sim800
unsigned int RST_PIN = 12; //RST pin for sim800 - not in use
volatile unsigned long Rotations; // cup rotation counter used in interrupt routine 
volatile unsigned long ContactBounceTime; // Timer to avoid contact bounce in interrupt routine 
byte tmpSensors = 0; //number of temperature sensors
byte debounce = 15; // debounce timeout in ms
int wind_delay = 2; // time for each anemometer measurement in seconds
int WindSpeed; // speed  
long WindAvr=0; //sum of all wind speed between update
int WindGust[3]={ 0,0,0 }; // top three gusts
int WindGustAvg=0; //wind gust average
int avrDir[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; // array where wind direction are saved and later one with highest value is selected as dominant
int measure_count=0; // count each mesurement
float Water; // water Temperature
char wat[6]; // Water char value
float Temp; //Stores Temperature value
char tmp[6]; // Temperature char value
int VaneValue;       // raw analog value from wind vane
int Direction;       // translated 0 - 360 direction
int VaneOffset=29;        // define the anemometer offset from magnetic north
int CalDirection;    // converted value with offset applied
int wind_dir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
char voltage[3]; //battery percentage
char gps[20]; //gps location
char response[60];
char body[160]; 
int WhenSend=3;       // after how many measurements to send data to server
Result result;


HTTP http(9600, RX_PIN, TX_PIN, RST_PIN);
#define BODY_FORMAT "{\"id\":\"%s\",\"d\":\"%d\",\"s\":\"%d.%d\",\"g\":\"%d.%d\",\"t\":\"%s\",\"w\":\"%s\",\"b\":\"%s\",\"l\":\"%s\",\"c\":\"%d\" }"


// the setup routine runs once when you press reset:
void setup() {
    if (DEBUG){
    Serial.begin(9600);
    while(!Serial);
      Serial.println("Starting!");
  }
  sensors.begin();
    
}

// the loop routine runs over and over again forever:
  void loop() {
    anemometer();
    getWindDirection();
    LowPower.powerDown(SLEEP_8S, ADC_ON, BOD_ON);
  
      if (DEBUG){
        Serial.print("dir:"); 
        Serial.print(CalDirection); 
        Serial.print(" speed:"); 
        Serial.print(WindSpeed); 
        Serial.print(" gust:"); 
        Serial.print(WindGustAvg); 
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

void anemometer() { //measure wind speed
  ContactBounceTime = millis(); 
    Rotations = 0; // Set Rotations count to 0 ready for calculations 
    attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); //setup interrupt on anemometer input pin, interrupt will occur whenever falling edge is detected
    sei(); // Enables interrupts 
          delay (wind_delay*1000); // Wait x second to average 
    cli(); // Disable interrupts 
    detachInterrupt(digitalPinToInterrupt(WindSensorPin));
    
    WindSpeed = Rotations * (2.25/wind_delay) * 0.868976242 * 10 ;  // convert to mp/h using the formula V=P(2.25/Time);    *0.868976242 to get knots 
    ++measure_count; // add +1 to counter
    WindAvr += WindSpeed; // add to sum of average wind values       

    if (WindSpeed > WindGust[2]) { // check if > than old gust3 of wind
      WindGust[0]=WindGust[1];
      WindGust[1]=WindGust[2];      
      WindGust[2]=WindSpeed;
     } 

    else if (WindSpeed > WindGust[1]) { // check if > than old gust2 of wind
      WindGust[0]=WindGust[1];
      WindGust[1]=WindSpeed;      
     } 

    else if (WindSpeed > WindGust[0]) { // check if > than old gust1 of wind
      WindGust[0]=WindSpeed;
     } 
  WindGustAvg= (WindGust[0]+WindGust[1]+WindGust[2])/3;     
}


void isr_rotation () {  // This is the function that the interrupt calls to increment the rotation count 
  if ((millis() - ContactBounceTime) > debounce ) { // debounce the switch contact. 
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


void getTemp() {
    tmpSensors = sensors.getDeviceCount();
    sensors.requestTemperatures(); // get Temperature
       if (tmpSensors ==2){ getAir(); getWater(); }
       else if (tmpSensors ==1){ getAir(); }
       else { Water=-99.0; Temp=-98.0;  }
}

void getAir() {
 Temp=sensors.getTempCByIndex(0);  
  dtostrf(Temp, 4, 1, tmp); //float Tmp to char
}

void getWater() {
 Water=sensors.getTempCByIndex(1);  
  dtostrf(Water, 4, 1, wat); //float Tmp to char
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

}

// send data to server
void sendData(){
  dominantDirection();
  getAvgWInd();
  getTemp();
         
  http.configureBearer(bearer);
  result = http.connect();
  http.readVoltagePercentage(voltage); //battery percentage  
  http.readGpsLocation(gps); // GPS location


    sprintf(body, BODY_FORMAT, id,wind_dir,wind_speed/10,wind_speed%10,WindGustAvg/10,WindGustAvg%10,tmp,wat,voltage,gps,measure_count);

    if (DEBUG){
      Serial.println(body);
    }  

    result = http.post(webpage, body, response);
    if (result == SUCCESS) {
  
        measure_count=0;
        WindAvr=0; 
        WindGustAvg=0;
        Water=0;
        Temp=0;
        memset(avrDir,0,sizeof(avrDir)); // empty direction array
        memset(WindGust,0,sizeof(WindGust)); // empty direction array

        StaticJsonDocument<200> doc;
        deserializeJson(doc, response);
        JsonObject root = doc.as<JsonObject>();
        int WhenSend2 = root["w"];
        int Offset = root["o"];
        int wind_delay2 = root["wd"];
  
        if (WhenSend2> 0){  // server response to when to do next update 
         WhenSend=WhenSend2;
        }
  
        if (Offset > -999){  // server sends wind wane position 
         VaneOffset=Offset;
        }
        
             
        if (wind_delay2> 0){  // interval for one wind measurement
         wind_delay=wind_delay2;
        }  
   }

 http.disconnect();

}
