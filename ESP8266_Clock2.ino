// Getting time from the header of a webpage and update
// local RTC in the 8266
// Displays time, then day and date and then Temp + Hum

// mods by SPB Orig from Net
// DHT11 was giving wrong Humidity so replaced with DHT22
// 8 June 2024
// If during Testing several calls made in short period
// to google.co.in it bloked the ip so yahoo was selected
//
// the time is derived from header, so google.com 's
// time was inaccurate in the late night, so .in was taken

// We may perhaps get time from NTP (better)
// also a local RTC (eg 1307) may be connected which may be updated
// once a day only


#include "Arduino.h"
#include "ESP8266WiFi.h"

WiFiClient client;

String date;

// display
#define NUM_MAX 4
#define LINE_WIDTH 16
#define ROTATE  90

// for NodeMCU 1.0
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 12  // D6

#include "max7219.h"
#include "fonts.h"

// Temp & Humi sensor lib
#include "DHT.h"

#define DHTPIN 0          // D3 data line of sensor

#define DHTTYPE DHT22     // DHT 11 was giving wrong Hum 

DHT dht(DHTPIN, DHTTYPE, 15); // 15 was found on a website for fast processor

int temp, hum; //

// Wifi
const char* ssid     = "Airtel_9879391986";     // SSID of local network
const char* password = "air54614";   // Password on network
float utcOffset = 5.5; // Time Zone setting

void setup()
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1); // to display
  // sendCmdAll(CMD_INTENSITY,10); // Adjust the brightness between 0 and 15
  dht.begin();

  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting ", 16);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());
}
// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx = 0;
int dy = 0;
byte del = 0;
int h, m, s, h_ampm;
long localEpoc = 0;
long localMillisAtUpdate = 0;
bool pm = false; //am pm flag for displaying a dot
// =======================================================================
void loop()
{
  if (updCnt <= 0) { // every 10 scrolls, ~450s=7.5m
    updCnt = 1000;  // 80 ~1 hr
    Serial.println("Getting data ...");
    printStringWithShift("  Getting data", 15);

    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }

  if (millis() - clkTime > 20000 && !del && dots) {
    // clock for 20s,
    printStringWithShift(date.c_str(), 20);
    delay(5000); // Day date for 5 sec
    updCnt--;
    showTempHum();
    delay(4000); // T & H for 4 sec
    clkTime = millis();
  }
  if (millis() - dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }
  updateTime();
  showAnimClock();


  // Adjusting LED intensity.
  // 12am to 6am, lowest intensity 0
  if ( (h == 0) || ((h >= 1) && (h <= 6)) ) sendCmdAll(CMD_INTENSITY, 0);
  // 6pm to 11pm, intensity = 2
  else if ( (h >= 18) && (h <= 23) ) sendCmdAll(CMD_INTENSITY, 1);
  // max brightness during bright daylight
  else sendCmdAll(CMD_INTENSITY, 3);  // was 10 v bright
}
// =======================================================================
void showTempHum()   // Display Temp and Humidity from DHT11/22
{ // float temp0 = dht.readTemperature();
  // Serial.print(temp);
  // Serial.println(" C");

  temp = (int)dht.readTemperature();
  Serial.println(temp);
  dx = dy = 0;
  clr(); // display
  // in font table (font.h) the digits start at 17th posn so + 16, 0 is col no
  showDigit(temp / 10 + 16,  0, font);   //dig4x8); 1st digit of Temp
  showDigit (temp % 10 + 16, 5, font);   //dig4x8); 2nd Digit
  showDigit (35, 10, font);  // 35 (C) is entry no in font table for C

  //float hum0 = dht.readHumidity();
  //Serial.print(hum0);
  //Serial.println(" %");

  hum = (int)dht.readHumidity();
  if (hum ==100) hum=99;
  Serial.println(hum);
  showDigit(hum / 10 + 16, 17, font); // 1st digit of Hum
  showDigit (hum % 10 + 16, 22, font); // 2nd digit
  showDigit (5, 27, font);  // entry no 5 is %

  refreshAll();
}

void showSimpleClock()
{
  dx = dy = 0;
  clr();
  if (h < 13)
  {
    h_ampm = h;
    pm = false;
  }
  else
  {
    h_ampm = h - 12;
    pm = true;
  }
  showDigit(h_ampm / 10,  4, dig6x8);
  showDigit(h_ampm % 10, 12, dig6x8);
  showDigit(m / 10, 21, dig6x8);
  showDigit(m % 10, 29, dig6x8);
  showDigit(s / 10, 38, dig6x8);
  showDigit(s % 10, 46, dig6x8);
  setCol(19, dots ? B00100100 : 0);
  setCol(36, dots ? B00100100 : 0);
  if (pm)
    setCol(31, 1); // dot on  right top to indicate PM
  refreshAll();
  delay(50);
}

// =======================================================================

void showAnimClock()
{
  byte digPos[4] = {1, 8, 17, 25};
  int digHt = 12;
  int num = 4;
  int i;

// convert into 12 Hr format 
  if ( h > 12)
  {
    h_ampm = h - 12;
  }
  else
    h_ampm = h;

  if (del == 0) {
    del = digHt;
    for (i = 0; i < num; i++) digold[i] = dig[i];
    dig[0] = h_ampm / 10 ? h_ampm / 10 : 10;
    dig[1] = h_ampm % 10;
    dig[2] = m / 10;
    dig[3] = m % 10;
    for (i = 0; i < num; i++)  digtrans[i] = (dig[i] == digold[i]) ? 0 : digHt;
  } else
    del--;

  clr();
  for (i = 0; i < num; i++) {
    if (digtrans[i] == 0) {
      dy = 0;
      showDigit(dig[i], digPos[i], dig6x8); // font used dig6x8
    } else {
      dy = digHt - digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }
  dy = 0;
  setCol(15, dots ? B00100100 : 0); //to blink ':' between hr & min 
  refreshAll();
  delay(30);
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data)
{
  if (dy < -8 | dy > 8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy) scr[col + i] = v; else scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if (dy < -8 | dy > 8) return;
  col += dx;
  if (col >= 0 && col < 8 * NUM_MAX)
      if (!dy) scr[col] = v; else scr[col] |= dy > 0 ? v >> dy : v << -dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX * 8 + i] = 0;
  return w;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {

  if (c < ' ' || c > '~' + 25) return;
  c -= 32;
  int w = showChar(c, font);
  for (int i = 0; i < w + 1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char* s, int shiftDelay) {
  while (*s) {
    printCharWithShift(*s, shiftDelay);
    s++;
  }
}

// =======================================================================



void getTime()  // this could be done using NTP or GPS
{
  WiFiClient client;
  //  if (!client.connect("www.google.co.in", 80)) {
// quick retrievals during test  google blocked
  if (!client.connect("www.yahoo.co.in", 80)) {
    Serial.println("Wifi client connection failed");
    return;
  }

  client.print(String("GET / HTTP / 1.1\r\n") +
                 String("Host: www.yahoo.co.in\r\n") +
               //String("Host: www.google.co.in\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    //Serial.println(".");
    repeatCounter++;

  }

  String line;
  client.setNoDelay(false);
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      Serial.println(line);
      date = "     " + line.substring(6, 17);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);

    }
  }
  client.stop();
}

// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = fmod(round(curEpoch + 3600 * utcOffset + 86400L), 86400L);
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================
