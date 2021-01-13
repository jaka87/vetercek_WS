#include <avr/wdt.h> //watchdog
#include "src/LowPower/LowPower.h" //sleep library
#include <math.h> // wind speed calculations
#include "src/OneWire/OneWire.h" //tmp sensor
#include "src/DS18B20/DallasTemperature.h"
#include "src/TimerOne/TimerOne.h"
#define TINY_GSM_MODEM_SIM7000
#include "src/Fona/Adafruit_FONA.h"
#include "config.h"
#include <EEPROM.h>

#define ONE_WIRE_BUS_1 4 //air
#define ONE_WIRE_BUS_2 3 // water
#define windSensorPin 2 // The pin location of the anemometer sensor
#define windVanePin (A3)       // The pin the wind vane sensor is connected to
#define DTR 6
#define PWRKEY 10
#define FORMAT "id=%s&d=%d&s=%d.%d&g=%d.%d&t=%s&w=%s&b=%d&sig=%d&c=%d&r=%d"
#define FORMAT_URL "vetercek.com/xml/ws_data.php?id=%s&d=%d&s=%d.%d&g=%d.%d&t=%s&w=%s&b=%d&sig=%d&c=%d&r=%d"
byte data[] = { 11,11,11,11,11,11,11,1, 0,0, 0,0, 0,0, 0,0,0, 0,0,0, 0,0,0,0 }; // data

OneWire oneWire_in(ONE_WIRE_BUS_1);
OneWire oneWire_out(ONE_WIRE_BUS_2);
DallasTemperature sensor_air(&oneWire_in);
DallasTemperature sensor_water(&oneWire_out);


#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(8, 9); // RX, TX
//SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

//////////////////////////////////    EDIT THIS
#define APN "internet.simobil.si"
//#define APN "iot.1nce.net"
int cutoffWind = 0; // if wind is below this value time interval is doubled - 2x
int vaneOffset=0; // vane offset for wind dirrection
int whenSend = 40; // interval after how many measurements data is send
const char* broker = "vetercek.com";
//select how to post data - uncomment only one
//#define SEND_MQTT
  const char* MQTTuser = MQTTu;
  const char* MQTTpass = MQTTp;
//#define HTTP
#define UDP
#define DEBUG // comment out if you want to turn off debugging
//////////////////////////////////    RATHER DON'T CHANGE
// MQTT details
char replybuffer[255]; // For reading stuff coming through UART, like subscribed topic messages
char topicGET[20];
const char* topicPUT="ws";
char URL[90]; // Make sure this is long enough
unsigned int pwrAir = 11; // power for air sensor
unsigned int pwrWater = 12; // power for water sensor
int resetReason = MCUSR;
int windDelay = 2300; // time for each anemometer measurement in seconds
byte onOffTmp = 1;   //on/off temperature measure
volatile unsigned long timergprs = 0; // timer to check if GPRS is taking to long to complete
volatile unsigned long rotations; // cup rotation counter used in interrupt routine
volatile unsigned long contactBounceTime; // Timer to avoid contact bounce in interrupt routine
volatile unsigned long lastPulseMillis; // last anemometer measured time
volatile unsigned long firstPulseMillis; // fisrt anemometer measured time
volatile unsigned long currentMillis;
volatile unsigned long MQTTping;
byte firstWindPulse; // ignore 1st anemometer rotation since it didn't make full circle
int windSpeed; // speed
long windAvr = 0; //sum of all wind speed between update
int windGust[3] = { 0, 0, 0 }; // top three gusts
int windGustAvg = 0; //wind gust average
int measureCount = 0; // count each mesurement
byte MQTTcount=0;
float water=99.0; // water Temperature
char wat[6]; // water char value
float temp=99.0; //Stores Temperature value
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
char IMEI[15]; // Use this for device ID
int idd[15];
int sleepBetween=1;
byte sendBatTemp=10;
int GSMstate=13; // default value for network preference - 13 for 2G, 38 for nb-iot and 2 for automatic

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
  else if (EEPROM.read(9)==38) {
    //#define NBIOT
    GSMstate=38; }

  
  //power
  pinMode(PWRKEY, OUTPUT);
  powerOn(); // Power on the module
  delay(4000);
  wakeUp();
  pinMode(DTR, OUTPUT);

  moduleSetup(); // Establishes first-time serial comm and prints IMEI
 
  fona.setFunctionality(0); // AT+CFUN=0
  delay(3000);
  fona.setFunctionality(1); // AT+CFUN=1
  fona.setNetworkSettings(F(APN)); // APN
  delay(200);

  fona.setPreferredMode(GSMstate); 
  delay(500);
  fona.setNetLED(true,3,64,5000);
  delay(500);
  
  //fona.setOperatingBand("NB-IOT",20); // AT&T uses band 12
  fona.enableSleepMode(true);
  delay(200);
  fona.set_eDRX(1, 5, "1001");
  delay(200);
  fona.enablePSM(false);
  //fona.enablePSM(true, "00100001", "00100011");
  delay(200);

  
checkIMEI();
connectGPRS();
digitalWrite(DTR, HIGH);  //sleep

}

void loop() {
  Anemometer();                           // anemometer
  GetWindDirection();



  if ( sleepBetween == 1)  { // to sleap or not to sleap between wind measurement
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

#ifdef SEND_MQTT
  getMQTTmsg(); 
#endif 

  if ( (resetReason==2 and measureCount > 2) or (wind_speed >= (cutoffWind*10) and measureCount >= whenSend) or (measureCount >= (whenSend*2) or whenSend>150) ) {   // check if is time to send data online
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
  if (timergprs > 200) {
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
