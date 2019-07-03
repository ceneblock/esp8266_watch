#ifndef EEPROM_H
#define EEPROM_H
#include <Wire.h>
#include <time.h>
#include <math.h>
#include <string.h>

/**
 * @author ceneblock
 * @brief this class handles timezone info
 * @todo give it a better name
 */
class watch_eeprom
{
  public:
    struct time_zone_data_t
    {
      char name[4];    /// short name
      int  offset;     /// offset during DST
      bool celebrates; /// if this timezone uses DST
    }; 
   
    watch_eeprom(unsigned int address);
    tm calculate_dst_start(unsigned int year);
    tm calculate_dst_end(unsigned int year);
    
    
  private:
    void setTimeZones();
  protected:
    unsigned int address;
    tm dst_begin, dst_end;
    time_zone_data_t tzd[7];
}

void watch_eeprom::setTimeZones()
{
  ///strncpy should be used, but I don't think it matters in this case..
  strcpy(tzd[0].name,"HI");
  tzd[0].offset     = -10;
  tzd[0].celebrates = false;
  
  strcpy(tzd[1].name,"AK");
  tzd[1].offset     = -9;
  tzd[1].celebrates = true;

  strcpy(tzd[2].name,"LA");
  tzd[2].offset     = -8;
  tzd[2].celebrates = true;

  strcpy(tzd[3].name,"AZ");
  tzd[3].offset     = -7;
  tzd[3].celebrates = false;

  strcpy(tzd[4].name,"SLC");
  tzd[4].offset     = -7;
  tzd[4].celebrates = true;

  strcpy(tzd[5].name,"CHI");
  tzd[5].offset     = -6;
  tzd[5].celebrates = true;

  strcpy(tzd[6].name,"NYC");
  tzd[6].offset     = -5;
  tzd[6].celebrates = true;
}

#endif
