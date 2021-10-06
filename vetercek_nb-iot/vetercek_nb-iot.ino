#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include "src/OneWire/OneWire.h" //tmp sensor
#include "src/DS18B20/DallasTemperature.h"
#include "src/TimerOne/TimerOne.h"
#include "src/bmp/ErriezBMX280.h"
#include "src/Fona/Adafruit_FONA.h"
#include <EEPROM.h>

#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define windVanePin (A3)       // The pin the wind vane sensor is connected to
#define DTR 6
#define PWRKEY 10
#define RESET 7

byte data[] = { 11,11,11,11,11,11,11,1, 0,0, 0,0, 0,0, 0,0,0, 0,0,0, 0,0,0,0,0, 0,0 }; // data

OneWire oneWire_in(ONE_WIRE_BUS_1);   //tmp
DallasTemperature sensor_air(&oneWire_in);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sensor_water(&oneWire_out);

//////////////////////////////////    EEPROM DATA
// 1-8 IMEI
// 9   2G/nb-iot
// 10  water temperature or rain
// 11  solar on/off - since 0.4.4
// 12  ultraasonic on/off
// 13  pressure on/off
// 15 additional debug data
      //1 ultrasonic not connected
      //2 timer 200s
      //3 manual reset
      //4 ultrasonic check fail
      
//////////////////////////////////    EDIT THIS
#define APN "iot.1nce.net"
byte GSMstate=13; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 for automatic
byte cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 10; // interval after how many measurements data is send
const char* broker = "vetercek.com";
//#define DEBUG // comment out if you want to turn off debugging
//#define BMP // comment out if you want to turn off pressure sensor and save space
///////////////////////////////////////////////////////////////////////////////////

//#include <SoftwareSerial.h>
//SoftwareSerial fonaSS = SoftwareSerial(8, 9); // RX, TX
//SoftwareSerial ultrasonic(5,6);  //RX, STX   

#include <NeoSWSerial.h>
NeoSWSerial fonaSS( 8, 9 );
NeoSWSerial ultrasonic( 5, 6 );

//SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();


#ifdef BMP
  ErriezBMX280 bmx280 = ErriezBMX280(0x76); //pressure
#endif

//////////////////////////////////    RATHER DON'T CHANGE
unsigned int pwrAir = 11; // power for air sensor
unsigned int pwrWater = 12; // power for water sensor
byte resetReason = MCUSR;
int windDelay = 2300; // time for each anemometer measurement in miliseconds
byte onOffTmp = 1;   //on/off temperature measure
volatile unsigned long timergprs = 0; // timer to check if GPRS is taking to long to complete
volatile unsigned long rotations; // cup rotation counter used in interrupt routine
volatile unsigned long contactBounceTime; // Timer to avoid contact bounce in interrupt routine
volatile unsigned long lastPulseMillis; // last anemometer measured time
volatile unsigned long firstPulseMillis; // fisrt anemometer measured time
volatile unsigned long currentMillis;
volatile unsigned long currentMillis2;
volatile unsigned long contactBounceTime2; // Timer to avoid contact bounce in rain interrupt routine
volatile unsigned long updateBattery = 0;

int rainCount=-1; // count rain bucket tilts
byte SolarCurrent; // calculate solar cell current 
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
byte sleepBetween=8;
byte sendBatTemp=10;
int PDPcount=0; // first reset after 100s
byte failedSend=0; // if send fail
byte sonicError=0;
byte UltrasonicAnemo=0;
byte enableSolar=0;
byte enableRain=0;
byte enableBmp=0;
int pressure=0;

void setup() {
  MCUSR = 0; // clear reset flags
  wdt_disable();
  Timer1.initialize(1000000);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(CheckTimerGPRS);  // attaches checkTimer() as a timer overflow interrupt

pinMode(DTR, OUTPUT);
pinMode(RESET, OUTPUT);
pinMode(PWRKEY, OUTPUT);
digitalWrite(DTR, LOW); 
digitalWrite(RESET, HIGH); 
digitalWrite(PWRKEY, LOW);
  
#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start");
#endif

  pinMode(13, OUTPUT);     // this part is used when you bypass bootloader to signal when board is starting...
  digitalWrite(13, HIGH);   // turn the LED on
  delay(1000);                       // wait
  digitalWrite(13, LOW);    // turn the LED
  delay(100);
  pinMode(pwrAir, OUTPUT);      // sets the digital pin as output
  pinMode(pwrWater, OUTPUT);      // sets the digital pin as output
  digitalWrite(pwrAir, HIGH);   // turn on power
  digitalWrite(pwrWater, HIGH);   // turn on power  
  delay(100);
  sensor_air.begin();

  if (EEPROM.read(11)==255 or EEPROM.read(11)==1) {  enableSolar=1; }   
  if (EEPROM.read(10)==0) { attachInterrupt(digitalPinToInterrupt(3), rain_count, FALLING); enableRain=1;} // rain counts
  else { sensor_water.begin(); } // water temperature
  if (EEPROM.read(9)==13) { GSMstate=13; }
  else if (EEPROM.read(9)==2) { GSMstate=2; }
  else if (EEPROM.read(9)==38) {GSMstate=38; } //#define NBIOT


#ifdef BMP
  if ((EEPROM.read(13)==255 or EEPROM.read(13)==1) and bmx280.begin()) {  
      enableBmp=1; 
      bmx280.setSampling(BMX280_MODE_FORCED,    // SLEEP, FORCED, NORMAL
                       BMX280_SAMPLING_NONE,   // Temp:  NONE, X1, X2, X4, X8, X16
                       BMX280_SAMPLING_X8,   // Press: NONE, X1, X2, X4, X8, X16
                       BMX280_SAMPLING_NONE,   // Hum:   NONE, X1, X2, X4, X8, X16 (BME280)
                       BMX280_FILTER_X8,     // OFF, X2, X4, X8, X16
                       BMX280_STANDBY_MS_500);// 0_5, 10, 20, 62_5, 125, 250, 500, 1000
  }   
#endif  
  //power
  //powerOn(); // Power on the module

moduleSetup(); // Establishes first-time serial comm and prints IMEI
checkIMEI();
connectGPRS();



if (EEPROM.read(12)==1 or EEPROM.read(12)==255) {   // if ultrasonic enabled
    ultrasonic.begin(9600);
    delay(4000);
     if ( ultrasonic.available()) { // check if ultrasonic anemometer is pluged in
      UltrasonicAnemo=1;
      windDelay=1000; // only make one measurement with sonic anemometer
     }
 }
   
  if (resetReason==8 ) { //////////////////// reset reason detailed        
    if (EEPROM.read(15)==1 ) { 
      resetReason=81; 
    }
    else if (EEPROM.read(15)==2 ) { 
      resetReason=82; 
    }  
    else if (EEPROM.read(15)==3 ) { 
      resetReason=83; 
    }  
    else if (EEPROM.read(15)==4 ) { 
      resetReason=84; 
    }      
    else { 
      resetReason=88; 
    }   
  EEPROM.write(15, 0); 
  }  
}

void loop() {

 if ( UltrasonicAnemo==1 ) {    // if ultrasonic anemometer pluged in at boot
  unsigned long startedWaiting = millis();
  while (!ultrasonic.available() && millis() - startedWaiting <= 5000) {  // if US not aveliable start it
    ultrasonic.begin(9600);
    delay(100);
   }

  if ( millis() - startedWaiting >= 5000 && sonicError < 5)  { // if US error 
    sonicError++;
     }
  else if ( millis() - startedWaiting >= 5000 && sonicError >= 5)  { // if more than 5 US errors
        reset(1);
     }
  else  { 
   UltrasonicAnemometer();       
     }   

//  if ( sleepBetween > 0)  { // to sleap or not to sleap between wind measurement
//      ultrasonic.flush();
//      ultrasonic.end();
//     }     
        #ifdef DEBUG
          Serial.flush();
        #endif                       
 }           

 else { 
 
    Anemometer();                           // anemometer
    GetWindDirection();
 }


 if ( UltrasonicAnemo==1 ) { 
  LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_ON, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_ON, TWI_OFF); 
 }
 else {
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
}
  
  #ifdef DEBUG                                 // debug data
    Serial.print(" d:");
    Serial.print(calDirection);
    Serial.print(" s:");
    Serial.print(windSpeed);
    Serial.print(" g:");
    Serial.print(windGustAvg);
    Serial.print(" c:");
    Serial.println(measureCount);
  #endif

  GetAvgWInd();                                 // avg wind


  digitalWrite(13, HIGH);   // turn the LED on
  delay(50);                       // wait
  digitalWrite(13, LOW);    // turn the LED
  delay(50);                       // wait
  digitalWrite(13, HIGH);   // turn the LED on
  delay(50);                       // wait
  digitalWrite(13, LOW);    // turn the LED


// check if is time to send data online  
if ( ((resetReason==2 or resetReason==5) and measureCount > 2)  // if reset buttion is pressed and 3 measurements are made
  or (wind_speed >= (cutoffWind*10) and ((measureCount >= whenSend and UltrasonicAnemo==0) or (measureCount >= whenSend*10 and UltrasonicAnemo==1 ))) // if wind avg exeeds cut off value and enough measurements are  made
  or ((measureCount >= (whenSend*2) and UltrasonicAnemo==0) or (measureCount >= (whenSend*20) and UltrasonicAnemo==1))
  or ((measureCount>250 and UltrasonicAnemo==0) or (measureCount>2500 and UltrasonicAnemo==1)) // if 2x measurements is made no matter the speed avg (max is 250 measurements)
  )
  
  {  
      digitalWrite(DTR, LOW);  //wake up  
      delay(100);
      fonaSS.listen();
      delay(500);
        SendData();
      digitalWrite(DTR, HIGH);  //sleep  
      if (UltrasonicAnemo==1){
        ultrasonic.listen();
      }
  delay(500);

  }
  else { // restart timer
    noInterrupts();
    timergprs = 0;                                // reset timer 
    interrupts();
  }
}


void CheckTimerGPRS() { // if unable to send data in 200s
  timergprs++;
  if (timergprs > 200 ) {
    #ifdef DEBUG
      Serial.println("hardR");
    #endif    
    timergprs = 0;
    //powerOn();
    //delay(1000);
    reset(2);
  }
}

void reset(byte rr) {
#ifdef DEBUG
  Serial.flush();
  Serial.end();
  EEPROM.write(15, rr);
  delay(200);
#endif
  powerOn(); // turn off power
  delay(1000);
  wdt_enable(WDTO_1S);
}



// Power on the module
void powerOn() {
    //pinMode(PWRKEY, OUTPUT);
  digitalWrite(PWRKEY, LOW);
  delay(3000); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
   #ifdef DEBUG
    Serial.println("Pwr on");
   #endif   
  delay(1000);
}

void wakeUp() {
  digitalWrite(PWRKEY, LOW);
  delay(100); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
}
