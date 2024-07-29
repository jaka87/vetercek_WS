//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xEF:m efuse:w:0xFF:m hfuse:w:DA:m lock:w:0xFF:m 
// 57600 max baud rate
// before uploading skech burn bootloader
// hardware serial buffer 128b 

//ultrasonic anemometer To active ASCI.
//>UartPro:2\r\n
//>SaveConfig\r\n


#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include "src/OneWire/OneWire.h" //tmp sensor
//#include "src/DS18B20/DS18B20.h"
#include "src/DS18B20/DallasTemperature.h"
#include "src/TimerOne/TimerOne.h"
#include "src/bmp/ErriezBMX280.h"
#include "src/Fona/Adafruit_FONA.h"
#include <avr/interrupt.h>
#include <EEPROM.h>
int resetReason = MCUSR;


//////////////////////////////////    EDIT THIS FOR CUSTOM SETTINGS
#define APN "iot.1nce.net"
int GSMnetwork1= 29340; // A1 SLOVENIA
int GSMnetwork2= 21910; // A1 HR
int GSMnetwork3= 21902; // tele2 2g HR
byte GSMstate=13; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 for automatic
byte cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 10; // interval after how many measurements data is send
const char* broker = "vetercek.com";
int sea_level_m=5; // enter elevation for your location for pressure calculation
/////////////////////////////////    OPTIONS TO TURN ON AN OFF
#define DEBUG // comment out if you want to turn off debugging
//#define UZ_Anemometer // if ultrasonic anemometer - PCB minimum PCB v.0.5
//#define HUMIDITY 31 // 31 or 41 or comment out if you want to turn off humidity sensor
//#define BMP // comment out if you want to turn off pressure sensor and save space
//#define TMPDS18B20 // comment out if you want to turn off humidity sensor
//#define TMP_POWER_ONOFF // comment out if you want power to be on all the time
//#define SIM_NEW_LIBRARY
///////////////////////////////////////////////////////////////////////////////////

#ifdef SIM_NEW_LIBRARY 
  #include "src/SIM/BotleticsSIM7000.h"
#else
  #include "src/Fona/Adafruit_FONA.h"
#endif

#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define USRX 11
#define USTX 12
#define PWRAIR 8
#define PWRWATER 9  
#define windVanePin PIN_A3       // The pin the wind vane sensor is connected to
#define DTR 6
#define PWRKEY 10
#define ENABLE_UART_START_FRAME_INTERRUPT UCSR1D = (1 << RXSIE) | (1 << SFDE)
#define DISABLE_UART_START_FRAME_INTERRUPT UCSR1D = (0 << RXSIE) | (0 << SFDE)

byte data[] = { 11,11,11,11,11,11,11,1, 0,0, 0,0, 0,0, 0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0 }; // data


#ifdef TMPDS18B20
  #include "src/OneWire/OneWire.h" //tmp sensor
  #include "src/DS18B20/DallasTemperature.h"
  OneWire oneWire_in(ONE_WIRE_BUS_1);   //tmp
  DallasTemperature sensor_air(&oneWire_in);
  OneWire oneWire_out(ONE_WIRE_BUS_2);
  DallasTemperature sensor_water(&oneWire_out);
#endif

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
// 86 - gsm NC
// 88 - other
// 89 - no GSM serial connection


HardwareSerial *fonaSS = &Serial;
#ifdef UZ_Anemometer
   #define ultrasonic Serial1
#endif
#ifdef DEBUG
  #include <NeoSWSerial.h>
  NeoSWSerial DEBUGSERIAL( 5, 7 ); 
#endif



#ifdef SIM_NEW_LIBRARY 
  Botletics_modem_LTE fona = Botletics_modem_LTE();
#else
  Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();
#endif

#ifdef BMP
  #include "LPS35HW.h"
  const uint8_t address = 0x5D; 
  LPS35HW lps(address);
#endif

#ifdef HUMIDITY
  #include "Wire.h"
  #if HUMIDITY == 31
    #include "src/HUM/SHT31.h"
    #define SHT_ADDRESS   0x44
    SHT31 sht;    
  #else
    #include "src/HUM/SHT4x.h"
    SHT4x sht;
  #endif
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
int PDPcount=0; // first reset after 100s
byte failedSend=0; // if send fail
byte sonicError=0;
byte sonicError2=0;
byte UltrasonicAnemo=0;
byte enableSolar=0;
byte enableRain=0;
byte enableBmp=0;
int pressure=0;
byte changeSleep=0;
byte batteryState=0; // 0 normal; 1 low battery; 2 very low battery
byte stopSleepChange=0; //on
volatile byte countWake = 0;
byte enableHum=0;
byte humidity=0;


void setup() {
  MCUSR = 0; // clear reset flags
  wdt_disable();
  
  Timer1.initialize(1000000UL);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(CheckTimerGPRS);  // attaches checkTimer() as a timer overflow interrupt

  pinMode(PWRKEY, OUTPUT);
  digitalWrite(PWRKEY, LOW);
  pinMode(13, OUTPUT);     // this part is used when you bypass bootloader to signal when board is starting...
  digitalWrite(13, HIGH);   // turn the LED on
  delay(500);              // wait
  digitalWrite(13, LOW);    // turn the LED
  delay(50);
  pinMode(pwrAir, OUTPUT);      // sets the digital pin as output
  pinMode(pwrWater, OUTPUT);      // sets the digital pin as output
  digitalWrite(pwrAir, HIGH);   // turn on power
  digitalWrite(pwrWater, HIGH);   // turn on power  
  pinMode(DTR, OUTPUT);
  digitalWrite(DTR, LOW); 
  pinMode(PIN_A2, OUTPUT);
  digitalWrite(PIN_A2, HIGH);   
  
  #ifdef UZ_Anemometer
    ENABLE_UART_START_FRAME_INTERRUPT;
  #endif


  
#ifdef DEBUG
  DEBUGSERIAL.begin(9600);
  delay(20);
  DEBUGSERIAL.println(F("S"));
  DEBUGSERIAL.println(resetReason);
  Serial1.begin(9600); //for sim7000 debug
#endif



#ifdef TMPDS18B20
  sensor_air.begin();
#endif

if (EEPROM.read(11)==255 or EEPROM.read(11)==1) {  enableSolar=1; }   
  if (EEPROM.read(10)==0) { attachInterrupt(digitalPinToInterrupt(3), rain_count, FALLING); enableRain=1;} // rain counts
  #ifdef TMPDS18B20
    else { sensor_water.begin(); } // water temperature
  #endif  
  if (EEPROM.read(9)==13) { GSMstate=13; }
  else if (EEPROM.read(9)==2) { GSMstate=2; }
  else if (EEPROM.read(9)==38) {GSMstate=38; } //#define NBIOT
  else if (EEPROM.read(9)==51) {GSMstate=51; } //#define NBIOT
  if (EEPROM.read(14)==10) { stopSleepChange=3; } // UZ sleep on/off

#ifdef BMP
  if ((EEPROM.read(13)==255 or EEPROM.read(13)==1) and lps.begin()) {  
      enableBmp=1; 
      lps.setLowPower(true);
      lps.setOutputRate(LPS35HW::OutputRate_OneShot);   
      }
#endif   

#ifdef HUMIDITY
  #if HUMIDITY == 31
    Wire.begin();
    sht.begin(SHT_ADDRESS);
    Wire.setClock(100000);
    uint16_t stat = sht.readStatus();
  
      if ( sht.isConnected() ){
        enableHum=1;
      }
  #else
    Wire.begin();
    sht.setChipType(SHT4X_CHIPTYPE_A);
    sht.setMode(SHT4X_CMD_MEAS_HI_PREC);
  if (sht.checkSerial() == SHT4X_STATUS_OK) {
    enableHum=1;
  }
  #endif 
#endif

delay(4000);  
digitalWrite(PWRKEY, HIGH);
moduleSetup(); // Establishes first-time serial comm and prints IMEI 
bool checkAT = fona.checkAT();
if (fona.checkAT()) { checkIMEI(); }
connectGPRS(); 


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
    else if (EEPROM.read(15)==7 ) { 
      resetReason=87; 
    } 
    else if (EEPROM.read(15)==9 ) { 
      resetReason=89; 
    } 
      else { 
        resetReason=88; 
      }   
    EEPROM.write(15, 0); 
    } 
  } 


 

  beforeSend();

#ifdef UZ_Anemometer
  unsigned long startedWaiting = millis();
  UZ_wake(startedWaiting);
  if (millis() - startedWaiting <= (45000) ) {
    UltrasonicAnemo=1;
    windDelay=1000;
    ultrasonicFlush();
   #ifdef DEBUG
    DEBUGSERIAL.println(F("UZ"));
    delay(50);
  #endif  
  }
  else {
  #ifdef DEBUG
    DEBUGSERIAL.println(F("UZ fail"));
    delay(50);
  #endif  
    reset(7); // reset board if UZ not aveliable
  } 
  LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
#endif 
}

void loop() {
     
#ifdef UZ_Anemometer
  DISABLE_UART_START_FRAME_INTERRUPT;    
  unsigned long startedWaiting = millis();

  while(ultrasonic.available() < 2 and millis() - startedWaiting <= 50) {  delay(10); }
  if (ultrasonic.available() < 2 ) { // sleep while receiving data and anemometer sleep time 3s or more
        LowPower.idle(SLEEP_2S, ADC_OFF, TIMER4_OFF,TIMER3_OFF,TIMER2_ON, TIMER1_OFF, TIMER0_OFF,SPI1_OFF,SPI0_OFF,USART1_ON, USART0_OFF, TWI1_OFF,TWI0_OFF,PTC_OFF);
  }
  else { // sleep while receiving data and anemometer sleep time 2s or less 
        LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER4_OFF,TIMER3_OFF,TIMER2_ON, TIMER1_OFF, TIMER0_OFF,SPI1_OFF,SPI0_OFF,USART1_ON, USART0_OFF, TWI1_OFF,TWI0_OFF,PTC_OFF);
  }

    while (ultrasonic.read() != ',' and millis() - startedWaiting <= 7000) {  } 
    delay(90);
  
  if (ultrasonic.available()>61){  
    UltrasonicAnemometer();
    } 
  else {  
  ultrasonicFlush();
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
if ( measureCount >= ((whenSend*2)+40))   {  reset(6);  } // reset if more than 40 tries
if ( ((resetReason==2 or resetReason==5) and measureCount > 2)  // if reset buttion is pressed and 3 measurements are made
  or (wind_speed >= (cutoffWind*10) and measureCount >= whenSend ) // if wind avg exeeds cut off value and enough measurements are  made
  or (measureCount >= (whenSend*2))
  or (measureCount>500 ) // if more than 500 measurements
  )
  
  {  
    beforeSend();
  }
  
  else if ( UltrasonicAnemo==0 ){ // restart timer
    noInterrupts();
    timergprs = 0;                                
    interrupts();
  }

#ifdef UZ_Anemometer
  else if ( UltrasonicAnemo==1 ){ // go to sleep
      #ifdef DEBUG
      DEBUGSERIAL.print(F("wc "));
      DEBUGSERIAL.println(countWake);
    #endif   
    ENABLE_UART_START_FRAME_INTERRUPT;
    countWake=0;
    LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
    }    

#endif 
}


void beforeSend() { 
      /////////////////////////// send data to server ///////////////////////////////////////////////  
//      #ifdef UZ_Anemometer
//        ultrasonic.end();
//      #endif
      GetTmpNow();
      digitalWrite(DTR, LOW);  //wake up  
      delay(10);
      bool checkAT = fona.checkAT();
        if (fona.checkAT()) { SendData(0); }
        else {moduleSetup(); SendData(0); }
      digitalWrite(DTR, HIGH);  //sleep  
      delay(50);

      #ifdef UZ_Anemometer
//        unsigned long startedWaiting = millis();
//        UZ_wake(startedWaiting);
        if (UltrasonicAnemo==1){
            if ( changeSleep== 1 and stopSleepChange<3) { //change of sleep time
          ultrasonicFlush();   
          UZsleep(sleepBetween);
            }
        }
      ultrasonicFlush();  
      ENABLE_UART_START_FRAME_INTERRUPT;
      LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
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
    if (rr > 0 ) {
      EEPROM.write(15, rr);
    }
  delay(20);
  #ifdef DEBUG
    DEBUGSERIAL.print(F("err_r: "));
    DEBUGSERIAL.println(rr);
  #endif  
  digitalWrite(PWRKEY, LOW);
  delay(4000);  
  wdt_enable(WDTO_60MS);
  delay(100);
}



void simReset() {
  digitalWrite(PIN_A2, LOW);     
  delay(300);   
  digitalWrite(PIN_A2, HIGH);  
}


#ifdef UZ_Anemometer
void UZ_wake(unsigned long startedWaiting) {
  while (!ultrasonic.available() && millis() - startedWaiting <= 45000) {  // if US not aveliable start it
    ultrasonic.begin(9600);
    delay(800);
    }      
}
#endif  



ISR(USART1_START_vect){
countWake++;
}
