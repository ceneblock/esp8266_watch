#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <Ticker.h> 
#include <SD.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <time.h>

///This contains the network ssid and password
#include "networkinfo.h"

ThreeWire myWire(4,5,2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

const int CS_PIN = 10;
Ticker timer;
String display_time = "", old_display_time = "";

int offset = -5 * 60 * 60; //offset * seconds * minuets

ESP8266WiFiMulti wifiMulti;      // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
WiFiUDP UDP;                     // Create an instance of the WiFiUDP class to send and receive
IPAddress timeServerIP;          // time.nist.gov NTP server address
const char* NTPServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

void updateTime();




#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);          // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

   Rtc.Begin();

  startWiFi();                   // Try to connect to some given access points. Then wait for a connection

  startUDP();

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  
  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);

  timer.attach(1, updateTime);


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }


  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
//  // Clear the buffer
  display.clearDisplay();
  display.display();
//
//  // Draw a single pixel in white
//  display.drawPixel(10, 10, WHITE);
//
//  // Show the display buffer on the screen. You MUST call display() after
//  // drawing commands to make them visible on screen!
//  display.display();
//  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...



//  if(!SD.begin(CS_PIN))
//  {
//    Serial.println("Unable to access SD Card");
//  }
//  else
//  {
//    Serial.println("Hello SD Card!");
//  }
}

/*__________________________________________________________LOOP__________________________________________________________*/

unsigned long intervalNTP = 60000; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;

unsigned long prevActualTime = 0;

void loop() {
  if(old_display_time != display_time)
  {
    old_display_time = display_time; 
  
    display.clearDisplay();
  
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
  
    display.println(" " + (display_time));
    display.display();
  }
}

/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Try to connect to some given access points. Then wait for a connection
  wifiMulti.addAP(NETWORK, PASSWORD);   // add Wi-Fi networks you want to connect to
//  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
//  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());             // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  Serial.println("\r\n");
}

void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

tm* getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];

  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears + offset;

  printf("Time %s", ctime( ( const time_t* ) &UNIXTime ));
  
  return localtime((const time_t *)&UNIXTime);
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode

  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

/**
 * @brief updateTime time as a critical section
 * @note Looks like doing the i2c stuff in here causes some major problems
 */
void updateTime()
{
  tm* time = NULL;
  unsigned long currentMillis = millis();
  sendNTPpacket(timeServerIP);               // Send an NTP request

  time = getTime(); //interrupt the results
  if (time)
  {              
    display_time = "";    
    lastNTPResponse = currentMillis;
    if(time-> tm_hour < 10)
    {
      display_time = "0";
    }
    display_time += String(time -> tm_hour) + ":";
    if(time -> tm_min < 10)
    {
      display_time += "0";
    }
    display_time += String(time -> tm_min) + ":";
    if(time -> tm_sec < 10)
    {
     display_time +="0";
    }
    display_time +=  String(time -> tm_sec);
  } 
  else if ((currentMillis - lastNTPResponse) > 3600000) 
  {
    Serial.println("More than 1 hour since last NTP response. Rebooting.");
    Serial.flush();
    ESP.reset();
    display_time = "Error!";
  }
}


//#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
//#include <ESP8266HTTPClient.h>
//#include <ESP8266WiFiMulti.h>
//
////needed for library
//#include <DNSServer.h>
//#include <ESP8266WebServer.h>
//#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
//
//#include <WiFiUdp.h>
//
//void setup() {
//  // put your setup code here, to run once:
//  Serial.begin(115200);
//  //WiFiManager
//  //Local intialization. Once its business is done, there is no need to keep it around
//  WiFiManager wifiManager;
//  //reset saved settings
//  //wifiManager.resetSettings();
//  //set custom ip for portal
//  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
//  
//  //fetches ssid and pass from eeprom and tries to connect
//  //if it does not connect it starts an access point with the specified name
//  //here  "AutoConnectAP"
//  //and goes into a blocking loop awaiting configuration
//  wifiManager.autoConnect("LinkNodeAP");
//  //or use this for auto generated name ESP + ChipID
//  //wifiManager.autoConnect();
//  
//  //if you get here you have connected to the WiFi
//  Serial.println("connected... :)");
//
//  
//
////  HTTPClient http;
////
////  http.begin("http://cosmic.decay.space/centurylink/");
////
////  int httpCode = http.GET();
////
////  Serial.println(httpCode);
////  String payload = http.getString();
////  Serial.println(payload);
//
//}
//// the loop function runs over and over again forever
//void loop() {
//digitalWrite(BUILTIN_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
//delay(1000);              // wait for a second
//digitalWrite(BUILTIN_LED, LOW);    // turn the LED off by making the voltage LOW
//delay(1000);              // wait for a second
//}
