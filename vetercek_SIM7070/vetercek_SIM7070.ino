//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xEF:m efuse:w:0xFF:m hfuse:w:DA:m lock:w:0xFF:m 
// 57600 max baud rate = AT+IPR=57600
// before uploading skech burn bootloader
// hardware serial buffer 128b 

//ultrasonic anemometer To active ASCI.
//>UartPro:0\r\n or 0
//>SaveConfig\r\n
//>ComMode:0\r\n 0 232/1 48r5

//PCB 0.6.4 and up

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
char* broker = "10.64.124.253";
#define DEVICE_ID 1   

byte GSMstate=2; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 (2g with nb-iot as backup) and 51 (nb-iot with 2g as backup)
byte cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 3; // interval after how many measurements data is send
int sea_level_m=0; // enter elevation for your location for pressure calculation
/////////////////////////////////    OPTIONS TO TURN ON AN OFF
//#define DEBUG // comment out if you want to turn off debugging
//#define DEBUG2 // comment out if you want to turn off SIM debugging
//#define DEBUG_MEASURE  // debug data
//#define DEBUG_ERROR  // debug connection,DEBUG_ERROR not written to serial but as reset reason
#define LOCAL_WS // comment out if the station is global - shown on windgust.eu
#define UZ_Anemometer // if ultrasonic anemometer - PCB minimum PCB v.0.5
#define toggle_UZ_power // toggle ultrasonic power - PCB minimum PCB v.0.6.6

//#define UZ_old // if ultrasonic anemometer - PCB minimum PCB v.0.5
//#define BMP // comment out if you want to turn off pressure sensor and save space
#define HUMIDITY 31 // 31 or 41 or comment out if you want to turn off humidity sensor
//#define TMPDS18B20 // comment out if you want to turn off temerature sensor
//#define BME // comment out if you want to turn off pressure and humidity sensor
//#define TMP_POWER_ONOFF // comment out if you want power to be on all the time
#define ANEMOMETER 1 //1 Davis // 2 - chinese 20 pulses per rotation //3 - custom 1 pulses per rotation
#define ANEMOMETER_DEBOUNCE 15 // 15 davis anemometer, 4 chinese with 20 pulses
#define GSM

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
  // 11. Spain
///////////////////////////////////////////////////////////////////////////////////

#if ANEMOMETER == 1 //Davis mechanical anemometer
  #define windFactor 19555.96 
#elif ANEMOMETER == 2
 #define windFactor 1700.086 //chinese with 20 pulses per turn
#elif ANEMOMETER == 3
  #define windFactor 34001.72 // custom 1 pulses per rotation
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
// USART0 (GSM)
#define IGNORE_GSM_DATA UCSR0B &= ~((1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0));  // Disable RX, TX, and RX interrupt
#define ENABLE_GSM_DATA UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);    // Enable RX, TX, and RX interrupt

#define TX1_PIN 17  // PD3 = TX1 on ATmega328PB
// Completely disable UART1 and disconnect TX
#define IGNORE_UZ_DATA UCSR1B &= ~((1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1));   // TX1 high-impedance
// Re-enable UART1 and reconnect TX
#define ENABLE_UZ_DATA UCSR1B |= (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1)
// Disable only TX (float TX pin to prevent sending)
#define DISABLE_TX1 UCSR1B &= ~(1 << TXEN1); pinMode(TX1_PIN, INPUT)
// Enable only TX (to send a command to the anemometer)
#define ENABLE_TX1 pinMode(TX1_PIN, OUTPUT); UCSR1B |= (1 << TXEN1)



byte data[] = { DEVICE_ID & 0xFF, (DEVICE_ID >> 8) & 0xFF, 0,0, 0,0, 0,0, 0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0 }; // data


#ifdef TMPDS18B20
  #include "src/OneWire/OneWire.h" //tmp sensor
  #include "src/DS18B20/DallasTemperature.h"
  OneWire oneWire_in(ONE_WIRE_BUS_1);   //tmp
  DallasTemperature sensor_air(&oneWire_in);
  OneWire oneWire_out(ONE_WIRE_BUS_2);
  DallasTemperature sensor_water(&oneWire_out);
#endif
//////////////////////////////////    EEPROM DATA
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
// 94 - 3x failres



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

#if defined HUMIDITY 
  #include "Wire.h"
#endif




#ifdef HUMIDITY
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

volatile int rainCount=0; // count rain bucket tilts
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
byte UltrasonicAnemo=0;
byte enableSolar=1;
byte enableRain=0; //disable if water
byte enableBmp=0;
byte enableHum=0;
int pressure=0;
byte humidity=0;
byte changeSleep=0;
byte batteryState=0; // 0 normal; 1 low battery; 2 very low battery
byte stopSleepChange=0; //on
volatile byte countWake = 0;
byte checkServernum=0;
byte sendError=0;
bool uzInitialized = false;


#if NETWORK_OPERATORS == 1
  int network1=29340; //A1
  int network2=29370; //telemach
  byte net_ver1=9;
  byte net_ver2=0;
#elif NETWORK_OPERATORS == 2
  int network1=21901; //H-telekom
  int network2=21902; //a1
  byte net_ver1=0;
  byte net_ver2=9;
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
#elif NETWORK_OPERATORS == 11
  int network1=21401;
  int network2=21403;
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

  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);    // turn off UZ anemometer


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
  
  //#ifdef UZ_Anemometer
  //  ENABLE_UART_START_FRAME_INTERRUPT;
  //#endif


  
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
//  if (EEPROM.read(10)==0 or enableRain==1) { attachInterrupt(digitalPinToInterrupt(3), rain_count, FALLING); enableRain=1;} // rain counts
  if (EEPROM.read(10)==0 or enableRain==1) { attachInterrupt(digitalPinToInterrupt(3), rain_count, RISING); enableRain=1;} // rain counts
  #ifdef TMPDS18B20
    else { sensor_water.begin(); } // water temperature
  #endif

  int eepromValue9 = EEPROM.read(9);
  int eepromValue14 = EEPROM.read(14);
  
  if (eepromValue9 == 13) { 
      GSMstate = 13; 
  } else if (eepromValue9 == 2) { 
      GSMstate = 2; 
  } else if (eepromValue9 == 38) { 
      GSMstate = 38; // #define NBIOT
  } else if (eepromValue9 == 51) { 
      GSMstate = 51; // #define NBIOT or 2G
  }
  
  if (eepromValue14 == 10) { 
      stopSleepChange = 10; // UZ sleep on/off
  }
  

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


#if defined(HUMIDITY) 
    Wire.begin();
#endif





#ifdef HUMIDITY
  #if HUMIDITY == 31
    //sht.begin(SHT_ADDRESS);
    SHT31 sht(SHT_ADDRESS);

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


int eepromValue27 = EEPROM.read(27);
if (eepromValue27 == 255 || eepromValue27 == 1) {  
    int eepromValue20 = EEPROM.read(20);
    int eepromValue23 = EEPROM.read(23);

    if (eepromValue20 > 0 && eepromValue20 < 250) {  
        readEEPROMnetwork(20, 21, 22);
    }   
    if (eepromValue23 > 0 && eepromValue23 < 250) {  
        readEEPROMnetwork(23, 24, 25);
    }   
} 

if (resetReason == 8) { //////////////////// reset reason detailed
    int eepromValue15 = EEPROM.read(15);
    if (eepromValue15 > 0) {
        if (eepromValue15 >= 1 && eepromValue15 <= 15) {
            resetReason = 80 + eepromValue15; // Map directly to 81–95
        } else {
            resetReason = 88; // Default case
        }
        EEPROM.write(15, 0); // Reset EEPROM value
    }
}

#ifdef GSM 
delay(7000);
#ifdef DEBUG
  DEBUGSERIAL.println(F("MOD_ST"));
#endif
moduleSetup(); // Establishes first-time serial comm and prints IMEI 
bool checkAT = fona.checkAT();
delay(100);
#ifdef DEBUG
  DEBUGSERIAL.println(checkAT);
#endif


//r change network
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
IGNORE_GSM_DATA;
#endif



#ifdef UZ_Anemometer
  UZ_wake(); 
  LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
#endif 
}

void loop() {
     
#ifdef UZ_Anemometer
  DISABLE_UART_START_FRAME_INTERRUPT;    
  unsigned long startedWaiting = millis();
  
  #ifdef UZ_old
    while (ultrasonic.available() < 2 && millis() - startedWaiting <= 15000) {  delay(10);}
    if (ultrasonic.available() < 2 ) { // sleep while receiving data and anemometer sleep time 3s or more
          LowPower.idle(SLEEP_2S, ADC_OFF, TIMER4_OFF,TIMER3_OFF,TIMER2_ON, TIMER1_OFF, TIMER0_OFF,SPI1_OFF,SPI0_OFF,USART1_ON, USART0_OFF, TWI1_OFF,TWI0_OFF,PTC_OFF);
    }
    else { // sleep while receiving data and anemometer sleep time 2s or less 
          LowPower.idle(SLEEP_60MS, ADC_OFF, TIMER4_OFF,TIMER3_OFF,TIMER2_ON, TIMER1_OFF, TIMER0_OFF,SPI1_OFF,SPI0_OFF,USART1_ON, USART0_OFF, TWI1_OFF,TWI0_OFF,PTC_OFF);
    }
    while (ultrasonic.read() != ',' and millis() - startedWaiting <= 7000) {  } 
  #else
    LowPower.idle(SLEEP_1S, ADC_OFF, TIMER4_OFF,TIMER3_OFF,TIMER2_ON, TIMER1_OFF, TIMER0_OFF,SPI1_OFF,SPI0_OFF,USART1_ON, USART0_OFF, TWI1_OFF,TWI0_OFF,PTC_OFF);
  
        //delay(15);
//        #ifdef DEBUG
//          DEBUGSERIAL.println(ultrasonic.available());
//        #endif
          //if (ultrasonic.available() ==1 ) { // sleep while receiving data and anemometer sleep time 3s or more
           // LowPower.idle(SLEEP_1S, ADC_OFF, TIMER4_OFF,TIMER3_OFF,TIMER2_ON, TIMER1_OFF, TIMER0_OFF,SPI1_OFF,SPI0_OFF,USART1_ON, USART0_OFF, TWI1_OFF,TWI0_OFF,PTC_OFF);
          //}
      //while (ultrasonic.read() != ',' and millis() - startedWaiting <= 7000) { delay(10); } 

//while (ultrasonic.read() != ',' and millis() - startedWaiting <= 7000) { delay(10); } 
while (millis() - startedWaiting <= 1000) {
  char c = ultrasonic.read();
  if (c == ',') {
    break;
  }
  #ifdef DEBUG
    if (c >= 32 && c <= 126) {  // Printable ASCII range
      DEBUGSERIAL.println(c);
    }
  #endif
  delay(10);
}


  #endif 

    delay(90);
  if (ultrasonic.available()>61){  
    UltrasonicAnemometer();
    #ifdef DEBUG
      unsigned long totalTime = millis() - startedWaiting;
      DEBUGSERIAL.print(F("UZ time: "));
      DEBUGSERIAL.println(totalTime);
    #endif
    } 
  else {  
    UZerror(1);
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
    DEBUGSERIAL.println(rainCount);
  #endif

  GetAvgWInd();                                 // avg wind


  digitalWrite(13, HIGH);   // turn the LED on
  delay(15);                       // wait
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
    ENABLE_UART_START_FRAME_INTERRUPT;
    countWake=0;
    LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF,TIMER2_ON);  // sleep  
    }    

#endif 
}

void beforeSend() {
  /////////////////////////// send data to server ///////////////////////////////////////////////
  //ultrasonic.end();
  IGNORE_UZ_DATA;
  ENABLE_GSM_DATA;
  digitalWrite(DTR, LOW);  // wake up
  delay(100);

  bool sendSuccess = false;
  int attempts = 0;

  while (!sendSuccess && attempts < 3) {
    sendSuccess = SendData();
    attempts++;
    if (!sendSuccess) {
      delay(1000);  // Optional delay between retries
    }
  }



  if (sendSuccess) {
  digitalWrite(DTR, HIGH);  // sleep
  delay(50);
  IGNORE_GSM_DATA;
  ENABLE_UZ_DATA;
  //UZ_wake(); 
  
  #ifdef UZ_Anemometer
    if (UltrasonicAnemo == 1) {
      if (changeSleep == 1 && stopSleepChange < 3) {  // change of sleep time
        UZsleep(sleepBetween);
      }
    }
    #endif


    // ultrasonicFlush();
    ENABLE_UART_START_FRAME_INTERRUPT;
    LowPower.powerExtStandby(SLEEP_8S, ADC_OFF, BOD_OFF, TIMER2_ON);  // sleep
  }
  else {
    reset(14);
  }
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


  #ifdef UZ_Anemometer
    //IGNORE_UZ_DATA;
    ultrasonic.end();
  #endif 

    
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
    connectGPRS(0); //just connect
}



#ifdef UZ_Anemometer
void UZ_wake() {
    #ifdef toggle_UZ_power
      digitalWrite(26, HIGH);  
      delay(1000); 
    #endif


  if (uzInitialized) {
    #ifdef DEBUG
      DEBUGSERIAL.println(F("UZze"));
      delay(50);
    #endif
    return; // Skip if already initialized
  }

  #ifdef DEBUG
    DEBUGSERIAL.println(F("startUZ"));
    delay(50);
  #endif

  // Try to flush and reset the hardware serial buffer
  while (ultrasonic.available()) {
    ultrasonic.read(); // Clear any junk data
  }

  ultrasonic.end();     // Stop serial port
  delay(100);           // Give hardware time to release resources
  ultrasonic.begin(9600); // Reopen with proper baud rate
  DISABLE_TX1;

  unsigned long startedWaiting = millis();

  while (!ultrasonic.available() && millis() - startedWaiting <= 65000) {
    delay(1000);
  }

  if (millis() - startedWaiting <= 65000) {
    UltrasonicAnemo = 1;
    uzInitialized = true;
    windDelay = 1000;
    ultrasonicFlush();

    #ifdef DEBUG
      DEBUGSERIAL.println(F("UZ"));
      delay(50);
    #endif
  } else {
    #ifdef DEBUG
      DEBUGSERIAL.println(F("UZ fail"));
      delay(50);
    #endif

    reset(7); // reset board if UZ not available
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
