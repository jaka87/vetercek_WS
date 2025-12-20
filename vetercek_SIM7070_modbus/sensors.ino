
// === CRC16 Modbus Function ===
uint16_t crc16(const uint8_t *data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}

// Union for converting bytes to float
union FloatUnion {
  float f;
  uint8_t b[4];
};

FloatUnion floatUnion; // Create an instance

void UltrasonicAnemometer() { //measure wind speed using Modbus RTU

    uint8_t device_id = 1;
    uint8_t function_code = 3;
    uint16_t start_address = 0x0001; // Start from register 2 (Wind Direction)
    uint16_t num_registers = 3;      // Read registers 2-4 (Wind Dir + Speed)

    uint8_t frame[8];
    frame[0] = device_id;
    frame[1] = function_code;
    frame[2] = highByte(start_address);
    frame[3] = lowByte(start_address);
    frame[4] = highByte(num_registers);
    frame[5] = lowByte(num_registers);

    uint16_t crc = crc16(frame, 6);
    frame[6] = lowByte(crc);
    frame[7] = highByte(crc);

    // Clear RX buffer and send request
    while (ULTRASONIC.available()) ULTRASONIC.read();
    ULTRASONIC.write(frame, 8);
    ULTRASONIC.flush();

    // Wait for response
    unsigned long startTime = millis();
    uint8_t response[32];
    uint8_t idx = 0;
    
    const uint8_t expected_len = 5 + 2 * num_registers;
    
    while ((millis() - startTime) < 1000) { // 1 second timeout
        if (ULTRASONIC.available()) {
            response[idx++] = ULTRASONIC.read();
            if (idx >= expected_len) break;
        }
        delay(1);
    }

    if (idx < 5) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("x modbus"));
        #endif
        UZerror();
        return;
    }

    // Verify CRC
    uint16_t recv_crc = (uint16_t)response[idx - 2] | ((uint16_t)response[idx - 1] << 8);
    uint16_t calc_crc = crc16(response, idx - 2);

    if (recv_crc != calc_crc) {
        #ifdef DEBUG
            DEBUGSERIAL.println(F("x Modbus CRC"));
        #endif
        return;
    }


    // Parse response
    uint8_t byte_count = response[2];
    uint8_t *payload = &response[3];

    // Extract Wind Direction (Register 2 - 16-bit integer)
    uint16_t wind_dir_raw = ((uint16_t)payload[0] << 8) | payload[1];
    
    // Extract Wind Speed (Registers 3-4 - 32-bit float)
    // Word-swapped order: [4,5,2,3]
    floatUnion.b[3] = payload[4]; 
    floatUnion.b[2] = payload[5]; 
    floatUnion.b[1] = payload[2]; 
    floatUnion.b[0] = payload[3]; 
    
    float wind_speed_raw = floatUnion.f;

    // Apply calibration and processing (same as your original code)
    calDirection = wind_dir_raw + vaneOffset;
    CalculateWindDirection();  // calculate wind direction from data
    
    // Convert m/s to your preferred units if needed
    // Original code used: windSpeed = atof(wind)*19.4384449;
    windSpeed = wind_speed_raw*19.4384449;
    
    CalculateWindGust(windSpeed);
    CalculateWind();
    
    if ((wind_speed >= (cutoffWind * 10) && measureCount <= whenSend) || 
        (wind_speed < (cutoffWind * 10) && measureCount <= (whenSend * 2))) {
        timergprs = 0;  
    }

  digitalWrite(13, HIGH);   // turn the LED on
  delay(15);                       // wait
  digitalWrite(13, LOW);    // turn the LED
  
    noInterrupts();
    timergprs = 0;                                
    interrupts();
}

void UZerror() { //ultrasonic error
  sonicError++;
  #ifdef DEBUG
      DEBUGSERIAL.print(F("err UZ"));
  #endif

  #ifdef toggle_UZ_power
    if ( sonicError >=4)  { UZ_power_toggle(); sonicError=0; }   // if more than x US errors 
  #else
    if ( sonicError >=4)  { reset(4);  }   // if more than x US errors 
  #endif
}

#ifdef toggle_UZ_power
void UZ_power_toggle(){
  digitalWrite(26, LOW);  
  delay(2000); 
  digitalWrite(26, HIGH);  
  
    #ifdef DEBUG
      DEBUGSERIAL.println("uz pwr");
  #endif
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




void DominantDirection() { // get dominant wind direction
  windDir =  atan2(windAvgY, windAvgX) / PI * 180;
  if (windDir < 0) windDir += 360;
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
    if (sht.read()) { // Returns true if read successful
      temp = sht.getTemperature();
      humidity = sht.getHumidity();
    } else {
      #ifdef DEBUG
        DEBUGSERIAL.println(F("Failed to read from SHT31"));
      #endif
    }
  #else
    if (sht.measure() == SHT4X_STATUS_OK) {
      if (sht.TcrcOK) { 
        temp = sht.TtoDegC();
      } 
      if (sht.RHcrcOK) { 
        humidity = sht.RHtoPercent();
      } 
    }
  #endif 
}
#endif




void GetTmpNow() {
#ifdef TMPDS18B20
  if (onOffTmp == 1) {
    GetAir();                               // air
    data[12]=99;
    delay(20);
  }
  else if (onOffTmp == 2) {    
    if (enableRain==0){  
      GetWater();                             // water
    }          
    data[13]=99;
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
