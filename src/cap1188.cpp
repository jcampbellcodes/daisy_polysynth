#include "cap1188.h"

using namespace daisy;
/*!
 *    @brief  Instantiates a new CAP1188 class using hardware I2C
 *    @param  resetpin
 *            number of pin where reset is connected
 *
 */
cap1188::cap1188(int8_t resetpin) 
: i2c_dev()
{
  // I2C
    // setup the configuration
    I2CHandle::Config i2c_conf;
    i2c_conf.periph = I2CHandle::Config::Peripheral::I2C_1;
    i2c_conf.speed  = I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_conf.mode   = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_conf.pin_config.scl  = {DSY_GPIOB, 8};
    i2c_conf.pin_config.sda  = {DSY_GPIOB, 9};
    i2c_dev.Init(i2c_conf);

  _resetpin = resetpin;

}

/*!
 *    @brief  Setups the i2c depending on selected mode (I2C / SPI, Software /
 * Hardware). Displays useful debug info, as well as allow multiple touches
 * (CAP1188_MTBLK), links leds to touches (CAP1188_LEDLINK), and increase the
 * cycle time value (CAP1188_STANDBYCFG)
 *    @param  i2caddr
 *            optional i2caddres (default to 0x29)
 *    @param  theWire
 *            optional wire object
 *    @return True if initialization was successful, otherwise false.
 */
bool cap1188::begin(uint8_t i2caddr) {
    // I2C
    // i2c_dev = new Adafruit_I2CDevice(i2caddr, theWire);
    // if (!i2c_dev->begin())
    //   return false;
    i2c_addr = i2caddr;
    

//   if (_resetpin != -1) {
//     pinMode(_resetpin, OUTPUT);
//     digitalWrite(_resetpin, LOW);
//     delay(100);
//     digitalWrite(_resetpin, HIGH);
//     delay(100);
//     digitalWrite(_resetpin, LOW);
//     delay(100);
//   }

  readRegister(CAP1188_PRODID);

//   // Useful debugging info

//   Serial.print("Product ID: 0x");
//   Serial.println(readRegister(CAP1188_PRODID), HEX);
//   Serial.print("Manuf. ID: 0x");
//   Serial.println(readRegister(CAP1188_MANUID), HEX);
//   Serial.print("Revision: 0x");
//   Serial.println(readRegister(CAP1188_REV), HEX);
  

  auto prodID = readRegister(CAP1188_PRODID);
  auto manuID = readRegister(CAP1188_MANUID);
  auto rev = readRegister(CAP1188_REV);

  if ((prodID != 0x50) ||
      (manuID != 0x5D) ||
      (rev != 0x83)) 
  {
    return false;
  }

  // allow multiple touches
  writeRegister(CAP1188_MTBLK, 0);
  // Have LEDs follow touches
  writeRegister(CAP1188_LEDLINK, 0xFF);
  // speed up a bit
  writeRegister(CAP1188_STANDBYCFG, 0x30);
  return true;
}

/*!
 *   @brief  Reads the touched status (CAP1188_SENINPUTSTATUS)
 *   @return Returns read from CAP1188_SENINPUTSTATUS where 1 is touched, 0 not
 * touched.
 */
uint8_t cap1188::touched() {
  uint8_t t = readRegister(CAP1188_SENINPUTSTATUS);
  if (t) {
    writeRegister(CAP1188_MAIN, readRegister(CAP1188_MAIN) & ~CAP1188_MAIN_INT);
  }
  return t;
}

/*!
 *   @brief  Controls the output polarity of LEDs.
 *   @param  inverted
 *           0 (default) - The LED8 output is inverted.
 *           1 - The LED8 output is non-inverted.
 */
void cap1188::LEDpolarity(uint8_t inverted) {
  writeRegister(CAP1188_LEDPOL, inverted);
}

/*!
 *    @brief  Reads from selected register
 *    @param  reg
 *            register address
 *    @return
 */
uint8_t cap1188::readRegister(uint8_t reg) {
  uint8_t buffer[3] = {reg, 0, 0};
//   if (i2c_dev) {
//     i2c_dev->write_then_read(buffer, 1, buffer, 1);
//   }
  i2c_dev.TransmitBlocking(i2c_addr, buffer, sizeof(buffer), 1000);
  i2c_dev.ReceiveBlocking(i2c_addr, buffer, sizeof(buffer), 1000);
  return buffer[0];
}

/*!
 *   @brief  Writes 8-bits to the specified destination register
 *   @param  reg
 *           register address
 *   @param  value
 *           value that will be written at selected register
 */
void cap1188::writeRegister(uint8_t reg, uint8_t value) {
  uint8_t buffer[4] = {reg, value, 0, 0};
//   if (i2c_dev) {
//     i2c_dev->write(buffer, 2);
//   }
i2c_dev.TransmitBlocking(i2c_addr, buffer, sizeof(buffer), 1000);
}