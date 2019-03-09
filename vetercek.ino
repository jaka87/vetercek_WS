#include <Http.h> // send data
#include <ArduinoJson.h> //parse server response
#include <Sleep_n0m1.h> // put arduino to sleep
#include <math.h> // wind speed calculations
#include <OneWire.h> // water Temperature sensor
#include <DallasTemperature.h> // water Temperature sensor
#include <DHT.h> // Temperature sensor
#include <DHT_U.h>
#define WindSensorPin (2) // The pin location of the anemometer sensor 
#define WindVanePin (A1)       // The pin the wind vane sensor is connected to
#define DHTPIN 6     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define ONE_WIRE_BUS 2
Sleep sleep;
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor
OneWire oneWire(ONE_WIRE_BUS); // water semperature
DallasTemperature sensors(&oneWire);

const char *bearer="internet.bob.si";
const char *id="e128c930fca75af8be9bff3232c697ce";  // get this unique ID in order to send data to vetercek.com
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
int Water; // water Temperature
int Temp; //Stores Temperature value
int VaneValue;       // raw analog value from wind vane
int Direction;       // translated 0 - 360 direction
int VaneOffset=0;        // define the anemometer offset from magnetic north
int CalDirection;    // converted value with offset applied
int wind_dir;  //calculated wind direction
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
char voltage[2]; //battery state
char response[100];
char body[200]; 
const int SleepTime=10000;       // delay between each masurement
int WhenSend=1;       // after how many measurements to send data to server
Result result;

HTTP http(9600, RX_PIN, TX_PIN, RST_PIN);
#define BODY_FORMAT "{\"id\": \"%s\", \"dir\": \"%d\", \"speed\": \"%d.%d\", \"gust\": \"%d.%d\", \"tmp\": \"%d.%d\", \"wat\": \"%d.%d\",  \"btt\": \"%s\"}"


// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);
  while(!Serial); // show debug info in serial monitor
  Serial.println("Starting!");
  dht.begin();
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING); // interupt for anemometer
  //pinMode(3, INPUT_PULLUP);               //setup pin 3 with internal pull-up
    
}

// the loop routine runs over and over again forever:
  void loop() {
 
  Rotations = 0; // Set Rotations count to 0 ready for calculations 
  sei(); // Enables interrupts 
  delay (2000); // Wait 1 second to average 
  cli(); // Disable interrupts 
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(10000); //sleep for: sleepTime

  WindSpeed = Rotations * 1.125 * 0.868976242 * 10 ;  // convert to mp/h using the formula V=P(2.25/Time); V = P(2.25/3) = P * 0.75 ;    0.868976242 to get knots 
  ++measure_count; // add +1 to counter
  WindAvr += WindSpeed; // add to sum of average wind values
  
  if (WindSpeed > WindGust) { // check if > than old gust of wind
     WindGust=WindSpeed;
  } 
  getWindDirection();
  
  Serial.println(CalDirection); 
  Serial.println(WindSpeed); 
  Serial.println(WindGust); 
  Serial.println(Temp); 
  Serial.println(Water); 
  Serial.println(WhenSend-measure_count); 
  Serial.println(""); 
  
    if (measure_count >= WhenSend) { // check if is time to send data online
    Temp= dht.readTemperature()*10; //read temperature...
    sensors.requestTemperatures(); // get water Temperature
    Water=sensors.getTempCByIndex(0)*10;

      http.wakeUp();
      sendData();
      http.sleep();
    } 
}


// This is the function that the interrupt calls to increment the rotation count 
void isr_rotation () { 
if ((millis() - ContactBounceTime) > 15 ) { // debounce the switch contact. 
Rotations++; 
ContactBounceTime = millis(); 
} 
}


void getHeading(int direction) {  // split wind direction in 16 parts and save it to array
if(direction < 11.25) { ++avrDir[0]; }
else if(direction < 33.75) { ++avrDir[1]; }
else if(direction < 56.25) { ++avrDir[2]; }
else if(direction < 78.75) { ++avrDir[3]; }
else if(direction < 101.25) { ++avrDir[4]; }
else if(direction < 123.75) { ++avrDir[5]; }
else if(direction < 146.25) { ++avrDir[6]; }
else if(direction < 168.75) { ++avrDir[7]; }
else if(direction < 191.25) { ++avrDir[8]; }
else if(direction < 213.75) { ++avrDir[9]; }
else if(direction < 236.25) { ++avrDir[10]; }
else if(direction < 258.75) { ++avrDir[11]; }
else if(direction < 281.25) { ++avrDir[12]; }
else if(direction < 303.75) { ++avrDir[13]; }
else if(direction < 326.25) { ++avrDir[14]; }
else if(direction < 348.75) { ++avrDir[15]; }
else { ++avrDir[0]; }
}

int dominantDirection(int* array, int size){ // get dominant wind direction
 int maxIndex = 0;
 int max = array[maxIndex];
 for (int i=1; i<size; i++){
   if (max<array[i]){
     max = array[i];
     maxIndex = i*22; //this is just approximate calculation so server can return the right char value
   }
 }
 return maxIndex;
}


// Get Wind Direction
void getWindDirection() {
   VaneValue = analogRead(WindVanePin);
   Direction = map(VaneValue, 0, 1023, 0, 360);
   CalDirection = Direction + VaneOffset;
   
   if(CalDirection > 360)
     CalDirection = CalDirection - 360;
     
   if(CalDirection < 0)
     CalDirection = CalDirection + 360;

  getHeading(CalDirection);
}



void print(const __FlashStringHelper *message, int code = -1){
  if (code != -1){
    Serial.print(message);
    Serial.println(code);
  }
  else {
    Serial.println(message);
  }
}


// send data to server
void sendData(){
  wind_dir=dominantDirection(avrDir,16);
  wind_speed=WindAvr/measure_count; 
  
  print(F("Cofigure bearer: "), http.configureBearer(bearer));
  result = http.connect();
  print(F("HTTP connect: "), result);

  http.batteryState(voltage); //battery percentage
  //http.gpsLocation(gps); // GPS location
     
  sprintf(body, BODY_FORMAT, id,wind_dir,wind_speed/10,wind_speed%10,WindGust/10,WindGust%10,Temp/10,Temp%10,Water/10,Water%10,voltage);
  Serial.println(body);
  result = http.post("vetercek.com/xml/post.php", body, response);
  print(F("HTTP POST: "), result);
  if (result == SUCCESS) {

      measure_count=0;
      WindAvr=0; 
      WindGust=0;
      Water=0;
      Temp=0;
      memset(avrDir,0,sizeof(avrDir));


      StaticJsonDocument<200> doc;
      deserializeJson(doc, response);
      JsonObject root = doc.as<JsonObject>();
      const char* idd = root["id"];
      int WhenSend2 = root["whensend"];
      int Offset = root["offset"];

      if (WhenSend2> -999){  // server response to when to do next update 
       WhenSend=WhenSend2;
      }

      if (Offset > -999){  // server response to when to do next update 
       VaneOffset=Offset;
      }
 }

  print(F("HTTP disconnect: "), http.disconnect());
}
