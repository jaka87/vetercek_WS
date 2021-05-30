#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include "src/OneWire/OneWire.h" //tmp sensor
#include "src/DS18B20/DallasTemperature.h"
#include "src/TimerOne/TimerOne.h"
#define TINY_GSM_MODEM_SIM7000
#include "src/Fona/Adafruit_FONA.h"
#include <EEPROM.h>

#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define windVanePin (A3)       // The pin the wind vane sensor is connected to
#define DTR 6
#define PWRKEY 10
byte data[] = { 11,11,11,11,11,11,11,1, 0,0, 0,0, 0,0, 0,0,0, 0,0,0, 0,0,0,0 }; // data

OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sensor_air(&oneWire_in);
DallasTemperature sensor_water(&oneWire_out);


#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(8, 9); // RX, TX
SoftwareSerial ultrasonic(5,6);  //RX, STX   

//SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

//////////////////////////////////    EDIT THIS
#define APN "iot.1nce.net"
int GSMstate=13; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 for automatic
int cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 40; // interval after how many measurements data is send
const char* broker = "vetercek.com";
#define DEBUG // comment out if you want to turn off debugging
#define ULTRASONIC // comment out if using mechanical anemometer
//////////////////////////////////    RATHER DON'T CHANGE
unsigned int pwrAir = 11; // power for air sensor
unsigned int pwrWater = 12; // power for water sensor
int resetReason = MCUSR;
int windDelay = 2300; // time for each anemometer measurement in miliseconds
byte onOffTmp = 1;   //on/off temperature measure
volatile unsigned long timergprs = 0; // timer to check if GPRS is taking to long to complete
volatile unsigned long rotations; // cup rotation counter used in interrupt routine
volatile unsigned long contactBounceTime; // Timer to avoid contact bounce in interrupt routine
volatile unsigned long lastPulseMillis; // last anemometer measured time
volatile unsigned long firstPulseMillis; // fisrt anemometer measured time
volatile unsigned long currentMillis;
String serialResponse = "";
byte firstWindPulse; // ignore 1st anemometer rotation since it didn't make full circle
int windSpeed; // speed
long windAvr = 0; //sum of all wind speed between update
int windGust[3] = { 0, 0, 0 }; // top three gusts
int windGustAvg = 0; //wind gust average
int measureCount = 0; // count each mesurement
float water=99.0; // water Temperature
float temp=99.0; //Stores Temperature value
int vaneValue;       // raw analog value from wind vane
int direction;       // translated 0 - 360 direction
int calDirection;    // converted value with offset applied
int windDir;  //calculated wind direction
float windAvgX; //x and y wind dirrection component
float windAvgY;
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
int battLevel = 0; // Battery level (percentage)
uint16_t battVoltage = 0; // Battery voltage
unsigned int sig = 0;
float actualWindDelay; //time between first and last measured anemometer rotation
char IMEI[15]; // Use this for device ID
int idd[15];
int sleepBetween=8;
byte sendBatTemp=10;
int PDPcount=0; // first reset after 100s
int failedSend=0; // if send fail
byte sonicError=0;
byte UltrasonicAnemo=0;

void setup() {


  MCUSR = 0; // clear reset flags
  wdt_disable();
  
#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting!");
#endif

  pinMode(13, OUTPUT);     // this part is used when you bypass bootloader to signal when board is starting...
  digitalWrite(13, HIGH);   // turn the LED on
  delay(1000);                       // wait
  digitalWrite(13, LOW);    // turn the LED
  pinMode(pwrAir, OUTPUT);      // sets the digital pin as output
  pinMode(pwrWater, OUTPUT);      // sets the digital pin as output
  digitalWrite(pwrAir, HIGH);   // turn on power
  digitalWrite(pwrWater, HIGH);   // turn on power  
  sensor_air.begin();
  sensor_water.begin();
  Timer1.initialize(1000000);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(CheckTimerGPRS);  // attaches checkTimer() as a timer overflow interrupt

  if (EEPROM.read(9)==13) { GSMstate=13; }
  else if (EEPROM.read(9)==2) { GSMstate=2; }
  else if (EEPROM.read(9)==38) {GSMstate=38; } //#define NBIOT

  
  //power
  pinMode(PWRKEY, OUTPUT);
  powerOn(); // Power on the module
  delay(4000);
  pinMode(DTR, OUTPUT);

//7/////////////////////////////////////////////dej stran /
//moduleSetup(); // Establishes first-time serial comm and prints IMEI
//checkIMEI();
//connectGPRS();
digitalWrite(DTR, HIGH);  //sleep

    ultrasonic.begin(9600);
    delay(4000);

     if ( ultrasonic.available()) { // check if ultrasonic anemometer is pluged in
      UltrasonicAnemo=1;
      windDelay=1000; // only make one measurement with sonic anemometer
     }

}

void loop() {

 if ( UltrasonicAnemo==1 ) {    // if ultrasonic anemometer pluged in at boot
  unsigned long startedWaiting = millis();
  while (!ultrasonic.available() && millis() - startedWaiting <= 5000) {
    ultrasonic.begin(9600);
    delay(100);
   }

  if ( millis() - startedWaiting >= 5000 && sonicError < 5)  { 
    sonicError++;
     }
  else if ( millis() - startedWaiting >= 5000 && sonicError >= 5)  { 
        reset();
     }
  else  { 
   UltrasonicAnemometer();       
     }   

  if ( sleepBetween > 0)  { // to sleap or not to sleap between wind measurement
      ultrasonic.flush();
      ultrasonic.end();
     }
        #ifdef DEBUG
          Serial.flush();
        #endif                       
 }           

 else { 
 
    Anemometer();                           // anemometer
    GetWindDirection();
 }

  if ( sleepBetween == 1)  { // to sleap or not to sleap between wind measurement
    LowPower.powerDown(SLEEP_1S, ADC_ON, BOD_ON);  // sleep
  }
  else if ( sleepBetween == 2)  { // to sleap or not to sleap between wind measurement
    LowPower.powerDown(SLEEP_2S, ADC_ON, BOD_ON);  // sleep
  }
  else if ( sleepBetween == 4)  { // to sleap or not to sleap between wind measurement
    LowPower.powerDown(SLEEP_4S, ADC_ON, BOD_ON);  // sleep
  }
  else if ( sleepBetween == 8)  { // to sleap or not to sleap between wind measurement
    LowPower.powerDown(SLEEP_8S, ADC_ON, BOD_ON);  // sleep
  }

  
  #ifdef DEBUG                                 // debug data
    Serial.print(" rot:");
    Serial.print(rotations);
    Serial.print(" delay:");
    Serial.print(actualWindDelay);
    Serial.print(" dir:");
    Serial.print(calDirection);
    Serial.print(" speed:");
    Serial.print(windSpeed);
    Serial.print(" gust:");
    Serial.print(windGustAvg);
    Serial.print(" next:");
    Serial.print(whenSend - measureCount);
    Serial.print(" count:");
    Serial.println(measureCount);
  #endif

  GetAvgWInd();                                 // avg wind


  if ( measureCount > 150 ) {   // check if is time to send data online
//  if ( (resetReason==2 and measureCount > 2) or (wind_speed >= (cutoffWind*10) and measureCount >= whenSend) or (measureCount >= (whenSend*2) or whenSend>150) ) {   // check if is time to send data online
      digitalWrite(DTR, LOW);  //wake up  
        SendData();
      digitalWrite(DTR, HIGH);  //sleep  

  }
  else { // restart timer
  #ifdef DEBUG
      Serial.print("tim: ");
      Serial.println(timergprs);
  #endif
    noInterrupts();
    timergprs = 0;                                // reset timer 
    interrupts();
  }
}


void CheckTimerGPRS() { // if unable to send data in 200s
  timergprs++;
  if (timergprs > 200 ) {
    #ifdef DEBUG
      Serial.println("hard reset");
    #endif    
    timergprs = 0;
    powerOn();
    delay(1000);
    reset();
  }
}

void reset() {
#ifdef DEBUG
  //Serial.println("timer reset");
  Serial.flush();
  Serial.end();
#endif
  powerOn(); // turn off power
  delay(1000);
  wdt_enable(WDTO_1S);
}



// Power on the module
void powerOn() {
  digitalWrite(PWRKEY, LOW);
  delay(3000); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
  delay(1000);
}

void wakeUp() {
  digitalWrite(PWRKEY, LOW);
  delay(100); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
}
