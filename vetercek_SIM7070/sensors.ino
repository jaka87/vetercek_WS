#ifdef UZ_Anemometer
void UltrasonicAnemometer() { //measure wind speed

// first: :1,0,0.00,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,E5
// second: :01,0,0.00,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,BE
// latest: :01,0,0.00,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,0,,23

    char buffer[75];
    char hexbuffer[5];
    char hexbuffer2[5]; // new anemometer with aditional 0 in string
    int sum;
    int sum2; // new anemometer with aditional 0 in string
    unsigned long startedWaiting = millis();
    unsigned long startTime = millis(); // Get the start time
    int size = 0;
    while ((millis() - startTime) < 30000) {  // 30 seconds = 30000 milliseconds
        // Check if data is available to read
        if (ultrasonic.available()) {
            size = ultrasonic.readBytesUntil('\r\n', buffer, 75);
            buffer[size] = '\0'; // Null-terminate the string
            break; // Exit the loop once data is read
        }
        delay(10); // Small delay to prevent busy-waiting (optional)
    }
    
    if ((millis() - startTime) >= 30000) {
        // Timeout occurred
        UZerror(1); // Handle timeout error (1 indicates timeout for this case)
        return;
    }
              
      delay(20); // important in case of error          
      char *dir = strtok(buffer, ",/");
      char *wind = strtok(NULL, ",/");
      char* check = strtok(NULL, ",/");
      char* check2 = strtok(NULL, ",/");


      if( check!=NULL and wind!=NULL and dir!=NULL)  {    

        if( check2!=NULL)  {  check=check2;  }
        sum+=countBytes(dir);
        sum+=countBytes(wind);
        sum+=2605;
        if( check2!=NULL)  {  sum2=sum-62;  } //2781
        else {  sum2=sum+48;  }
        
        sum=-(sum % 256);   
        sum2=-(sum2 % 256);   
        sprintf(hexbuffer,"%02X", sum);
        sprintf(hexbuffer2,"%02X", sum2); // new anemometer with aditional 0 in string
   
        if( (check[0] ==hexbuffer[2] and check[1] ==hexbuffer[3]) or (check[0] ==hexbuffer2[2] and check[1] ==hexbuffer2[3]) )  {  
            calDirection = atoi(dir) + vaneOffset;
            
            #if defined(COMPASS)
              int heading = getCompassHeading();
              calDirection=calDirection+heading;
              if (calDirection >= 360) {
                  calDirection -= 360;  // Not executed because calDirection = 135
              } else if (calDirection < 0) {
                  calDirection += 360;  // Not executed because calDirection = 135
              }
            #endif 
            
            CalculateWindDirection();  // calculate wind direction from data
            windSpeed=atof(wind)*19.4384449;
            CalculateWindGust(windSpeed);
            CalculateWind();
            if ( (wind_speed >= (cutoffWind*10) and measureCount <= whenSend )  or (wind_speed < (cutoffWind*10) and measureCount <= (whenSend*2))  ) {  // uz dont reset timer if more measurement than needed
              timergprs = 0;  
            } 
                                           
        }
        else { 
            UZerror(3); 
          }        
      }
      else { 
        UZerror(2); 
        }      

ultrasonicFlush();

}


int countBytes( const char * data )
{
  int total = 0;
  const char * p = data;
  while ( *p != '\0' )
  {
    total += *p++;
  }
  return total;
}

void UZerror(byte where) { //ultrasonic error
      ultrasonicFlush();
  sonicError++;
  #ifdef DEBUG
      DEBUGSERIAL.print(F("err UZ "));
      DEBUGSERIAL.println(where);
  #endif

  #ifdef DEBUG_ERROR
    resetReason=55;  
  #endif

if ( sonicError ==3)  { ultrasonic.end(); delay(2000); ultrasonic.begin(9600);delay(5000);  }  
if ( sonicError >=7)  { reset(4);  }   // if more than x US errors 
}

void UZsleep(byte sleepT) { //ultrasonic anemometer sleep mode
  char buffer[25];
  char buffer2[80];
  byte slponoff=1;
  
  if (sleepT==0){slponoff=0;}
  sprintf(buffer, ">PwrIdleCfg:%d,%d\r\n", slponoff,sleepT);  

while (ultrasonic.available() <2) {  delay(10); } 
  unsigned long startedWaiting = millis();   
  #ifdef DEBUG
      DEBUGSERIAL.println(F("UZzzz"));
      delay(20);
  #endif

  byte trow_away;
  trow_away=(ultrasonic.read());
  int size = ultrasonic.readBytesUntil('\n', buffer2, 70);
  buffer[size]='\0'; 

  while (strstr (buffer2,"IdleSec") == NULL && millis() - startedWaiting <= 20000) {
    trow_away=(ultrasonic.read());
    size = ultrasonic.readBytesUntil('\n', buffer2, 70); 
    buffer2[size]='\0';   
    ultrasonic.write(buffer);  
    delay(100);
     #ifdef DEBUG 
      DEBUGSERIAL.println(F("slptry")); 
      delay(10);
     #endif 
    }

    if(millis() - startedWaiting < 19900){ ultrasonic.write(">SaveConfig\r\n"); }
    if(millis() - startedWaiting < 19900){ 
      sleepBetween=sleepT;
      changeSleep=0;
      stopSleepChange=0;
     #ifdef DEBUG 
      DEBUGSERIAL.print(F("sleepcok ")); 
      DEBUGSERIAL.println(sleepT); 
      delay(10);
     #endif 
      }
    else { 
     stopSleepChange++;
     #ifdef DEBUG 
      DEBUGSERIAL.println(F("err sleepc")); 
     #endif       
      }
}
#endif 

#ifndef UZ_Anemometer
  void Anemometer() { //measure wind speed
    float actualWindDelay; //time between first and last measured anemometer rotation
    firstWindPulse = 1; // dont count first rotation
    contactBounceTime = millis();
    rotations = 0; // Set rotations count to 0 ready for calculations
    EIFR = (1 << INTF0); // clear interrupt flag
    attachInterrupt(digitalPinToInterrupt(windSensorPin), ISRrotation, FALLING); //setup interrupt on anemometer input pin, interrupt will occur whenever falling edge is detected
    delay (windDelay); // Wait x second to average
    detachInterrupt(digitalPinToInterrupt(windSensorPin));
  
  if (rotations == 0)  {
    windSpeed = 0;
  }
  else  {
    actualWindDelay = (lastPulseMillis - firstPulseMillis);
      windSpeed = (rotations * windFactor) / actualWindDelay;  //chinese anemometer
   }

  if (windSpeed < 600) { // delete data larger than 60KT
      CalculateWind();
      CalculateWindGust(windSpeed);
  }

}
#endif

void CalculateWind() {  
    ++measureCount; // add +1 to counter
    windAvr += windSpeed; // add to sum of average wind values
}


void CalculateWindGust( int w ) {
      if (w >= windGust[2]) { // check if > than old gust3 of wind
        windGust[0] = windGust[1];
        windGust[1] = windGust[2];
        windGust[2] = w;
      }
    
      else if (w >= windGust[1]) { // check if > than old gust2 of wind
        windGust[0] = windGust[1];
        windGust[1] = w;
      }
    
      else if (w > windGust[0]) { // check if > than old gust1 of wind
        windGust[0] = w;
      }
      windGustAvg = (windGust[0] + windGust[1] + windGust[2]) / 3;
}

void GetAvgWInd() {
  wind_speed = windAvr / measureCount; // calculate average wind
}


#ifndef UZ_Anemometer
  void ISRrotation () {  // This is the function that the interrupt calls to increment the rotation count
  currentMillis = millis(); //we have to read millis at the same position in ISR each time to get the most accurate readings
  if (firstWindPulse == 1) { //discard first pulse as we don't know exactly when it happened
    contactBounceTime = currentMillis;
    firstWindPulse = 0;
    firstPulseMillis = currentMillis;
  }

    else if ((currentMillis - contactBounceTime) > ANEMOMETER_DEBOUNCE ) { // debounce the switch contact
    rotations++;
    contactBounceTime = currentMillis;
    lastPulseMillis = currentMillis;
  }
}
#endif

void DominantDirection() { // get dominant wind direction
  windDir =  atan2(windAvgY, windAvgX) / PI * 180;
  if (windDir < 0) windDir += 360;
}

// Get Wind direction, and split it in 16 parts and save it to array
void GetWindDirection() {
  vaneValue = analogRead(PIN_A3);

#if ANEMOMETER == 1 //Davis mechanical anemometer
  direction = map(vaneValue, 0, 1023, 0, 360);
#else
  float actualReferenceVoltage = 4.08;  // Measured voltage reference
  float voltage = (vaneValue * actualReferenceVoltage) / 1023.0;    
    // Calculate the angle (0 to 360 degrees based on voltage)
  direction = (voltage / actualReferenceVoltage) * 360.0;  // Assuming 5V supply for AS5600
#endif
    
  calDirection = direction + vaneOffset;
  CalculateWindDirection();
}

void CalculateWindDirection() {
   // convert reading to radians
  float theta = calDirection / 180.0 * PI;
  // running average
  windAvgX = windAvgX * .75 + cos(theta) * .25;
  windAvgY = windAvgY * .75 + sin(theta) * .25;
}

void rain_count() {
  currentMillis2 = millis(); //we have to read millis at the same position in ISR each time to get the most accurate readings
if ((currentMillis2 - contactBounceTime2) > 350 ) { // debounce the switch contact.
    contactBounceTime2 = currentMillis2;
    rainCount++;
  digitalWrite(13, HIGH);   // turn the LED on
  delay(15);                       // wait
  digitalWrite(13, LOW);    // turn the LED
  }
}

#ifdef TMPDS18B20
void GetAir() {
  unsigned long startedWaiting = millis();    
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrAir, HIGH);   // turn on power
  delay(100);
#endif  
  sensor_air.requestTemperatures(); // Send the command to get temperatures
  while (!sensor_air.isConversionComplete() and millis()-startedWaiting<900);
  temp = sensor_air.getTempCByIndex(0);
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrAir, LOW);   // turn off power
#endif

#ifdef DEBUG
  DEBUGSERIAL.print(F("tmp: "));
  DEBUGSERIAL.println(temp);
#endif
}


void GetWater() {
  unsigned long startedWaiting = millis();    
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrWater, HIGH);   // turn on power
  delay(100);
#endif  
  sensor_water.requestTemperatures(); // Send the command to get temperatures
  while (!sensor_water.isConversionComplete() and millis()-startedWaiting<1500);  
  water = sensor_water.getTempCByIndex(0);
#ifdef TMP_POWER_ONOFF
  digitalWrite(pwrWater, LOW);   // turn off power
#endif

#ifdef DEBUG
    DEBUGSERIAL.print(F("water: "));
    DEBUGSERIAL.println(water);
#endif
}
#endif

#ifdef BMP
void GetPressure() {
  float abs_pressure;  
  lps.requestOneShot();  // important to request new data before reading
  delay(100);
  abs_pressure = lps.readPressure();  // hPa
  if (sea_level_m > 0) { 
    if (temp> -30) { 
      pressure=(abs_pressure / pow(1.0 - 0.0065 * sea_level_m / (temp  + 273.15), 5.255))*10;  // ICAO formula
      }
    else { 
      temp=lps.readTemp();
      pressure=(abs_pressure / pow(1.0 - 0.0065 * sea_level_m / (temp  + 273.15), 5.255))*10;  // ICAO formula
      }
  }
  else
  { 
      pressure=abs_pressure*10; 
  }
}
#endif

#ifdef BME
void GetPressure() {

  static int32_t  temp2, humi, abs_pressure;  // BME readings
  BME680.getSensorData(temp2, humi, abs_pressure);  // Get readings

    humidity=humi / 1000;
    temp=temp2/100;
    pressure=((abs_pressure/100) / pow(1.0 - 0.0065 * sea_level_m / ((temp2/100)  + 273.15), 5.255))*10;  // ICAO formula

       #ifdef DEBUG 
       delay(20);
        DEBUGSERIAL.println("hum "); 
        DEBUGSERIAL.println(pressure); 
        DEBUGSERIAL.println(humidity); 
       delay(20);
       #endif 
   
}
#endif

#ifdef HUMIDITY
  void GetHumidity() {
  #if HUMIDITY == 31    
    if ( sht.isConnected() ){
      sht.read();         // default = true/fast       slow = false
      temp=sht.getTemperature();
      humidity=sht.getHumidity();
  #else
    if (sht.measure() == SHT4X_STATUS_OK) {
     if (sht.TcrcOK) {temp=sht.TtoDegC();} 
     if (sht.RHcrcOK) { humidity=sht.RHtoPercent();} 
  #endif 
     #ifdef DEBUG
      DEBUGSERIAL.print(F("hum: "));
      DEBUGSERIAL.println(humidity);
    #endif 
    }
  }

#endif

void GetTmpNow() {
#ifdef TMPDS18B20
  if (onOffTmp == 1) {
    GetAir();                               // air
    data[18]=99;
    delay(20);
  }
  else if (onOffTmp == 2) {    
    if (enableRain==0){  
      GetWater();                             // water
    }          
    data[19]=99;
    delay(20);
  }
  else if (onOffTmp > 2) {
    GetAir();                               // air
    if (enableRain==0){  
      GetWater();                             // water
    }          
    delay(20);
  }
#endif 

  #ifdef BMP
    if (enableBmp == 1) {
      GetPressure();
      delay(20);
    }
  #endif 

  #ifdef HUMIDITY
    if (enableHum == 1) {
      GetHumidity();
      delay(20);
    }
  #endif 
  
  #ifdef BME
    if (enableBmp == 1) {
      GetPressure();
      delay(20);
    }
  #endif 
   
}


void BeforePostCalculations( byte kind) {
  DominantDirection();                          // wind direction
  GetAvgWInd();                                 // avg wind

  if (kind==1){
    if (enableSolar==1){
      int curr = 0;  // measure solar cell current
      volatile unsigned currCount = 0;
      while (currCount < 10) {
            curr += ((analogRead(A0)*3.98)/1000/1.15)*940; //2.2k resistor
            currCount++;
            delay(20);
        }
      SolarCurrent=(curr/currCount)/5;  // calculate average solar current // divide with 5 so it can be send as byte
      }     
    GetTmpNow(); 
  }     
}

// Read the module's power supply voltage
float readVcc() {
    // Read battery voltage
  if (fona.getBattVoltage(&battVoltage)) {
    battLevel=battVoltage/20; // voltage  
  }
  else {
    battLevel=0;
  }
  return battLevel;
}

#ifdef UZ_Anemometer
void ultrasonicFlush(){
  while(ultrasonic.available() > 0 ) {
    char t = ultrasonic.read();
  }
}

  
#endif 




#if defined(COMPASS)


// Function to handle compass data reading and heading calculation
int getCompassHeading() {
  int x, y, z;

  // Start communication with the compass sensor
  Wire.beginTransmission(0x1E);
  Wire.write(0x03);  // Start reading from the data register
  Wire.endTransmission();

  // Request 6 bytes from the compass sensor (X, Y, Z axes data)
  Wire.requestFrom(0x1E, 6);

  // Check if we received all the required bytes
  if (Wire.available() == 6) {
    x = (Wire.read() << 8) | Wire.read();  // X-axis data
    y = (Wire.read() << 8) | Wire.read();  // Y-axis data
    z = (Wire.read() << 8) | Wire.read();  // Z-axis data (if needed)
  } else {
    // If no data is available, return an invalid value
    return -1.0;  // Error value
  }

  // Calculate the heading in degrees
  int heading = atan2((float)y, (float)x) * 180 / PI;

  // Correct for when the heading is negative
  if (heading < 0) {
    heading += 360;
  }

  return heading;  // Return the calculated heading
}


void initCompass() {
  Wire.beginTransmission(0x1E);
  Wire.write(0x00);  // Write to configuration register A
  Wire.write(0x70);  // 8-average, 15 Hz default, normal measurement
  Wire.endTransmission();
  
  Wire.beginTransmission(0x1E);
  Wire.write(0x01);  // Write to configuration register B
  Wire.write(0xA0);  // Gain = 5
  Wire.endTransmission();

  Wire.beginTransmission(0x1E);
  Wire.write(0x02);  // Select mode register
  Wire.write(0x00);  // Continuous measurement mode
  Wire.endTransmission();
}

#endif
