//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xEF:m efuse:w:0xFF:m hfuse:w:DA:m lock:w:0xFF:m 
// 57600 max baud rate = AT+IPR=57600
// before uploading skech burn bootloader
// hardware serial buffer 128b 

//ultrasonic anemometer To active ASCI.
//>UartPro:0\r\n or 0
//>SaveConfig\r\n
//>ComMode:0\r\n 0 232/1 485


#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
//#include "src/DS18B20/DS18B20.h"
#include "src/TimerOne/TimerOne.h"
#include <avr/interrupt.h>
#include "src/SIM/BotleticsSIM7000.h"
#include <EEPROM.h>
int resetReason = MCUSR;


//////////////////////////////////    EDIT THIS FOR CUSTOM SETTINGS
#define APN "iot.1nce.net"
byte GSMstate=51; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 (2g with nb-iot as backup) and 51 (nb-iot with 2g as backup)
byte cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 3; // interval after how many measurements data is send
int sea_level_m=0; // enter elevation for your location for pressure calculation
/////////////////////////////////    OPTIONS TO TURN ON AN OFF
#define DEBUG // comment out if you want to turn off debugging
//#define DEBUG2 // comment out if you want to turn off SIM debugging
//#define DEBUG_MEASURE // comment out if you want to turn off SIM debugging
//#define LOCAL_WS // comment out if the station is global - shown on windgust.eu
#define UZ_Anemometer // if ultrasonic anemometer - PCB minimum PCB v.0.5
//#define BMP // comment out if you want to turn off pressure sensor and save space
#define HUMIDITY 31 // 31 or 41 or comment out if you want to turn off humidity sensor
//#define TMPDS18B20 // comment out if you want to turn off temerature sensor
//#define BME // comment out if you want to turn off pressure and humidity sensor
//#define TMP_POWER_ONOFF // comment out if you want power to be on all the time
#define NETWORK_OPERATORS 1
  // 1. Slovenia
  // 2. Croatia
  // 3. Italy
  // 4. Hungary
  // 5. Austria
  // 6. Germany 
  // 7. France
  // 8. Netherlands
  // 9. Portugal
  // 10. Greece
///////////////////////////////////////////////////////////////////////////////////

#ifdef LOCAL_WS 
  char* broker = "vetercek.com";
#else
  char* broker = "data.windgust.eu";
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
// 85 - cant connect
// 86 - can't send data
// 87 - uz NC
// 88 - other
// 89 - no GSM serial connection
// 90 - no GSM connection
// 91 - no GPRS connection
// 92 - no access to server



HardwareSerial *fonaSS = &Serial;
#ifdef UZ_Anemometer
   #define ultrasonic Serial1
#endif
#ifdef DEBUG
  #include <NeoSWSerial.h>
  NeoSWSerial DEBUGSERIAL( 5, 7 ); 
#endif



Botletics_modem_LTE fona = Botletics_modem_LTE();


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

#ifdef BME
  #include "src/BME/Zanshin_BME680.h"
  BME680_Class BME680;  ///< Create an instance of the BME680 class
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

volatile int rainCount=-1; // count rain bucket tilts
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
byte enableSolar=1;
byte enableRain=0;
byte enableBmp=0;
byte enableHum=0;
int pressure=0;
byte humidity=0;
byte changeSleep=0;
byte batteryState=0; // 0 normal; 1 low battery; 2 very low battery
byte stopSleepChange=0; //on
volatile byte countWake = 0;
byte checkServernum=0;

#if NETWORK_OPERATORS == 1
  int network1=29340;
  int network2=29341;
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 2
  int network1=21901;
  int network2=21902;
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 3
  int network1=22210;
  int network2=22288;
  byte net_ver1=0;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 4
  int network1=21630;
  int network2=21670;
  byte net_ver1=0;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 5
  int network1=23201;
  int network2=23203;
  byte net_ver1=0;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 6
  int network1=26201;
  int network2=26202;
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 7
  int network1=20801;
  int network2=20810;
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 8
  int network1=20402;
  int network2=20401;
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 9
  int network1=26802;
  int network2=26801;
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 10
  int network1=20202;
  int network2=20201;
  byte net_ver1=9;
  byte net_ver2=0;
#else
  int network1=0;
  int network2=0;
  byte net_ver1=0;
  byte net_ver2=0;
#endif
//MCC + MNC

void setup() {
  MCUSR = 0; // clear reset flags
  wdt_disable();
  
  Timer1.initialize(1000000UL);         // initialize timer1, and set a 1 second period
  Timer1.attachInterrupt(CheckTimerGPRS);  // attaches checkTimer() as a timer overflow interrupt

  pinMode(13, OUTPUT);     // this part is used wPWRKEYhen you bypass bootloader to signal when board is starting...
  digitalWrite(13, HIGH);   // turn the LED on
  delay(1000);              // wait
  digitalWrite(13, LOW);    // turn the LED
  delay(50);
  pinMode(pwrAir, OUTPUT);      // sets the digital pin as output
  pinMode(pwrWater, OUTPUT);      // sets the digital pin as output
  digitalWrite(pwrAir, HIGH);   // turn on power
  digitalWrite(pwrWater, HIGH);   // turn on power  
  pinMode(DTR, OUTPUT);
  digitalWrite(DTR, LOW); 
  pinMode(PWRKEY, OUTPUT);
  digitalWrite(PWRKEY, LOW);
  pinMode(PIN_A2, OUTPUT);
  digitalWrite(PIN_A2, HIGH);   
  //adc_init();
  
  #ifdef UZ_Anemometer
    ENABLE_UART_START_FRAME_INTERRUPT;
  #endif

#ifdef DEBUG
  DEBUGSERIAL.begin(9600);
  delay(20);
  DEBUGSERIAL.println(F("S"));
  DEBUGSERIAL.println(resetReason);
#endif

#ifdef DEBUG2
  Serial1.begin(9600); //for sim7070 AT commands debug
#endif
//delay(3000);
//Serial1.begin(9600); //for sim7000 debug


#ifdef TMPDS18B20
  sensor_air.begin();
#endif
  if (EEPROM.read(10)==0) { attachInterrupt(digitalPinToInterrupt(3), rain_count, FALLING); enableRain=1;} // rain counts
  #ifdef TMPDS18B20
    else { sensor_water.begin(); } // water temperature
  #endif
  if (EEPROM.read(9)==13) { GSMstate=13; }
  else if (EEPROM.read(9)==2) { GSMstate=2; }
  else if (EEPROM.read(9)==38) {GSMstate=38; } //#define NBIOT
  else if (EEPROM.read(9)==51) {GSMstate=51; } //#define NBIOT or 2G
  if (EEPROM.read(14)==10) { stopSleepChange=3; } // UZ sleep on/off


#ifdef BMP
  if ((EEPROM.read(13)==255 or EEPROM.read(13)==1) and lps.begin()) {  
      enableBmp=1; 
      lps.setLowPower(true);
      lps.setOutputRate(LPS35HW::OutputRate_OneShot);   
      }
#endif  


#ifdef BME
  enableBmp=1; 
  if ((EEPROM.read(13)==255 or EEPROM.read(13)==1) ) {  
  BME680.begin(I2C_STANDARD_MODE);
  BME680.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
  BME680.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
  BME680.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values
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
//GetHumidity();
//GetPressure();



if (EEPROM.read(27)==255 or EEPROM.read(27)==1) {  
    if (EEPROM.read(20)>0 and EEPROM.read(20)<250) {  
      readEEPROMnetwork(20,21,22);
    }   
    if (EEPROM.read(23)>0 and EEPROM.read(23)<250) {  
      readEEPROMnetwork(23,24,25);
    }   
 }  
 else{ 
    //connectGPRS(0); //just connect
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
    else if (EEPROM.read(15)==7 ) { 
      resetReason=87; 
    } 
    else if (EEPROM.read(15)==9 ) { 
      resetReason=89; 
    } 
    else if (EEPROM.read(15)==10 ) { 
      resetReason=90; 
    } 
    else if (EEPROM.read(15)==11 ) { 
      resetReason=91; 
    }       
    else if (EEPROM.read(15)==12 ) { 
      resetReason=92; 
    }
    else if (EEPROM.read(15)==13 ) { 
      resetReason=93; 
    }
    
      else { 
        resetReason=88; 
      }   
    EEPROM.write(15, 0); 
    } 
  } 

delay(7000);
#ifdef DEBUG
  DEBUGSERIAL.println(F("MOD_ST"));
#endif
moduleSetup(); // Establishes first-time serial comm and prints IMEI 
bool checkAT = fona.checkAT();
delay(50);
if (fona.checkAT()) { checkIMEI(); }
if (network1>0  and EEPROM.read(26)!= 1) { 
  EEPROM.write(26,1); 
    #ifdef DEBUG                                 
    DEBUGSERIAL.println("net1: ");
    DEBUGSERIAL.println(network1);
  #endif  
  changeNetwork_id(network1,net_ver1);
  } 
else if (network2>0) { 
  EEPROM.write(26,2); 
    #ifdef DEBUG                                 
    DEBUGSERIAL.println("net2: ");
    DEBUGSERIAL.println(network2);
  #endif
  changeNetwork_id(network2,net_ver2);
  } 

beforeSend();

#ifdef UZ_Anemometer
   #ifdef DEBUG
    DEBUGSERIAL.println(F("startUZ"));
    delay(50);
  #endif 
  unsigned long startedWaiting = millis();
  UZ_wake(startedWaiting);
  if (millis() - startedWaiting <= (65000) ) {
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


  #ifdef DEBUG_MEASURE                                 // debug data
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

#ifndef UZ_Anemometer  
  else { // restart timer in mechanic anemometer
    noInterrupts();
    timergprs = 0;                                
    interrupts();
  }
#endif

#ifdef UZ_Anemometer
  else { // go to sleep 
    ENABLE_UART_START_FRAME_INTERRUPT;
    countWake=0;
    LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
    }    

#endif 
}


void beforeSend() { 
      /////////////////////////// send data to server ///////////////////////////////////////////////  
      digitalWrite(DTR, LOW);  //wake up  
      delay(100); // wait for wake up
      bool checkAT = fona.checkAT();
      SendData();  
      digitalWrite(DTR, HIGH);  //sleep  
      delay(50);

      #ifdef UZ_Anemometer
        if ( changeSleep== 1 and stopSleepChange<3) { //change of sleep time
          ultrasonicFlush();   
          UZsleep(sleepBetween);
        }
      ultrasonicFlush();  
      ENABLE_UART_START_FRAME_INTERRUPT;
      LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
      #endif    
}


void CheckTimerGPRS() { // if unable to send data in 200s
  timergprs++;
    
  if (timergprs > 130 ) {
    #ifdef DEBUG
      DEBUGSERIAL.println(F("hardR"));
    #endif    
    timergprs = 0;
    reset(2);
  }
}

void reset(byte rr) {
  delay(100);
    if (rr > 0 ) { EEPROM.write(15, rr);}
    
    if (rr > 0 and rr!=3 and measureCount > 5) {
      // Write data to EEPROM
        #ifdef DEBUG
          DEBUGSERIAL.println("EEPDATA");
        #endif 
      const int dataSize = sizeof(data) / sizeof(data[0]);
      const int eepromStartAddress = 40;    
      EEPROM.write(39, 1);
      for (int i = 0; i < dataSize; i++) {EEPROM.write(eepromStartAddress + i, data[i]); }    
    }
  delay(100);
  #ifdef DEBUG
    DEBUGSERIAL.print(F("rst: "));
    DEBUGSERIAL.println(rr);
  #endif  
  wdt_enable(WDTO_60MS);
  delay(100);
}



void simReset() {  
  #ifdef DEBUG
    DEBUGSERIAL.println("SIM RST");
  #endif 
    dropConnection(1);  //deactivate PDP, drop GPRS, drop network  
    delay(200);
    fona.reset(); // AT+CFUN=1,1
    delay(300);
    moduleSetup(); // Establishes first-time serial comm and prints IMEI 
    checkIMEI();
    connectGPRS(0); //just connect
}


#ifdef UZ_Anemometer
void UZ_wake(unsigned long startedWaiting) {
  while (!ultrasonic.available() && millis() - startedWaiting <= (65000)) {  // if US not aveliable start it
    ultrasonic.begin(9600);
    delay(5000);
    }      
}
#endif  



ISR(USART1_START_vect){
countWake++;
}

void readEEPROMnetwork(byte ee1,byte ee2, byte ee3) {
    byte lastbyte = EEPROM.read(ee3);
    byte firstpart;
    byte secondpart;
    if (lastbyte <10) {
      secondpart=lastbyte;
      firstpart=0;
    }
    else {
      secondpart = lastbyte%10; 
      firstpart  = (lastbyte/10)%10;
    }
    if (ee1==20) {
    network1=(EEPROM.read(ee1)*1000)+(EEPROM.read(ee2)*10)+firstpart;
    net_ver1=secondpart;
    }
    if (ee1==23) {
    network2=(EEPROM.read(ee1)*1000)+(EEPROM.read(ee2)*10)+firstpart;
    net_ver2=secondpart;
    }

}
