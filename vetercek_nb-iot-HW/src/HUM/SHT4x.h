/*

Arduino library for the Sensirion SHT4x humidity and temperature sensors (SHT40, SHT41, SHT45)

*/

#ifndef SHT4X_H
#define SHT4X_H

#include <Arduino.h>
#include <Wire.h>

/** \typedef SHT4X_ChipTypes_TypeDef
 *  \brief Chip type: there're 2 versions with different addresses. Those marked A have address 0x44, those marked B 0x45.
 */
typedef enum {
  /** Chip type A (address = 0x44) */
  SHT4X_CHIPTYPE_A = 0x44,
  /** Chip type A (address = 0x45) */
  SHT4X_CHIPTYPE_B = 0x45
} SHT4X_ChipTypes_TypeDef;

/** \typedef SHT4X_Commands_TypeDef
 *  \brief General sensor commands
 */
typedef enum {
  /** Soft reset */
  SHT4X_CMD_SFT_RST = 0x94,
  /** Read serial */
  SHT4X_CMD_READ_SERIAL = 0x89
} SHT4X_Commands_TypeDef;

/** \typedef SHT4X_MeasurementModes_TypeDef
 *  \brief Sensor measurement modes
 */
typedef enum {
  /** measure T & RH with high precision (high repeatability) */
  SHT4X_CMD_MEAS_HI_PREC = 0xFD,
  /** measure T & RH with medium precision (medium repeatability) */
  SHT4X_CMD_MEAS_MED_PREC = 0xF6,
  /** measure T & RH with lowest precision (low repeatability) */
  SHT4X_CMD_MEAS_LOW_PREC = 0xE0,
  /** activate highest heater power & high precis. meas. (typ. 200mW @ 3.3V) for 1s */
  SHT4X_CMD_HI_HEAT_1S_MEAS = 0x39,
  /** activate highest heater power & high precis. meas. (typ. 200mW @ 3.3V) for 0.1s */
  SHT4X_CMD_HI_HEAT_01S_MEAS = 0x32,
  /** activate medium heater power & high precis. meas. (typ. 110mW @ 3.3V) for 1s */
  SHT4X_CMD_MED_HEAT_1S_MEAS = 0x2F,
  /** activate medium heater power & high precis. meas. (typ. 110mW @ 3.3V) for 0.1s */
  SHT4X_CMD_MED_HEAT_01S_MEAS = 0x24,
  /** activate lowest heater power & high precis. meas. (typ. 20mW @ 3.3V) for 1s */
  SHT4X_CMD_LOW_HEAT_1S_MEAS = 0x1E,
  /** activate lowest heater power & high precis. meas. (typ. 20mW @ 3.3V) for 0.1s */
  SHT4X_CMD_LOW_HEAT_01S_MEAS = 0x15
} SHT4X_MeasurementModes_TypeDef;

/** \typedef SHT4X_Status_TypeDef
 *  \brief Status return codes
 */
typedef enum {
  /** returned value when the operation went fine */
  SHT4X_STATUS_OK = 0,
  /** an error occurred during the operation */
  SHT4X_STATUS_ERROR,
  /** CRC checksum didn't match */
  SHT4X_STATUS_CRC_FAIL
} SHT4X_Status_TypeDef;

/** \class SHT4x
 *  \brief Class handling communication with a Sensirion SHT4x sensor
 */
class SHT4x {
protected:
  /** \property TwoWire* _i2c
   *  \brief Underlying I2C channel object
   */
  TwoWire*  _i2c = &Wire;

  /** \property SHT4X_MeasurementModes_TypeDef _measurement_mode
   *  \brief Measurement mode (see SHT4X_MeasurementModes_TypeDef definition for details)
   */
  SHT4X_MeasurementModes_TypeDef _measurement_mode = SHT4X_CMD_MEAS_HI_PREC;

  /** \property bool _busy
   *  \brief true when the sensor is known to be busy, false otherwise (note that there's no way
   *  to request this from the sensor, so if you start a measurement with the heater on for 1s and
   *  call abort() straight away, this will be set to false while the sensor is still busy)
   */
  bool _busy;

  /** \property SHT4X_ChipTypes_TypeDef _address
   *  \brief Sensor's I2C address (see SHT4X_ChipTypes_TypeDef definition for details)
   */
  SHT4X_ChipTypes_TypeDef _address = SHT4X_CHIPTYPE_A;
   
  /** \fn void _clearBuffer()
   *  \brief Clear I2C buffer.
   */
  void _clearBuffer();
  
  /** \fn SHT4X_Status_TypeDef sendCommand(SHT4X_Commands_TypeDef cmd)
   *  \brief Send a command to the sensor.
   *  \param cmd : command to send.
   *  \returns a status code telling whether the command was acknowledged.
   */
  SHT4X_Status_TypeDef sendCommand(SHT4X_Commands_TypeDef cmd);
  /** \fn SHT4X_Status_TypeDef sendCommand(SHT4X_MeasurementModes_TypeDef cmd)
   *  \brief Send a measurement command to the sensor.
   *  \param cmd : command to send.
   *  \returns a status code telling whether the command was acknowledged.
   */
  SHT4X_Status_TypeDef sendCommand(SHT4X_MeasurementModes_TypeDef cmd);
  
  /** \fn SHT4X_Status_TypeDef abort()
   *  \brief Abort an ongoing transaction.
   *  \returns an error status.
   */
  SHT4X_Status_TypeDef abort();
  
public:
  /** \fn SHT4X()
   *  \brief Constructor.
   */
  SHT4x();
  
  /** \property uint16_t RH
   *  \brief Latest relative humidity measurement (raw)
   */
  uint16_t RH;
  
  /** \property uint16_t T
   *  \brief Latest temperature measurement (raw)
   */
  uint16_t T;
  
  /** \property bool TcrcOK
   *  \brief true if the latest temperature measurement matches the CRC checksum, false otherwise.
   */
  bool TcrcOK;
  
  /** \property bool RHcrcOK
   *  \brief true if the latest relative humidity measurement matches the CRC checksum, false otherwise.
   */
  bool RHcrcOK;
  
  /** \property uint16_t serial[2]
   *  \brief Latest serial number measurement (raw)
   */
  uint16_t serial[2];
  
  /** \fn SHT4X_Status_TypeDef setPort(TwoWire &wirePort)
   *  \brief Set I2C port instance.
   *  \param wirePort : I2C Wire port instance over which to communicate with the sensor.
   *  \returns a status code telling whether a sensor was found on this port.
   */
  SHT4X_Status_TypeDef setPort(TwoWire &wirePort);
    
  /** \fn bool checkCRC(uint16_t data, uint8_t crc)
   *  \brief Check a CRC checksum against a given value.
   *  \param data : 2 byte data to check
   *  \param crc : 1 byte CRC to compare against
   *  \returns true if CRC matched, false otherwise.
   */
  bool checkCRC(uint16_t data, uint8_t crc);
  
  /** \fn void setChipType(SHT4X_ChipTypes_TypeDef type)
   *  \brief Set chip type.
   *  \param type : sensor type (see SHT4X_ChipTypes_TypeDef definition for details)
   */
  void setChipType(SHT4X_ChipTypes_TypeDef type);
  
  /** \fn void SHT4X_ChipTypes_TypeDef getChipType()
   *  \brief Get chip type.
   *  \returns sensor type (see SHT4X_ChipTypes_TypeDef definition for details).
   */
  SHT4X_ChipTypes_TypeDef getChipType();
  
  /** \fn SHT4X_Status_TypeDef softReset()
   *  \brief Send a soft reset command to the sensor.
   *  \returns a status code telling whether the command was acknowledged.
   */
  SHT4X_Status_TypeDef softReset();
  
  /** \fn SHT4X_Status_TypeDef checkSerial()
   *  \brief Request serial number from the sensor.
   *  \returns a status code telling whether the command succeeded and serial number passed CRC test.
   */
  SHT4X_Status_TypeDef checkSerial();
  
  /** \fn SHT4X_Status_TypeDef setMode(SHT4X_MeasurementModes_TypeDef mode)
   *  \brief Set measurement mode.
   *  \param mode : the code for the measurement mode (see SHT4X_MeasurementModes_TypeDef definition for details)
   */
  void setMode(SHT4X_MeasurementModes_TypeDef mode);
  
  /** \fn SHT4X_MeasurementModes_TypeDef getMode()
   *  \brief Get current measurement mode.
   *  \returns the code for the currently set measurement mode (see SHT4X_MeasurementModes_TypeDef definition for details)
   */
  SHT4X_MeasurementModes_TypeDef getMode();
  
  /** \fn SHT4X_Status_TypeDef measure()
   *  \brief Tell the sensor to perform a measurement using the currently set measurement mode.
   *  \returns a status code telling whether the command was acknowledged.
   */
  SHT4X_Status_TypeDef measure();

  /** \fn float TtoDegC()
   *  \brief Compute temperature in degrees Celsius using raw data stored in the T property.
   *  \returns a floating point value representing temperature in degrees Celsius.
   */
  float TtoDegC();
  
  /** \fn float TtoDegF()
   *  \brief Compute temperature in degrees Fahrenheit using raw data stored in the T property.
   *  \returns a floating point value representing temperature in degrees Fahrenheit.
   */
  float TtoDegF();
  
  
  /** \fn float RHtoPercent()
   *  \brief Compute relative humidity in percent using raw data stored in the RH property.
   *  \returns a floating point value representing relative humidity in percent.
   */
  float RHtoPercent();
  
};

#endif /* SHT4X_H */
