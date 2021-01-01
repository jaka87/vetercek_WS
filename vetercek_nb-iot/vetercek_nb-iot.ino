#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include "src/OneWire/OneWire.h" //tmp sensor
#include "src/DS18B20/DS18B20.h"
#include "src/TimerOne/TimerOne.h"
#include "src/PubSubClient/PubSubClient.h"
#define TINY_GSM_MODEM_SIM7000
#include "src/TinyGsmClient/TinyGsmClient.h"
#include "config.h"

#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define windVanePin (A3)       // The pin the wind vane sensor is connected to
#define DTR 6
#define PWRKEY 10
#define FORMAT "id=%s&d=%d&s=%d.%d&g=%d.%d&t=%s&w=%s&b=%d&sig=%d&c=%d&r=%d"
OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DS18B20 sensor_air(&oneWire_in);
DS18B20 sensor_water(&oneWire_out);

// Select your modem:
#define Serial Serial
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(8, 9); // RX, TX
// Define the serial console for debug prints, if needed
//#define TINY_GSM_DEBUG Serial
// Range to attempt to autobaud
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 9600

//////////////////////////////////    EDIT THIS
// Your GPRS credentials, if any
const char* apn = "internet";
int cutoffWind = 3; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 33; // interval after how many measurements data is send
const char* MTTTuser = MQTTu;
const char* MQTTpass = MQTTp;
#define DUMP_AT_COMMANDS // See all AT commands, if wanted
#define DEBUG // comment out if you want to turn off debugging
//////////////////////////////////    RATHER DON'T CHANGE
// MQTT details
const char* broker = "vetercek.com";
char topicGET[20];
const char* topicPUT="ws";
byte GPRSstate=0;
char URL[90]; // Make sure this is long enough
unsigned int pwrAir = 11; // power for air sensor
unsigned int pwrWater = 12; // power for water sensor
byte resetReason = MCUSR;
int windDelay = 2300; // time for each anemometer measurement in seconds
byte onOffTmp = 1;   //on/off temperature measure
volatile unsigned long timergprs = 0; // timer to check if GPRS is taking to long to complete
volatile unsigned long rotations; // cup rotation counter used in interrupt routine
volatile unsigned long contactBounceTime; // Timer to avoid contact bounce in interrupt routine
volatile unsigned long lastPulseMillis; // last anemometer measured time
volatile unsigned long firstPulseMillis; // fisrt anemometer measured time
volatile unsigned long currentMillis;
volatile unsigned long timeoutGPRS = 0;
byte firstWindPulse; // ignore 1st anemometer rotation since it didn't make full circle
int windSpeed; // speed
long windAvr = 0; //sum of all wind speed between update
int windGust[3] = { 0, 0, 0 }; // top three gusts
int windGustAvg = 0; //wind gust average
int measureCount = 0; // count each mesurement
float water; // water Temperature
char wat[6]; // water char value
float temp; //Stores Temperature value
char tmp[6]; // Temperature char value
int vaneValue;       // raw analog value from wind vane
int direction;       // translated 0 - 360 direction
int calDirection;    // converted value with offset applied
int windDir;  //calculated wind direction
float windAvgX; //x and y wind dirrection component
float windAvgY;
int wind_speed;  //calculated wind speed
int wind_gust;   //calculated wind gusts
int battLevel = 0; // Battery level (percentage)
unsigned int sig = 0;
float actualWindDelay; //time between first and last measured anemometer rotation
String IMEI;
const char*id;
int attempts=0;

#ifdef DUMP_AT_COMMANDS
  #include "src/StreamDebugger/StreamDebugger.h"
  StreamDebugger debugger(SerialAT, Serial);
  TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);




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
  
  //power
  pinMode(PWRKEY, OUTPUT);
  powerOn(); // Power on the module
  pinMode(DTR, OUTPUT);

  // Set GSM module baud rate
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  SerialAT.begin(9600);
  delay(5000);

  //modem.restart();
  modem.init();
  delay(3000);
  modem.sendAT("+CNETLIGHT=1"); // network LED light turn on
  modem.waitResponse(500);
  modem.sendAT("+SLEDS=3,64,3000"); // network LED light set timer
  modem.waitResponse(500);
  modem.sendAT("+CSGS=2"); // network LED light use timer
  modem.waitResponse(500);

  modem.sleepEnable(true); // sleep
  //modem.sendAT("+CPSMS=1"); // PSM
  //modem.waitResponse(500);
  modem.sendAT("+cpsms=1,,,\"00101110\",\"00000101\""); // PSM
  modem.waitResponse(500);
  //modem.sendAT("+CPSMS?"); // PSM
  //modem.waitResponse(500);

  IMEI=modem.getIMEI();
  id = IMEI.c_str();

  modem.setNetworkMode(2);  //2 automatic, 13 GSM, 38 nb-iot
  //modem.setPreferredMode(2); // 2 nb-iot

  sprintf(topicGET, "get/%s", id);  
  setupGSM();
  initMQTT();
  digitalWrite(DTR, HIGH);  //sleep

}

void loop() {
  Anemometer();                           // anemometer
  GetWindDirection();

  if ( whenSend>19)  { // don't sleep if whensend <20  for nb-iot
    LowPower.powerDown(SLEEP_8S, ADC_ON, BOD_ON);  // sleep
  }
  #ifdef DEBUG                                 // debug data
    //Serial.print(" rot:");
    //Serial.print(rotations);
    //Serial.print(" delay:");
    //Serial.print(actualWindDelay);
    //Serial.print(" dir:");
    //Serial.print(calDirection);
    Serial.print(" speed:");
    Serial.print(windSpeed);
    //Serial.print(" gust:");
    //Serial.print(windGustAvg);
    Serial.print(" next:");
    Serial.print(whenSend - measureCount);
    Serial.print(" count:");
    Serial.println(measureCount);
    Serial.println("");
  #endif

  GetAvgWInd();                                 // avg wind

  if ( (resetReason==2 and measureCount > 3) or (wind_speed >= (cutoffWind*10) and measureCount >= whenSend) or (measureCount >= (whenSend*2) or whenSend>150) ) {   // check if is time to send data online
       digitalWrite(DTR, LOW);  //wake up
      //modem.sleepEnable(false); 
      delay(100);
    SendData();
      digitalWrite(DTR, HIGH);  //sleep
      //modem.sleepEnable(true);

  }
  else { // check if is time to send data online
  #ifdef DEBUG
      //Serial.print("tim: ");
      Serial.println(timergprs);
  #endif
    noInterrupts();
    timergprs = 0;                                // reset timer 
    interrupts();
  }
}


void CheckTimerGPRS() { // if unable to send data in 100s
  timergprs++;
  if (timergprs > 200) {
    timergprs = 0;
    powerOff();
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
  modem.poweroff();
  powerOff(); // turn off power
  delay(1000);
  wdt_enable(WDTO_1S);
}



// Power on the module
void powerOn() {
  digitalWrite(PWRKEY, LOW);
  delay(100); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
  delay(1000);
}

void powerOff() {
  digitalWrite(PWRKEY, LOW);
  delay(3000); // For SIM7000 
  digitalWrite(PWRKEY, HIGH);
  delay(1000);
}
