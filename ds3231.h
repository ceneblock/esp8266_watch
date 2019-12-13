#ifndef DS3231_H
#define DS3231_H
class DS3231
{
  public:
    DS3231(byte DS3231_ADDRESS = 0x68);
    tm getCurrentTime();
    void setTime(tm);
  protected:
    byte DS3231_ADDRESS;

    tm current_time;

    static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); } ///This is going to get nasty in year 2100 and 2200
    static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

    const byte SEC_REGISTER   = 0x00;
    const byte MIN_REGISTER   = 0x01;
    const byte HOUR_REGISTER  = 0x02;
    const byte DOW_REGISTER   = 0x03;
    const byte DAY_REGISTER   = 0x04;
    const byte MONTH_REGISTER = 0x05;
    const byte YEAR_REGISTER  = 0x06;
};

DS3231::DS3231(byte DS3231_ADDRESS)
{
  Wire.begin();
  this -> DS3231_ADDRESS = DS3231_ADDRESS;
}

tm DS3231::getCurrentTime()
{
  ///Hmm...I wonder if there is a way to semaphore this...
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((byte)0);
  Wire.endTransmission();
  //Wire.end();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire.read() & 0x7F);
  uint8_t mm = bcd2bin(Wire.read());
  uint8_t hh = bcd2bin(Wire.read());
  uint8_t dd = bcd2bin(Wire.read()) - 1;
  uint8_t d = bcd2bin(Wire.read());
  uint8_t m = bcd2bin(Wire.read());
  uint16_t y = bcd2bin(Wire.read()) + 100;
  
  current_time.tm_sec = ss;
  current_time.tm_min = mm;
  current_time.tm_hour = hh;
  current_time.tm_wday = dd;
  current_time.tm_mday = d;
  current_time.tm_mon  = m;
  current_time.tm_year = y;
  
  return current_time;
}

void DS3231::setTime(tm current_time)
{
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(SEC_REGISTER);
  Wire.write(bin2bcd(current_time.tm_sec));
  Wire.write(bin2bcd(current_time.tm_min));
  Wire.write(bin2bcd(current_time.tm_hour));
  Wire.write(bin2bcd(current_time.tm_wday+1));
  Wire.write(bin2bcd(current_time.tm_mday));
  Wire.write(bin2bcd(current_time.tm_mon));
  Wire.write(bin2bcd(current_time.tm_year - 100));
  Wire.endTransmission();
}
#endif
