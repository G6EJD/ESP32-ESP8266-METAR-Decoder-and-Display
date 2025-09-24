/*  Revised METAR Decoder and display for ESP8266/ESP32 and ILI9341 TFT screen
  Now based on a JSON formated Metar and incorporates the recent API changes 

  This software, the ideas and concepts is Copyright (c) David Bird 2025. All rights to this software are reserved.

  Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
  1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
  2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
  3. You may not, except with my express written permission, distribute or commercially exploit the content.
  4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

  The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
  software use is visible to an end-user.

  THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
  OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  See more at http://www.dsbird.org.uk

*/
////////////////////////////////////////////////////////////////////////////////////
String version_num = "METAR ESP Version 1.0";
#ifdef ESP32
#include <WiFi.h>
#include "HTTPClient.h"
#define CS 17             // ESP32 GPIO 17 goes to TFT CS
#define DC 16             // ESP32 GPIO 16 goes to TFT DC
#define MOSI 23           // ESP32 GPIO 23 goes to TFT MOSI
#define SCLK 18           // ESP32 GPIO 18 goes to TFT SCK/CLK
#define RST 5             // ESP32 GPIO  5 ESP RST to TFT RESET
#define MISO              // Not connected
//      3.3V     // Goes to TFT LED
//      5v       // Goes to TFT Vcc
//      Gnd      // Goes to TFT Gnd
#else
#include <ESP8266WiFi.h>
#include "ESP8266HTTPClient.h"
#define CS D0    // Wemos D1 Mini D0 goes to TFT CS
#define DC D8    // Wemos D1 Mini D8 goes to TFT DC
#define MOSI D7  // Wemos D1 Mini D7 goes to TFT MOSI
#define SCLK D5  // Wemos D1 Mini D5 goes to TFT SCK/CLK
#define RST      // Wemos D1 Mini RST on ESP goes to TFT RESET
#define MISO     // Wemos D1 Mini Not connected
//      3.3V    // Goes to TFT LED
//      5v      // Goes to TFT Vcc
//      Gnd     // Goes to TFT Gnd
#endif

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson needs version v6 or above
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(CS, DC);

const char* ssid = "your WiFi SSID";
const char* password = "your WiFi PASSWORD";
const int httpsPort = 443;

WiFiClientSecure client;

const int centreX = 274;  // Location of the compass display on screen
const int centreY = 60;
const int diameter = 41;  // Size of the compass

// Assign human-readable names to common 16-bit color values:
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(__FILE__);
  tft.begin();
  tft.setRotation(3);
  clear_screen();
  display_progress("Connecting to Network", 25);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected at: " + WiFi.localIP().toString());
  display_status();
}

void loop() {
  // Change these METAR Stations to suit your needs see: Use this URL address = ftp://tgftp.nws.noaa.gov/data/observations/metar/decoded/
  // to establish your list of sites to retrieve (you must know the 4-letter
  // site dentification)
  GET_METAR("EGGD", "1 EGGD Bristol/Lulsgate");
  GET_METAR("KCHA", "2 KCHA Chattanooga Metro");
  GET_METAR("KPLN", "3 KPLN Kelston");
  GET_METAR("EGHQ", "4 EGHQ Newquay");
  GET_METAR("EGSS", "5 EGSS Stansted");
}

//----------------------------------------------------------------------------------------------------
void GET_METAR(String station, String Name) {  //client function to send/receive GET request data.
  String metar, raw_metar;
  bool metar_status = true;
  const int time_delay = 50000;
  display_item(35, 100, "Decoding METAR", GREEN, 3);
  display_item(90, 135, "for " + station, GREEN, 3);
  // https://aviationweather.gov/api/data/stationinfo?ids=EGLL
  // https://aviationweather.gov/api/data/metar?format=json&hours=0&ids=EGLL&hoursBeforeNow=1
  const char* host = "https://aviationweather.gov";
  String url = String(host) + "/api/data/metar?format=json&ids=" + station + "&hoursBeforeNow=1";
  Serial.println("Connected, Requesting data for : " + Name);
  HTTPClient http;
  #ifdef ESP32
  http.begin(url.c_str());    // Specify the URL and maybe a certificate
  #else
    WiFiClient client;
    http.begin(client, url);    // Specify the URL and maybe a certificate
  #endif
  int httpCode = http.GET();  // Start connection and send HTTP header
  Serial.println("Connection status: " + String(httpCode > 0 ? "Connected" : "Connection Error"));
  if (httpCode > 0) {  // HTTP header has been sent and Server response header has been handled
    if (httpCode == HTTP_CODE_OK) metar = http.getString();
    http.end();
    Serial.println(metar);
  } else {
    http.end();
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    metar_status = false;
    metar = "Station off-air";
  }
  display_item(265, 230, "Connected", RED, 1);
  client.stop();
  //clear_screen();  // Clear screen
  display_item(265, 230, "Connected", RED, 1);
  display_item(0, 230, version_num, GREEN, 1);
  display_item(0, 190, Name, YELLOW, 2);
  tft.drawLine(0, 185, 320, 185, YELLOW);
  display_item(0, 210, metar, YELLOW, 1);
  if (metar_status == true) {
    display_metar(metar);
    delay(time_delay);  // Delay for set time
  } else {
    display_item(70, 100, metar, YELLOW, 2);  // Now decode METAR
    delay(5000);                              // Wait less time if station off-air
  }
  clear_screen();  // Clear screen before moving to next station display
}

bool display_metar(String metar) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, metar);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  /*
    "icaoId": "KORD",
    "receiptTime": "2023-11-03 21:54:03",
    "obsTime": 1699048260,
    "reportTime": "2023-11-03 22:00:00",
    "temp": 14.4,
    "dewp": 2.8,
    "wdir": 230,
    "wspd": 6,
    "wgst": 12,
    "visib": 3,
    "altim": 1016.3,
    "slp": 1016.2,
    "wxString": "-RA",
    "presTend": null,
    "maxT": 23.4,
    "minT": 12.3,
    "maxT24": 23.4,
    "minT24": 12.3,
    "precip": 0.01,
    "pcp3hr": 0.1,
    "pcp6hr": 0.23,
    "pcp24hr": 0.53,
    "snow": 1,
    "vertVis": 100,
    "metarType": "METAR",
    "rawOb": "KORD 032151Z 23006KT 10SM BKN110 OVC250 14/03 A3000 RMK AO2 SLP162 VIRGA OHD T01440028",
    "lat": 41.9602,
    "lon": -87.9316,
    "elev": 202,
    "name": "Chicago/O'Hare Intl",
    "clouds": [],
    "fltCat": "IFR"
  */
  // ##############################################################################
  // decode the metar data, some may be missing depending on station report
  JsonObject root_0 = doc[0];
  String StationId = root_0["icaoId"];  // "EGLL"
  String StationName = root_0["name"];  // "London/Heathrow Intl, EN, GB"
  String ReceiptTime = root_0["receiptTime"];  // "2025-09-21T18:54:12.948Z"
  int obsTime = root_0["obsTime"];  // 1758480600
  float Temperature = root_0["temp"];  // 13
  float DewPoint = root_0["dewp"];  // 2
  float WindDir = root_0["wdir"];  // 30
  float WindSpeed = root_0["wspd"];  // 6
  float WindGustSpeed = root_0["wgst"]; // 12
  String Visibility = root_0["visib"];  // "6+" in kts
  float Altimeter = root_0["altim"];         // 1022
  float SeaLevelPressure = root_0["slp"];  // 1017.3
  float PressureTend = root_0["presTend"];
  String WxString = root_0["WxString"]; // -RA 
  float MaxTemp = root_0["maxT"];  // 13
  float MinTemp = root_0["minT"];  // 2
  float MaxTemp24 = root_0["maxT24"];  // 13
  float MinTemp24 = root_0["minT24"];  // 2
  float Preciptation = root_0["precip"]; // 0.01
  float Preciptation3hr  = root_0["pcp3hr"];  // 0.01
  float Preciptation6hr  = root_0["pcp6hr"];  // 0.11
  float Preciptation24hr = root_0["pcp24hr"]; // 0.53
  float Snow = root_0["snow"];  // 0.1
  float VerticalVis = root_0["vertVis"];  // 0.1
  String MessageType = root_0["metarType"];  // "METAR"
  String RawMetar = root_0["rawOb"];  // "METAR EGLL 211850Z AUTO 03006KT 340V060 9999 NCD 13/02 ...
  float Latitude = root_0["lat"];  // 51.477
  float Longitude = root_0["lon"];  // -0.461
  float Elevation = root_0["elev"];  // 26
  String VFR = root_0["fltCat"]; // light category restriction
  int QualityLevel = root_0["qcfield"];
  String StationMode = "MANUAL";
  if (RawMetar.indexOf("AUTO") >= 0) StationMode = "AUTO";
  int winddir = 0;
  if (WindDir.startsWith("VRB")) {
    // Variable wind direction e.g. VRB3Kts
    winddir = WindDir.substring(3).toInt();
  } else winddir = WindDir.toInt();
  // WindDir could also be 21010KT 180V240  if veering
  String Veering = "";
  int WindVeerStart, WindVeerEnd;
  if (RawMetar.indexOf("V") > 15  && RawMetar.indexOf("VRB") == -1 && RawMetar.indexOf("OVC") == -1) {
    Veering = RawMetar.substring(RawMetar.indexOf("V") - 3, RawMetar.indexOf("V") + 4); // To avoid station names with a 'V' in them
  }
  Serial.println(RawMetar.indexOf("VRB"));
  if (Veering != "") {
    WindVeerStart = Veering.substring(0, 3).toInt();
    WindVeerEnd = Veering.substring(4, 7).toInt();
  }
  //==================================================================
  // Print the received values
  Serial.println("Station Id               : " + StationId);
  Serial.println("Station name             : " + StationName);
  Serial.println("Time                     : " + ReceiptTime);
  Serial.println("Observation Time         : " + String(obsTime));
  Serial.println("Receipt Time             : " + ReceiptTime);
  Serial.println("Temperature              : " + String(Temperature, 1));    // in Celcius
  Serial.println("Maximum Temperature      : " + String(MaxTemp, 1));        // in Celcius
  Serial.println("Minimum Temperature      : " + String(MinTemp, 1));        // in Celcius
  Serial.println("Maximum 24hr Temperature : " + String(MaxTemp24, 1));      // in Celcius
  Serial.println("Minimum 24hr Temperature : " + String(MinTemp24, 1));      // in Celcius
  Serial.println("Dew Point                : " + String(DewPoint, 1));       // in Celcius
  Serial.println("Wind Direction           : " + String(winddir));           // in degrees
  if (Veering != ""){
    Serial.println("Wind Veering Dir Start   : " + String(WindVeerStart));     // in degrees
    Serial.println("Wind Veering Dir End     : " + String(WindVeerEnd));       // in degrees
  }
  Serial.println("Windspeed                : " + String(WindSpeed));         // in knots
  Serial.println("Wind Gust Speed          : " + String(WindGustSpeed, 1));  // in knots
  Serial.println("Visibility               : " + String(Visibility));           // in statute Miles
  Serial.println("Vertical Visibility      : " + String(VerticalVis));          // in statute Miles
  Serial.println("Altimeter                : " + String(Altimeter));            //  // in hectopascals
  Serial.println("Sea Level Pressure       : " + String(SeaLevelPressure, 1));  // in hectopascals
  Serial.println("Weather report           : " + WxString);
  Serial.println("Quality                  : " + String(QualityLevel));
  Serial.println("Pressure Tendency        : " + String(PressureTend, 1));      // tendency over last 3 hours in hectopascals
  Serial.println("Preciptation (Now)       : " + String(Preciptation, 1));      // over last hour in inches
  Serial.println("Preciptation (3hrs)      : " + String(Preciptation3hr, 1));   // over last 3-hours in inches
  Serial.println("Preciptation (6hrs)      : " + String(Preciptation6hr, 1));   // over last 6-hours in inches
  Serial.println("Preciptation (24hrs)     : " + String(Preciptation24hr, 1));  // over last 24-hours in inches
  Serial.println("Snow                     : " + String(Snow, 1));              // snow depth in inches
  Serial.println("Metar type               : " + MessageType);
  Serial.println("Raw Metar                : " + RawMetar);
  Serial.println("Latitude                 : " + String(Latitude, 1));   // in degrees
  Serial.println("Longitude                : " + String(Longitude, 1));  // in degrees
  Serial.println("Elevation                : " + String(Elevation));     // in metres
  Serial.println("VFR                      : " + VFR);                   // light category restriction
  Serial.println("Station Mode             : " + StationMode);
//==================================================================
  String cloudcover[5] = {};
  String cloudbase[5] = {};
  const char* root_0_cover = root_0["cover"];  // "SCT"
  String SkyCondition = String(root_0_cover);
  Serial.println("Sky Condition            : " + SkyCondition);
  int cnt = 0;
  for (JsonObject root_0_cloud : root_0["clouds"].as<JsonArray>()) {
    const char* c = root_0_cloud["cover"];  // "FEW", "FEW", "SCT", "SCT"
    cloudcover[cnt] = String(c);
    int root_0_cloud_base = root_0_cloud["base"];  // 5500, 8000, 12000, 16000
    cloudbase[cnt] = String(root_0_cloud_base);
    cnt++;
  }
  for (int i = 0; i < cnt; i++) {
  Serial.println("Cloud cover / Base       : " + cloudcover[i] + ", Cloud base : " + cloudbase[i]);
  }

// There is no Veering report in the metar now in TAF
// There is no Gusting report in the metar now in TAF
// #####################################################################################
  // "2025-09-21T18:54:12.948Z"
  Serial.println(ReceiptTime);
  display_item(0, 0, "Date:" + ReceiptTime.substring(8, 10) + " @ " + ReceiptTime.substring(11, 16), GREEN, 2);  // Date-time
  Serial.println("Date                     : " + ReceiptTime.substring(8, 10) + " @ " + ReceiptTime.substring(11, 16));
  float wind_speedMPH = 0;
  float wind_speedKPH = 0;
  float wind_speedKTS = 0;
  //----------------------------------------------------------------------------------------------------
  // Process any reported wind direction and speed e.g. 270019KTS means direction is 270 deg and speed 19Kts
  // radians = (degrees * 71) / 4068 from Pi/180 to convert to degrees
  Draw_Compass_Rose();  // Draw compass rose
  wind_speedKTS = WindSpeed; // default wind speed is in Knots
  wind_speedMPH = WindSpeed * 1.15077945;
  wind_speedKPH = WindSpeed * 1.852;
  //Serial.println("KTS=" + String(wind_speedKTS));
  //Serial.println("MPH=" + String(wind_speedMPH));
  //Serial.println("KPH=" + String(wind_speedKPH));
   if (wind_speedMPH < 10) display_item((centreX - 28), (centreY + 50), (String(wind_speedMPH) + " MPH"), YELLOW, 2);
  else {
    display_item((centreX - 35), (centreY + 50), (String(wind_speedMPH) + " MPH"), YELLOW, 2);
    if (wind_speedMPH >= 18) display_item((centreX - 35), (centreY + 50), (String(wind_speedMPH) + " MPH"), RED, 2);
  }
  //----------------------------------------------------------------------------------------------------
  // Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
  tft.drawLine(0, 40, 229, 40, YELLOW);
  //  Serial.println("Cloud cover / Base       : " + cloudcover[i] + ", Cloud base : " + cloudbase[i]);
  display_item(0, 45, cloudcover[0] + cloudbase[0], WHITE, 1);
  display_item(0, 55, convert_clouds(cloudcover[1]), WHITE, 1);
  display_item(0, 65, convert_clouds(cloudcover[2]), WHITE, 1);
  display_item(0, 75, convert_clouds(cloudcover[2]), WHITE, 1);
  //----------------------------------------------------------------------------------------------------
  display_item(0, 110, " Temp " + String(Temperature) + char(247) + "C", CYAN, 2);
  //----------------------------------------------------------------------------------------------------
  if (WindSpeed > 3 && Temperature <= 14) {
    int wind_chill = int(calc_windchill(Temperature, WindSpeed));
    display_item(0, 164, "WindC " + String(wind_chill) + char(247) + "C", CYAN, 2);
  }
  //----------------------------------------------------------------------------------------------------
  // Calculate and display Relative Humidity
  display_item(0, 127, " Dewp " + String(DewPoint) + char(247) + "C", CYAN, 2);
  int RH = calc_rh(Temperature, DewPoint);
  display_item(00, 146, "Rel.H " + String(RH) + "%", CYAN, 2);
  //----------------------------------------------------------------------------------------------------
  // Don't display heatindex unless temperature > 18C
  if (Temperature >= 18) {
    float T = (Temperature * 9 / 5) + 32;
    float RHx = RH;
    int tHI = (-42.379 + (2.04901523 * T) + (10.14333127 * RHx) - (0.22475541 * T * RHx) - (0.00683783 * T * T) - (0.05481717 * RHx * RHx) + (0.00122874 * T * T * RHx) + (0.00085282 * T * RHx * RHx) - (0.00000199 * T * T * RHx * RHx) - 32) * 5 / 9;
    display_item(0, 164, "HeatX " + String(tHI) + char(247) + "C", CYAN, 2);
  }
  //where   HI = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH - 0.00683783*T*T - 0.05481717*RH*RH + 0.00122874*T*T*RH + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH
  //tHI = heat index (oF)
  //t = air temperature (oF) (t > 57oF)
  //φ = relative humidity (%)
  //----------------------------------------------------------------------------------------------------
  display_item((centreX - 35), (centreY - 57), String(Altimeter) + "mB", YELLOW, 2);
  //----------------------------------------------------------------------------------------------------
  return true;
}
// finished decoding the METAR string
//----------------------------------------------------------------------------------------------------

String convert_clouds(String source) {
  String height = source.substring(3, 6);
  String cloud = source.substring(0, 3);
  String warning = " ";
  while (height.startsWith("0")) {
    height = height.substring(1);  // trim leading '0'
  }
  if (source.endsWith("TCU") || source.endsWith("CB")) {
    display_item(0, 95, "Warning - storm clouds detected", WHITE, 1);
    warning = " (storm) ";
  }
  // 'adjust offset if 0 replaced by space
  if (cloud != "SKC" && cloud != "CLR" && height != " ") {
    height = " at " + height + "00ft";
  } else height = "";
  if (source == "VV///") {
    return "No cloud reported";
  }
  if (cloud == "BKN") {
    return "Broken" + warning + "clouds" + height;
  }
  if (cloud == "SKC") {
    return "Clear skies";
  }
  if (cloud == "FEW") {
    return "Few" + warning + "clouds" + height;
  }
  if (cloud == "NCD") {
    return "No clouds detected";
  }
  if (cloud == "NSC") {
    return "No signficiant clouds";
  }
  if (cloud == "OVC") {
    return "Overcast" + warning + height;
  }
  if (cloud == "SCT") {
    return "Scattered" + warning + "clouds" + height;
  }
  return "";
}


int min_val(int num1, int num2) {
  if (num1 > num2) {
    return num2;
  } else {
    return num1;
  }
}

float calc_rh(int temp, int dewp) {
  return 100 * (exp((17.271 * dewp) / (237.7 + dewp))) / (exp((17.271 * temp) / (237.7 + temp))) + 0.5;
}

float calc_windchill(int temperature, int wind_speed) {
  float result;
  // Derived from wind_chill = 13.12 + 0.6215 * Tair - 11.37 * POWER(wind_speed,0.16)+0.3965 * Tair * POWER(wind_speed,0.16)
  wind_speed = wind_speed * 1.852;  // Convert to Kph
  result = 13.12 + 0.6215 * temperature - 11.37 * pow(wind_speed, 0.16) + 0.3965 * temperature * pow(wind_speed, 0.16);
  if (result < 0) {
    return result - 0.5;
  } else {
    return result + 0.5;
  }
}

void Process_secondary_reports(String temp_strA, int line_pos) {
  temp_strA.trim();
  if (temp_strA == "NOSIG") {
    display_item(136, line_pos, "No significant change expected", WHITE, 1);
  }
  if (temp_strA == "TEMPO") {
    display_item(136, line_pos, "Temporary conditions expected", WHITE, 1);
  }
  if (temp_strA == "RADZ") {
    display_item(136, line_pos, "Recent Rain/Drizzle", WHITE, 1);
  }
  if (temp_strA == "RERA") {
    display_item(136, line_pos, "Recent Moderate/Heavy Rain", WHITE, 1);
  }
  if (temp_strA == "REDZ") {
    display_item(136, line_pos, "Recent Drizzle", WHITE, 1);
  }
  if (temp_strA == "RESN") {
    display_item(136, line_pos, "Recent Moderate/Heavy Snow", WHITE, 1);
  }
  if (temp_strA == "RESG") {
    display_item(136, line_pos, "Recent Moderate/Heavy Snow grains", WHITE, 1);
  }
  if (temp_strA == "REGR") {
    display_item(136, line_pos, "Recent Moderate/Heavy Hail", WHITE, 1);
  }
  if (temp_strA == "RETS") {
    display_item(136, line_pos, "Recent Thunder storms", WHITE, 1);
  }
}

String display_conditions(String WX_state) {
  if (WX_state == "//") {
    return "No weather reported";
  }
  if (WX_state == "VC") {
    return "Vicinity has";
  }
  if (WX_state == "BL") {
    return "Blowing";
  }
  if (WX_state == "SH") {
    return "Showers";
  }
  if (WX_state == "TS") {
    return "Thunderstorms";
  }
  if (WX_state == "FZ") {
    return "Freezing";
  }
  if (WX_state == "UP") {
    return "Unknown";
  }
  //----------------
  if (WX_state == "MI") {
    return "Shallow";
  }
  if (WX_state == "PR") {
    return "Partial";
  }
  if (WX_state == "BC") {
    return "Patches";
  }
  if (WX_state == "DR") {
    return "Low drifting";
  }
  if (WX_state == "IC") {
    return "Ice crystals";
  }
  if (WX_state == "PL") {
    return "Ice pellets";
  }
  if (WX_state == "GR") {
    return "Hail";
  }
  if (WX_state == "GS") {
    return "Small hail";
  }
  //----------------
  if (WX_state == "DZ") {
    return "Drizzle";
  }
  if (WX_state == "RA") {
    return "Rain";
  }
  if (WX_state == "SN") {
    return "Snow";
  }
  if (WX_state == "SG") {
    return "Snow grains";
  }
  if (WX_state == "DU") {
    return "Widespread dust";
  }
  if (WX_state == "SA") {
    return "Sand";
  }
  if (WX_state == "HZ") {
    return "Haze";
  }
  if (WX_state == "PY") {
    return "Spray";
  }
  //----------------
  if (WX_state == "BR") {
    return "Mist";
  }
  if (WX_state == "FG") {
    return "Fog";
  }
  if (WX_state == "FU") {
    return "Smoke";
  }
  if (WX_state == "VA") {
    return "Volcanic ash";
  }
  if (WX_state == "DS") {
    return "Dust storm";
  }
  if (WX_state == "PO") {
    return "Well developed dust/sand swirls";
  }
  if (WX_state == "SQ") {
    return "Squalls";
  }
  if (WX_state == "FC") {
    return "Funnel clouds/Tornadoes";
  }
  if (WX_state == "SS") {
    return "Sandstorm";
  }
  return "";
}

void display_item(int x, int y, String token, int txt_colour, int txt_size) {
  tft.setCursor(x, y);
  tft.setTextColor(txt_colour);
  tft.setTextSize(txt_size);
  tft.print(token);
  tft.setTextSize(2);  // Back to default text size
}

void display_item_nxy(String token, int txt_colour, int txt_size) {
  tft.setTextColor(txt_colour);
  tft.setTextSize(txt_size);
  tft.print(token);
  tft.setTextSize(2);  // Back to default text size
}

void clear_screen() {
  tft.fillScreen(BLACK);
}

void arrow(int x1, int y1, int x2, int y2, int alength, int awidth, int colour) {
  float distance;
  int dx, dy, x2o, y2o, x3, y3, x4, y4, k;
  distance = sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2));
  dx = x2 + (x1 - x2) * alength / distance;
  dy = y2 + (y1 - y2) * alength / distance;
  k = awidth / alength;
  x2o = x2 - dx;
  y2o = dy - y2;
  x3 = y2o * k + dx;
  y3 = x2o * k + dy;
  //
  x4 = dx - y2o * k;
  y4 = dy - x2o * k;
  tft.drawLine(x1, y1, x2, y2, colour);
  tft.drawLine(x1, y1, dx, dy, colour);
  tft.drawLine(x3, y3, x4, y4, colour);
  tft.drawLine(x3, y3, x2, y2, colour);
  tft.drawLine(x2, y2, x4, y4, colour);
}

void draw_veering_arrow(int a_direction) {
  int dx = (diameter * 0.75 * cos((a_direction - 90) * 3.14 / 180)) + centreX;
  int dy = (diameter * 0.75 * sin((a_direction - 90) * 3.14 / 180)) + centreY;
  arrow(centreX, centreY, dx, dy, 2, 5, RED);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
}

void Draw_Compass_Rose() {
  int dxo, dyo, dxi, dyi;
  tft.drawCircle(centreX, centreY, diameter, GREEN);  // Draw compass circle
  // tft.fillCircle(centreX,centreY,diameter,GREY);  // Draw compass circle
  tft.drawRoundRect((centreX - 45), (centreY - 60), (diameter + 50), (centreY + diameter / 2 + 50), 10, YELLOW);  // Draw compass rose
  tft.drawLine(0, 105, 228, 105, YELLOW);                                                                         // Seperating line for relative-humidity, temp, windchill, temp-index and dewpoint
  tft.drawLine(132, 105, 132, 185, YELLOW);                                                                       // Seperating vertical line for relative-humidity, temp, windchill, temp-index and dewpoint
  for (float i = 0; i < 360; i = i + 22.5) {
    dxo = diameter * cos((i - 90) * 3.14 / 180);
    dyo = diameter * sin((i - 90) * 3.14 / 180);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    tft.drawLine(dxo + centreX, dyo + centreY, dxi + centreX, dyi + centreY, YELLOW);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
    dxo = dxo * 0.5;
    dyo = dyo * 0.5;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    tft.drawLine(dxo + centreX, dyo + centreY, dxi + centreX, dyi + centreY, YELLOW);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
  }
  display_item((centreX - 2), (centreY - 33), "N", GREEN, 1);
  display_item((centreX - 2), (centreY + 26), "S", GREEN, 1);
  display_item((centreX + 30), (centreY - 3), "E", GREEN, 1);
  display_item((centreX - 32), (centreY - 3), "W", GREEN, 1);
}

boolean valid_cloud_report(String temp_strA) {
  if (temp_strA.startsWith("BKN") || temp_strA.startsWith("CLR") || temp_strA.startsWith("FEW") || temp_strA.startsWith("NCD") || temp_strA.startsWith("NSC") || temp_strA.startsWith("OVC") || temp_strA.startsWith("SCT") || temp_strA.startsWith("SKC") || temp_strA.endsWith("CB") || temp_strA.endsWith("TCU")) {
    return true;
  } else {
    return false;
  }
}

void display_status() {
  display_progress("Initialising", 50);
  tft.setTextSize(2);
  display_progress("Waiting for IP address", 75);
  display_progress("Ready...", 100);
  clear_screen();
}

void display_progress(String title, int percent) {
  int title_pos = (320 - title.length() * 12) / 2;  // Centre title
  int x_pos = 35;
  int y_pos = 105;
  int bar_width = 250;
  int bar_height = 15;
  display_item(title_pos, y_pos - 20, title, GREEN, 2);
  tft.drawRoundRect(x_pos, y_pos, bar_width + 2, bar_height, 5, YELLOW);                            // Draw progress bar outline
  tft.fillRoundRect(x_pos + 2, y_pos + 1, percent * bar_width / 100 - 2, bar_height - 3, 4, BLUE);  // Draw progress
  delay(2000);
  tft.fillRect(x_pos - 30, y_pos - 20, 320, 16, BLACK);  // Clear titles
}

/*  Meaning of various codes
  Moderate/heavy rain RERA
  Moderate/heavy snow RESN
  Moderate/heavy small hail REGS
  Moderate/heavy snow pellets REGS Moderate/heavy ice pellets REPL
  Moderate/heavy hail REGR
  Moderate/heavy snow grains RESG

  Intensity       Description     Precipitation       Obscuration      Other
  -  Light        MI Shallow      DZ Drizzle          BR Mist          PO Well developed dust / sand whirls
    Moderate     PR Partial      RA Rain             FG Fog           SQ Squalls
  +  Heavy        BC Patches      SN Snow             FU Smoke         FC Funnel clouds inc tornadoes or waterspouts
  VC Vicinity     DR Low drifting SG Snow grains      VA Volcanic ash  SS Sandstorm
  BL Blowing      IC Ice crystals DU Widespread dust  DS Duststorm
  SH Showers      PL Ice pellets  SA Sand
  TS Thunderstorm GR Hail         HZ Haze
  FZ Freezing     GS Small hail   PY Spray
  UP Unknown

  e.g. -SHRA - Light showers of rain
  TSRA - Thunderstorms and rain.
*/



/* Returns the following METAR data from the server address
  <?xml version="1.0" encoding="UTF-8"?>
  -<response xsi:noNamespaceSchemaLocation="http://aviationweather.gov/adds/schema/metar1_2.xsd" version="1.2" xmlns:xsi="http://www.w3.org/2001/XML-Schema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <request_index>18793217</request_index>
  <data_source name="metars"/>
  <request type="retrieve"/>
  <errors/>
  <warnings/>
  <time_taken_ms>4</time_taken_ms>
  -<data num_results="2">
  -<METAR>
  <raw_text>EGLL 241950Z AUTO 07009KT 9999 NCD 03/M02 Q1023</raw_text>
  <station_id>EGLL</station_id>
  <observation_time>2018-02-24T19:50:00Z</observation_time>
  <latitude>51.48</latitude>
  <longitude>-0.45</longitude>
  <temp_c>3.0</temp_c>
  <dewpoint_c>-2.0</dewpoint_c>
  <wind_dir_degrees>70</wind_dir_degrees>
  <wind_speed_kt>9</wind_speed_kt>
  <visibility_statute_mi>6.21</visibility_statute_mi>
  <altim_in_hg>30.206694</altim_in_hg>
  -<quality_control_flags>
  <auto>TRUE</auto>
  </quality_control_flags>
  <sky_condition sky_cover="CLR"/>
  <flight_category>VFR</flight_category>
  <metar_type>METAR</metar_type>
  <elevation_m>24.0</elevation_m>
  </METAR>
  -<METAR>
  <raw_text>EGLL 241920Z AUTO 07008KT 9999 NCD 03/M03 Q1023</raw_text>
  <station_id>EGLL</station_id>
  <observation_time>2018-02-24T19:20:00Z</observation_time>
  <latitude>51.48</latitude>
  <longitude>-0.45</longitude>
  <temp_c>3.0</temp_c>
  <dewpoint_c>-3.0</dewpoint_c>
  <wind_dir_degrees>70</wind_dir_degrees>
  <wind_speed_kt>8</wind_speed_kt>
  <visibility_statute_mi>6.21</visibility_statute_mi>
  <altim_in_hg>30.206694</altim_in_hg>
  -<quality_control_flags>
  <auto>TRUE</auto>
  </quality_control_flags>
  <sky_condition sky_cover="CLR"/>
  <flight_category>VFR</flight_category>
  <metar_type>METAR</metar_type>
  <elevation_m>24.0</elevation_m>
  </METAR>
  </data>
  </response>
  <METAR>
  <raw_text>EGDM 121621Z 35005KT 9999 5000SW BR FEW001 BKN002 11/11 Q1014 AMB</raw_text>
  <station_id>EGDM</station_id>
  <observation_time>2016-11-12T16:21:00Z</observation_time>
  <latitude>51.17</latitude>
  <longitude>-1.75</longitude>
  <temp_c>11.0</temp_c>
  <dewpoint_c>11.0</dewpoint_c>
  <wind_dir_degrees>350</wind_dir_degrees>
  <wind_speed_kt>5</wind_speed_kt>
  <visibility_statute_mi>6.21</visibility_statute_mi>
  <altim_in_hg>29.940945</altim_in_hg>
  <wx_string>BR</wx_string>
  <sky_condition sky_cover="FEW" cloud_base_ft_agl="100" />
  <sky_condition sky_cover="BKN" cloud_base_ft_agl="200" />
  <flight_category>LIFR</flight_category>
  <metar_type>METAR</metar_type>
  <elevation_m>124.0</elevation_m>
  </METAR>
*/
