//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xEF:m efuse:w:0xFF:m hfuse:w:DA:m lock:w:0xFF:m 
// 57600 max baud rate
#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include "src/OneWire/OneWire.h" //tmp sensor
#include "src/DS18B20/DallasTemperature.h"
#include "src/TimerOne/TimerOne.h"
#include "src/bmp/ErriezBMX280.h"
#include "src/Fona/Adafruit_FONA.h"
#include <EEPROM.h>
#include "PinChangeInterrupt.h"
int resetReason = MCUSR;

//atmega328pb has different MCUSR than atmega328p!
// 6 - power ON 
// 2 - WTR
// 4 - brown out
// 2 - external
// 1 - power loss

//////////////////////////////////    EDIT THIS FOR CUSTOM SETTINGS
#define APN "iot.1nce.net"
byte GSMstate=2; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 for automatic
byte cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 10; // interval after how many measurements data is send
const char* broker = "vetercek.com";
int sea_level_m=0; // enter elevation for your location for pressure calculation
/////////////////////////////////    OPTIONS TO TURN ON AN OFF
//#define DEBUG // comment out if you want to turn off debugging
#define UZ_Anemometer // if ultrasonic anemometer - PCB minimum PCB v.0.5
//#define BMP // comment out if you want to turn off pressure sensor and save space
#define TMP_POWER_ONOFF // comment out if you want power to be on all the time
///////////////////////////////////////////////////////////////////////////////////




#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define USRX 11
#define USTX 12
#define RESET (A2)
#define PWRAIR 8
#define PWRWATER 9  
#define windVanePin (A3)       // The pin the wind vane sensor is connected to
#define DTR 6
#define PWRKEY 10

byte data[] = { 11,11,11,11,11,11,11,1, 0,0, 0,0, 0,0, 0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0 }; // data

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

//////////////////////////////////    RESET REASON
// 81 - UZ 10 errors in loop function
// 82 - unable to send data in 200s
// 83 - manual remote reset 
// 84 - X sonic errors in UZ function
// 85 - UDPclose
// 88 - other


HardwareSerial *fonaSS = &Serial;
#ifdef UZ_Anemometer
   #define ultrasonic Serial1
#endif
#ifdef DEBUG
  #include <NeoSWSerial.h>
  NeoSWSerial DEBUGSERIAL( 5, 7 ); 
#endif



Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

#ifdef BMP
  #include "LPS35HW.h"
  const uint8_t address = 0x5D; 
  LPS35HW lps(address);
#endif

//////////////////////////////////    RATHER DON'T CHANGE
unsigned int pwrAir = PWRAIR; // power for air sensor
unsigned int pwrWater = PWRWATER; // power for water sensor
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
//String serialResponse = "";
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
int idd[15];
byte sleepBetween=2;
byte sendBatTemp=10;
int PDPcount=0; // first reset after 100s
byte failedSend=0; // if send fail
byte sonicError=0;
byte UltrasonicAnemo=0;
byte enableSolar=0;
byte enableRain=0;
byte enableBmp=0;
int pressure=0;
byte changeSleep=0;
byte batteryState=0; // 0 normal; 1 low battery; 2 very low battery
byte stopSleepChange=0; //on
//char buffer[70];

void setup() {
  MCUSR = 0; // clear reset flags
  wdt_disable();
  
  Timer1.initialize(1000000UL);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(CheckTimerGPRS);  // attaches checkTimer() as a timer overflow interrupt
  #ifdef UZ_Anemometer
    attachPCINT(digitalPinToPCINT(12), wake_from_sleep, FALLING);
  #endif

pinMode(DTR, OUTPUT);
pinMode(RESET, OUTPUT);
pinMode(PWRKEY, OUTPUT);
digitalWrite(DTR, LOW); 
digitalWrite(RESET, HIGH); 
digitalWrite(PWRKEY, LOW);

//#ifndef ATMEGA328P
//UCSR0D = 1 << RXCIE1 | 1 << RXSIE | 1 << SFDE;
//#endif
  
#ifdef DEBUG
  DEBUGSERIAL.begin(9600);
  delay(20);
  DEBUGSERIAL.println(F("S"));
  DEBUGSERIAL.println(resetReason);
#endif

  //Serial1.begin(9600); //for sim7070 debug


  pinMode(13, OUTPUT);     // this part is used when you bypass bootloader to signal when board is starting...
  digitalWrite(13, HIGH);   // turn the LED on
  delay(1000);              // wait
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
  if (EEPROM.read(14)==10) { stopSleepChange=3; } // UZ sleep on/off


#ifdef UZ_Anemometer
  unsigned long startedWaiting = millis();
  UZ_wake(startedWaiting);
  if (millis() - startedWaiting <= 9900 ) {
    UltrasonicAnemo=1;
    windDelay=1000;
    ultrasonicFlush();
  }
   #ifdef DEBUG
    DEBUGSERIAL.println(F("UZ"));
    delay(50);
  #endif   
#endif

#ifdef BMP
  if ((EEPROM.read(13)==255 or EEPROM.read(13)==1) and lps.begin()) {  
      enableBmp=1; 
      lps.setLowPower(true);
      lps.setOutputRate(LPS35HW::OutputRate_OneShot);   
      }
#endif   

moduleSetup(); // Establishes first-time serial comm and prints IMEI
checkIMEI();
connectGPRS();

 



  if (resetReason==2 ) { //////////////////// reset reason detailed        
    if (EEPROM.read(15)>0 ) {
      if (EEPROM.read(15)==1 ) { 
        resetReason=21; 
      }
      else if (EEPROM.read(15)==2 ) { 
        resetReason=22; 
      }  
      else if (EEPROM.read(15)==3 ) { 
        resetReason=23; 
      }  
      else if (EEPROM.read(15)==4 ) { 
        resetReason=24; 
      }      
      else if (EEPROM.read(15)==5 ) { 
        resetReason=25; 
      }  
    else if (EEPROM.read(15)==6 ) { 
      resetReason=26; 
    }       
      else { 
        resetReason=22; 
      }   
    EEPROM.write(15, 0); 
    } 
  } 

  if (resetReason==8 ) { //////////////////// reset reason detailed        
    if (EEPROM.read(15)>0 ) {
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
      else if (EEPROM.read(15)==5 ) { 
        resetReason=85; 
      }  
    else if (EEPROM.read(15)==6 ) { 
      resetReason=86; 
    } 
      else { 
        resetReason=88; 
      }   
    EEPROM.write(15, 0); 
    } 
  } 

#ifdef UZ_Anemometer
  SendData();
#endif 
}

void loop() {
#ifdef UZ_Anemometer
 //if ( UltrasonicAnemo==1 ) {    // if ultrasonic anemometer pluged in at boot
 
  unsigned long startedWaiting = millis();
  //delay(15);
  //delay(25);
  UZ_wake(startedWaiting);

  if ( millis() - startedWaiting >= 10000 && sonicError < 10)  { // if US error 
    sonicError++;
    #ifdef DEBUG
    delay(50);
      DEBUGSERIAL.println(F("UZ err"));
    delay(50);
    #endif 
     }
  else if ( millis() - startedWaiting >= 10000 && sonicError >= 10)  { // if more than X US errors
        reset(1);
     }
  else { 
      while(ultrasonic.available() < 66 and millis() - startedWaiting <= 9000) {
        delay(10);
      }

      if ( millis() - startedWaiting <= 8900 )  {  
        UltrasonicAnemometer();
      }
      else  {  
        ultrasonicFlush();
         #ifdef DEBUG
          delay(50);
            DEBUGSERIAL.println(F("flushb"));
          delay(50);
          #endif 
      }
  }
          
#else 
    Anemometer();                           // anemometer
    GetWindDirection();

  if ( sleepBetween == 1)  { // to sleep or not to sleep between wind measurement
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  // sleep
  }
  else if ( sleepBetween == 2)  { // to sleep or not to sleep between wind measurement
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);  // sleep
  }
  else if ( sleepBetween > 2 and sleepBetween < 7)  { // to sleep or not to sleep between wind measurement
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);  // sleep
  }
  else if ( sleepBetween >= 7 )  { // to sleep or not to sleep between wind measurement
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  // sleep
  }
   
#endif  
  
  #ifdef DEBUG                                 // debug data
    DEBUGSERIAL.print(F(" d:"));
    DEBUGSERIAL.print(calDirection);
    DEBUGSERIAL.print(F(" s:"));
    DEBUGSERIAL.print(windSpeed);
    DEBUGSERIAL.print(F(" c:"));
    DEBUGSERIAL.print(measureCount);
    DEBUGSERIAL.print(F(" s:"));
    DEBUGSERIAL.println(sonicError);
  #endif

  GetAvgWInd();                                 // avg wind


  digitalWrite(13, HIGH);   // turn the LED on
  delay(50);                       // wait
  digitalWrite(13, LOW);    // turn the LED
 
// check if is time to send data online  
if ( (wind_speed >= (cutoffWind*10) and measureCount >= (whenSend+20) ) or (measureCount >= ((whenSend*2)+20)) )  {  reset(6);  } // reset if more than 40 tries
if ( ((resetReason==2 or resetReason==5) and measureCount > 2)  // if reset buttion is pressed and 3 measurements are made
  or (wind_speed >= (cutoffWind*10) and measureCount >= whenSend ) // if wind avg exeeds cut off value and enough measurements are  made
  or (measureCount >= (whenSend*2))
  or (measureCount>500 ) // if more than 500 measurements
  )
  
  {  
      /////////////////////////// send data to server ///////////////////////////////////////////////     
     #ifdef UZ_Anemometer
      //ultrasonic.end();
      detachPinChangeInterrupt(digitalPinToPCINT(12));
     #endif
     
      digitalWrite(DTR, LOW);  //wake up  
      delay(100);
      fona.flush();
      fona.flushInput();
      
      //delay(1000);
        bool checkAT = fona.checkAT();
        if (fona.checkAT()) {SendData();}
        else {moduleSetup(); SendData();}
      digitalWrite(DTR, HIGH);  //sleep  

      #ifdef UZ_Anemometer
        if (UltrasonicAnemo==1){
            if ( changeSleep== 1 and stopSleepChange<3) { //change of sleep time
          unsigned long startedWaiting = millis();
          //UZ_wake(startedWaiting);
          ultrasonicFlush();   
          UZsleep(sleepBetween);
            }
        }
      ultrasonicFlush();     
      attachPCINT(digitalPinToPCINT(12), wake_from_sleep, FALLING);
      #endif  
  }
  
  else if ( UltrasonicAnemo==0 ){ // restart timer
    noInterrupts();
    timergprs = 0;                                
    interrupts();
  }

#ifdef UZ_Anemometer
  else if ( UltrasonicAnemo==1 ){ // restart timer
    LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
  }
#endif 
}


void CheckTimerGPRS() { // if unable to send data in 200s
  timergprs++;
    
  if (timergprs > 200 ) {
    #ifdef DEBUG
      DEBUGSERIAL.println(F("hardR"));
    #endif    
    timergprs = 0;
    reset(2);
  }
}

void reset(byte rr) {
  EEPROM.write(15, rr);
  delay(200);
  if (rr==2 or rr==5){ powerOn(); }
  delay(100);
  wdt_enable(WDTO_60MS);
  delay(100);

}



// Power on the module
void powerOn() {
  digitalWrite(PWRKEY, LOW);
  delay(3000); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
   #ifdef DEBUG
    DEBUGSERIAL.println(F("Pwr on"));
   #endif   
  //delay(8000);
}

void wakeUp() {
  digitalWrite(PWRKEY, LOW);
  delay(100); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
}

#ifdef UZ_Anemometer
void UZ_wake(unsigned long startedWaiting) {
  while (!ultrasonic.available() && millis() - startedWaiting <= 10000) {  // if US not aveliable start it
    ultrasonic.begin(9600);
    delay(1200);
    }
 }
#endif 


void wake_from_sleep(void) {
}
