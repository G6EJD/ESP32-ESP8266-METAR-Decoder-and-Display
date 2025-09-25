te /*  Revised METAR Decoder and display for ESP32 SCT01
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
// ESP32 Dev Module
// Screen size = 480 x 320 (size ratio relative to 320x240 = 1.5 x 1.33
// Screen Name = ESP32 SCT01
// Drive Type  = ST7796
////////////////////////////////////////////////////////////////////////////////////
String version_num = "METAR ESP Version 1.0";
#include <WiFi.h>
// #define LGFX_M5STACK                       // M5Stack M5Stack Basic / Gray / Go / Fire
// #define LGFX_M5STACK_CORE2                 // M5Stack M5Stack Core2
// #define LGFX_M5STACK_COREINK               // M5Stack M5Stack CoreInk
// #define LGFX_M5STICK_C                     // M5Stack M5Stick C / CPlus
// #define LGFX_M5PAPER                       // M5Stack M5Paper
// #define LGFX_M5TOUGH                       // M5Stack M5Tough
// #define LGFX_M5ATOMS3                      // M5Stack M5ATOMS3
// #define LGFX_ODROID_GO                     // ODROID-GO
// #define LGFX_TTGO_TS                       // TTGO TS
// #define LGFX_TTGO_TWATCH                   // TTGO T-Watch
// #define LGFX_TTGO_TWRISTBAND               // TTGO T-Wristband
// #define LGFX_TTGO_TDISPLAY                 // TTGO T-Display
// #define LGFX_DDUINO32_XS                   // DSTIKE D-duino-32 XS
// #define LGFX_LOLIN_D32_PRO                 // LoLin D32 Pro
// #define LGFX_LOLIN_S3_PRO                  // LoLin S3 Pro
// #define LGFX_ESP_WROVER_KIT                // Espressif ESP-WROVER-KIT
// #define LGFX_ESP32_S3_BOX                  // Espressif ESP32-S3-BOX
// #define LGFX_ESP32_S3_BOX_LITE             // Espressif ESP32-S3-BOX Lite
// #define LGFX_ESP32_S3_BOX_V3               // Espressif ESP32-S3-BOX-3/3B
// #define LGFX_WIFIBOY_PRO                   // WiFiBoy Pro
// #define LGFX_WIFIBOY_MINI                  // WiFiBoy mini
// #define LGFX_MAKERFABS_TOUCHCAMERA         // Makerfabs Touch with Camera
// #define LGFX_MAKERFABS_MAKEPYTHON          // Makerfabs MakePython
// #define LGFX_MAKERFABS_TFT_TOUCH_SPI       // Makerfabs TFT Touch SPI
// #define LGFX_MAKERFABS_TFT_TOUCH_PARALLEL16// Makerfabs TFT Touch Parallel 16
#define LGFX_WT32_SC01  // Seeed WT32-SC01
// #define LGFX_WIO_TERMINAL                  // Seeed Wio Terminal
// #define LGFX_PYBADGE                       // Adafruit PyBadge
// #define LGFX_FUNHOUSE                      // Adafruit FunHouse
// #define LGFX_FEATHER_ESP32_S2_TFT          // Adafruit Feather ESP32 S2 TFT
// #define LGFX_FEATHER_ESP32_S3_TFT          // Adafruit Feather ESP32 S3 TFT
// #define LGFX_ESPBOY                        // ESPboy
// #define LGFX_WYWY_ESP32S3_HMI_DEVKIT       // wywy ESP32S3 HMI DevKit
// #define LGFX_SUNTON_ESP32_2432S028         // Sunton ESP32 2432S028

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

static LGFX lcd;

#include <WiFiClientSecure.h>
#include "HTTPClient.h"
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson needs version v7 or above

const char* ssid = "yourWiFISSID";
const char* password = "yourWiFiPASSWORD";
const int httpsPort = 443;

WiFiClientSecure client;

const int centreX = 409;  // x,y Location of the compass display on screen
const int centreY = 83;
const int diameter = 60;  // Size of the compass

// Assign human-readable names to common 16-bit color values:
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define timeDelay 15000

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(__FILE__);
  lcd.init();
  lcd.setRotation(3);      // 0-3
  lcd.setBrightness(128);  // 0-255
  lcd.setColorDepth(16);   // RGB565
  //lcd.setColorDepth(24);  // RGB888の24 RGB666の18
  lcd.startWrite();
  clear_screen();
  display_progress("Connecting to Network", 25);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected at: " + WiFi.localIP().toString());
  delay(2000);
  display_status();
}

void loop() {
  // Change these METAR Stations to suit your needs see: Use this URL address = ftp://tgftp.nws.noaa.gov/data/observations/metar/decoded/
  // to establish your list of sites to retrieve (you must know the 4-letter
  // site dentification)KCHA,KRMG,KVPC,
  GET_METAR("KCHA", "1 KCHA Lovell Field, Tennessee");
  GET_METAR("EGGD", "2 EGGD Bristol/Lulsgate");
  GET_METAR("KRMG", "3 KRMG Richard B Russell Airport");
  GET_METAR("WSSS", "4 WSSS · Singapore Changi Airport");
  GET_METAR("EGSS", "5 EGSS Stansted");
}

void GET_METAR(String station, String StationName) {  //client function to send/receive GET request data.
  String metar, raw_metar;
  bool metar_status = true;
  String StationStatus;
  display_item(30, 145, "Decoding METAR for " + station, GREEN, 3);
  delay(2000);
  // https://aviationweather.gov/api/data/stationinfo?ids=EGLL
  // https://aviationweather.gov/api/data/metar?format=json&hours=0&ids=EGLL&hoursBeforeNow=1
  // https://aviationweather.gov/api/data/metar?format=xml&hours=0&ids=NULL,KCHA,KRMG,KVPC,KATL,KLAL,KGOV,KPLN,KSDF,KDTW,KMYR,CYDC,EGLL test call
  const char* host = "https://aviationweather.gov";
  String url = String(host) + "/api/data/metar?format=json&ids=" + station + "&hoursBeforeNow=1";
  Serial.println("Connected, Requesting data for : " + StationName);
  HTTPClient http;
#ifdef ESP32
  http.begin(url.c_str());  // Specify the URL and maybe a certificate
#else
  WiFiClient client;
  http.begin(client, url);  // Specify the URL and maybe a certificate
#endif
  int httpCode = http.GET();  // Start connection and send HTTP header
  Serial.println("Connection status: " + String(httpCode > 0 ? "Connected" : "Connection Error"));
  if (httpCode > 0) {  // HTTP header has been sent and Server response header has been handled
    if (httpCode == HTTP_CODE_OK) metar = http.getString();
    http.end();
    metar_status = true;
    Serial.println(metar);
  } else {
    http.end();
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    metar_status = false;
    StationStatus = "Station off-air";
  }
  client.stop();
  clear_screen();  // Clear screen
  display_item(415, 305, "Connected", GREEN, 1);
  display_item(5, 305, version_num, GREEN, 1);
  display_item(5, 260, StationName, YELLOW, 2);
  lcd.drawLine(0, 247, 480, 247, YELLOW);
  if (metar_status == true) {
    display_metar(metar);
    delay(timeDelay);  // Delay for set time
  } else {
    display_item(100, 150, StationStatus, YELLOW, 2);  // Now decode METAR
    delay(5000);                                      // Wait less time if station off-air
  }
  clear_screen();  // Clear screen before moving to next station display
}

bool display_metar(String metar) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, metar);
  if (error) { // Test if parsing succeeds.
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
  String StationId = root_0["icaoId"];         // "EGLL"
  String StationName = root_0["name"];         // "London/Heathrow Intl, EN, GB"
  String ReceiptTime = root_0["receiptTime"];  // "2025-09-21T18:54:12.948Z"
  int obsTime = root_0["obsTime"];             // 1758480600
  float Temperature = root_0["temp"];          // 13
  float DewPoint = root_0["dewp"];             // 2
  String WindDir = root_0["wdir"];             // 30
  float WindSpeed = root_0["wspd"];            // 6
  float WindGustSpeed = root_0["wgst"];        // 12
  String Visibility = root_0["visib"];         // "6+" in kts
  int Altimeter = root_0["altim"];             // 1022
  float SeaLevelPressure = root_0["slp"];      // 1017.3
  float PressureTend = root_0["presTend"];
  String WxString = root_0["WxString"];        // -RA
  float MaxTemp = root_0["maxT"];              // 13
  float MinTemp = root_0["minT"];              // 2
  float MaxTemp24 = root_0["maxT24"];          // 13
  float MinTemp24 = root_0["minT24"];          // 2
  float Preciptation = root_0["precip"];       // 0.01
  float Preciptation3hr = root_0["pcp3hr"];    // 0.01
  float Preciptation6hr = root_0["pcp6hr"];    // 0.11
  float Preciptation24hr = root_0["pcp24hr"];  // 0.53
  float Snow = root_0["snow"];                 // 0.1
  float VerticalVis = root_0["vertVis"];       // 0.1
  String MessageType = root_0["metarType"];    // "METAR"
  String RawMetar = root_0["rawOb"];           // "METAR EGLL 211850Z AUTO 03006KT 340V060 9999 NCD 13/02 ...
  float Latitude = root_0["lat"];              // 51.477
  float Longitude = root_0["lon"];             // -0.461
  float Elevation = root_0["elev"];            // 26
  String VFR = root_0["fltCat"];               // light category restriction
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
  if ((RawMetar.indexOf("V") > 15) && (RawMetar.indexOf("VRB") < 0) && (RawMetar.indexOf("OVC") < 0)) {
    Veering = RawMetar.substring(RawMetar.indexOf("V") - 3, RawMetar.indexOf("V") + 4);  // To avoid station names with a 'V' in them
  }
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
  Serial.println("Temperature              : " + String(Temperature, 1));  // in Celcius
  Serial.println("Maximum Temperature      : " + String(MaxTemp, 1));      // in Celcius
  Serial.println("Minimum Temperature      : " + String(MinTemp, 1));      // in Celcius
  Serial.println("Maximum 24hr Temperature : " + String(MaxTemp24, 1));    // in Celcius
  Serial.println("Minimum 24hr Temperature : " + String(MinTemp24, 1));    // in Celcius
  Serial.println("Dew Point                : " + String(DewPoint, 1));     // in Celcius
  Serial.println("Wind Direction           : " + String(winddir));         // in degrees
  if (Veering != "") {
    Serial.println("Wind Veering Dir Start   : " + String(WindVeerStart));  // in degrees
    Serial.println("Wind Veering Dir End     : " + String(WindVeerEnd));    // in degrees
  }
  Serial.println("Windspeed                : " + String(WindSpeed));            // in knots
  Serial.println("Wind Gust Speed          : " + String(WindGustSpeed, 1));     // in knots
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
  // #####################################################################################
  // "2025-09-21T18:54:12.948Z"
  int lineLength = 75;
  if (RawMetar.length() > lineLength) {
    display_item(5, 280, RawMetar.substring(0, lineLength), YELLOW, 1);
    String Line2 = RawMetar.substring(lineLength);
    Line2.trim();
    display_item(5, 290, Line2, YELLOW, 1);
  }
  else display_item(5, 280, RawMetar, YELLOW, 1);
  display_item(5, 0, "Date:" + ReceiptTime.substring(8, 10) + " @ " + ReceiptTime.substring(11, 16), GREEN, 2);  // Date-time
  Serial.println("Date                     : " + ReceiptTime.substring(8, 10) + " @ " + ReceiptTime.substring(11, 16));
  lcd.drawLine(0, 20, 345, 20, YELLOW);
  //----------------------------------------------------------------------------------------------------
  // Process any reported wind direction and speed e.g. 270019KTS means direction is 270 deg and speed 19Kts
  Draw_Compass_Rose();              // Draw compass rose
  float wind_speedKTS = WindSpeed;  // default wind speed is in Knots
  float wind_speedMPH = WindSpeed * 1.15077945;
  float wind_speedKPH = WindSpeed * 1.852;
  if (wind_speedMPH < 10) display_item((centreX - 40), (centreY + 70), (String(wind_speedMPH, 0) + " MPH"), YELLOW, 2);
  else {
    if (wind_speedMPH >= 18) display_item((centreX - 45), (centreY + 70), (String(wind_speedMPH, 0) + " MPH"), RED, 2);
    else display_item((centreX - 40), (centreY + 70), (String(wind_speedMPH, 0) + " MPH"), YELLOW, 2);
  }
  if (WindDir.startsWith("VRB")) {
    display_item((centreX - 17), (centreY - 7), "VRB", YELLOW, 2);
  } else {
    winddir = winddir - 90;
    int dx = (diameter * cos((winddir)*0.017444)) + centreX;
    int dy = (diameter * sin((winddir)*0.017444)) + centreY;
    arrow(dx, dy, centreX, centreY, 5, 5, YELLOW);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
  }
  if (WindDir != "CAVOK" && Veering != "") {  // Check for variable wind direction
    // Minimum angle is either ABS(AngleA- AngleB) or (360-ABS(AngleA-AngleB))
    int veering = min_val(360 - abs(WindVeerStart - WindVeerEnd), abs(WindVeerStart - WindVeerEnd));
    display_item(150, (centreY + 65), "V " + String(veering) + char(247), RED, 2);
    display_item((centreX - 60), (centreY + 68), "v", RED, 2);  // Signify 'Variable wind direction
    draw_veering_arrow(WindVeerStart);
    draw_veering_arrow(WindVeerEnd);
  }
  //----------------------------------------------------------------------------------------------------
  // Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
  lcd.drawLine(0, 137, 340, 137, YELLOW);
  // Serial.println("Cloud cover / Base       : " + cloudcover[i] + ", Cloud base : " + cloudbase[i]);
  display_item(5, 35, convertClouds("", cloudcover[0]), WHITE, 2);
  display_item(5, 55, convertClouds("", cloudcover[1]), WHITE, 2);
  display_item(5, 75, convertClouds("", cloudcover[2]), WHITE, 2);
  display_item(5, 95, convertClouds("", cloudcover[3]), WHITE, 2);
  if (cloudbase[1].toInt() > 0) display_item(235, 55, String(cloudbase[1].toInt()) + " ft", WHITE, 2);
  if (cloudbase[0].toInt() > 0) display_item(235, 35, String(cloudbase[0].toInt()) + " ft", WHITE, 2);
  if (cloudbase[2].toInt() > 0) display_item(235, 75, String(cloudbase[2].toInt()) + " ft", WHITE, 2);
  if (cloudbase[3].toInt() > 0) display_item(235, 95, String(cloudbase[3].toInt()) + " ft", WHITE, 2);
  if (SkyCondition == "CLR") display_item(5, 35, "No Clouds Detected", WHITE, 2);
  //----------------------------------------------------------------------------------------------------
  display_item(5, 145, " Temp " + String(Temperature, 0) + char(247) + "C", CYAN, 2);
  //----------------------------------------------------------------------------------------------------
  lcd.drawLine(140, 139, 140, 246, YELLOW);   // Separating vertical line for relative-humidity, temp, windchill, temp-index and dewpoint
  //----------------------------------------------------------------------------------------------------
  int wind_chill = int(calc_windchill(Temperature, WindSpeed));
  if (WindSpeed > 3 && Temperature <= 14) {
    display_item(5, 165, "WindC N/A", CYAN, 2);
  } else display_item(5, 165, "WindC " + String(wind_chill) + char(247) + "C", CYAN, 2);
  //----------------------------------------------------------------------------------------------------
  // Calculate and display Relative Humidity
  display_item(5, 185, " Dewp " + String(DewPoint, 0) + char(247) + "C", CYAN, 2);
  int RH = calc_rh(Temperature, DewPoint);
  display_item(5, 205, "Rel.H " + String(RH) + "%", CYAN, 2);
  //----------------------------------------------------------------------------------------------------
  // Don't display heatindex unless temperature > 18C
  if (Temperature >= 17) {
    float T = (Temperature * 9 / 5) + 32;
    float RHx = RH;
    int tHI = (-42.379 + (2.04901523 * T) + (10.14333127 * RHx) - (0.22475541 * T * RHx) - (0.00683783 * T * T) - (0.05481717 * RHx * RHx) + (0.00122874 * T * T * RHx) + (0.00085282 * T * RHx * RHx) - (0.00000199 * T * T * RHx * RHx) - 32) * 5 / 9;
    display_item(5, 225, "HeatX " + String(tHI) + char(247) + "C", CYAN, 2);
  }
  //where   HI = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH - 0.00683783*T*T - 0.05481717*RH*RH + 0.00122874*T*T*RH + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH
  //tHI = heat index (oF)
  //t = air temperature (oF) (t > 57oF)
  //φ = relative humidity (%)
  //----------------------------------------------------------------------------------------------------
  display_item((centreX - 40), (centreY - 82), String(Altimeter) + "mB", YELLOW, 2);
  //---------------------------------------------------------------------------------------------------
  // Process any weather codes e.g. -RA
  String Report, Intensity;
  // Remove common intensity indicators
  if (WxString.startsWith("-")) {
    Intensity = "is Light ";
    WxString = WxString.substring(1);
  }
  if (WxString.startsWith("+")) {
    Intensity = " is Heavy ";
    WxString = WxString.substring(1);
  }
  if (WxString.startsWith("VC")) {
    Intensity = " is in the vicinity ";
    WxString = WxString.substring(2);
  }
  Report = display_conditions(WxString) + Intensity;
  display_item(150, 205, Report, YELLOW, 2);
  if (WxString == "" || WxString == "null") { // No weather report, but...
    if (RawMetar.indexOf("-RA") > 0) WxString = "Light Rain";
    if (RawMetar.indexOf("+RA") > 0) WxString = "Heavy Rain";
    if (RawMetar.indexOf("RA") > 0) WxString  = "Moderate Rain";
    if (RawMetar.indexOf("NOSIG") > 0) WxString  = "No Significant Change";
  }
  display_item(150, 225, "Wx: " + WxString, YELLOW, 2);
  return true;
}

//----------------------------------------------------------------------------------------------------
String convertClouds(String source, String cloud) {
  String warning = " ";
  if (source.endsWith("TCU") || source.endsWith("CB")) {
    display_item(5, 108, "Warning - storm clouds detected", WHITE, 1);
    warning = " (storm) ";
  }
  if (source == "VV///") return "No cloud reported";
  if (cloud == "BKN") return "Broken" + warning + "clouds";
  if (cloud == "SKC") return "Clear skies";
  if (cloud == "FEW") return "Few" + warning + "clouds";
  if (cloud == "NCD") return "No clouds detected";
  if (cloud == "NSC") return "No signficiant clouds";
  if (cloud == "OVC") return "Overcast" + warning;
  if (cloud == "SCT") return "Scattered" + warning + "clouds";
  return "";
}


int min_val(int num1, int num2) {
  if (num1 > num2) return num2;
  else return num1;
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
  String report = "";
  line_pos += 5;
  int x_pos = 205;
  if (temp_strA == "NOSIG") report = "No significant change expected";
  if (temp_strA == "TEMPO") report = "Temporary conditions expected";
  if (temp_strA == "RADZ") report = "Recent Rain/Drizzle";
  if (temp_strA == "RERA") report = "Recent Moderate/Heavy Rain";
  if (temp_strA == "REDZ") report = "Recent Drizzle";
  if (temp_strA == "RESN") report = "Recent Moderate/Heavy Snow";
  if (temp_strA == "RESG") report = "Recent Moderate/Heavy Snow grains";
  if (temp_strA == "REGR") report = "Recent Moderate/Heavy Hail";
  if (temp_strA == "RETS") report = "No significant change expected";
  display_item(x_pos, line_pos, report, WHITE, 1);
}

String display_conditions(String WxState) {
  if (WxState == "//") {
    return "No weather reported";
  }
  if (WxState == "VC") {
    return "Vicinity has";
  }
  if (WxState == "BL") {
    return "Blowing";
  }
  if (WxState == "SH") {
    return "Showers";
  }
  if (WxState == "TS") {
    return "Thunderstorms";
  }
  if (WxState == "FZ") {
    return "Freezing";
  }
  if (WxState == "UP") {
    return "Unknown";
  }
  if (WxState == "MI") {
    return "Shallow";
  }
  if (WxState == "PR") {
    return "Partial";
  }
  if (WxState == "BC") {
    return "Patches";
  }
  if (WxState == "DR") {
    return "Low drifting";
  }
  if (WxState == "IC") {
    return "Ice crystals";
  }
  if (WxState == "PL") {
    return "Ice pellets";
  }
  if (WxState == "GR") {
    return "Hail";
  }
  if (WxState == "GS") {
    return "Small hail";
  }
  if (WxState == "DZ") {
    return "Drizzle";
  }
  if (WxState == "RA") {
    return "Rain";
  }
  if (WxState == "SN") {
    return "Snow";
  }
  if (WxState == "SG") {
    return "Snow grains";
  }
  if (WxState == "DU") {
    return "Widespread dust";
  }
  if (WxState == "SA") {
    return "Sand";
  }
  if (WxState == "HZ") {
    return "Haze";
  }
  if (WxState == "PY") {
    return "Spray";
  }
  if (WxState == "BR") {
    return "Mist";
  }
  if (WxState == "FG") {
    return "Fog";
  }
  if (WxState == "FU") {
    return "Smoke";
  }
  if (WxState == "VA") {
    return "Volcanic ash";
  }
  if (WxState == "DS") {
    return "Dust storm";
  }
  if (WxState == "PO") {
    return "Well developed dust/sand swirls";
  }
  if (WxState == "SQ") {
    return "Squalls";
  }
  if (WxState == "FC") {
    return "Funnel clouds/Tornadoes";
  }
  if (WxState == "SS") {
    return "Sandstorm";
  }
  return "";
}

void display_item(int x, int y, String token, int txt_colour, int txt_size) {
  lcd.setCursor(x, y);
  lcd.setTextColor(txt_colour);
  lcd.setTextSize(txt_size);
  lcd.print(token);
  lcd.setTextSize(2);  // Back to default text size
}

void display_item_nxy(String token, int txt_colour, int txt_size) {
  lcd.setTextColor(txt_colour);
  lcd.setTextSize(txt_size);
  lcd.print(token);
  lcd.setTextSize(2);  // Back to default text size
}

void clear_screen() {
  lcd.clear();
  lcd.startWrite();
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
  lcd.drawLine(x1, y1, x2, y2, colour);
  lcd.drawLine(x1, y1, dx, dy, colour);
  lcd.drawLine(x3, y3, x4, y4, colour);
  lcd.drawLine(x3, y3, x2, y2, colour);
  lcd.drawLine(x2, y2, x4, y4, colour);
}

void draw_veering_arrow(int a_direction) {
  int dx = (diameter * 0.75 * cos((a_direction - 90) * 3.14 / 180)) + centreX;
  int dy = (diameter * 0.75 * sin((a_direction - 90) * 3.14 / 180)) + centreY;
  arrow(centreX, centreY, dx, dy, 2, 5, RED);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
}

void Draw_Compass_Rose() {
  int dxo, dyo, dxi, dyi;
  lcd.drawCircle(centreX, centreY, diameter, GREEN);  // Draw compass circle
  // lcd.fillCircle(centreX,centreY,diameter,GREY);  // Draw compass circle
  lcd.drawRoundRect((centreX - 67), (centreY - 64), (diameter + 75), (centreY + diameter / 2 + 40), 10, YELLOW);  // Draw compass rose
  for (float i = 0; i < 360; i = i + 22.5) {
    dxo = diameter * cos((i - 90) * 3.14 / 180);
    dyo = diameter * sin((i - 90) * 3.14 / 180);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    lcd.drawLine(dxo + centreX, dyo + centreY, dxi + centreX, dyi + centreY, YELLOW);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
    dxo = dxo * 0.5;
    dyo = dyo * 0.5;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    lcd.drawLine(dxo + centreX, dyo + centreY, dxi + centreX, dyi + centreY, YELLOW);  // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
  }
  display_item((centreX - 2), (centreY - 33), "N", GREEN, 1);
  display_item((centreX - 2), (centreY + 26), "S", GREEN, 1);
  display_item((centreX + 30), (centreY - 3), "E", GREEN, 1);
  display_item((centreX - 32), (centreY - 3), "W", GREEN, 1);
}

void display_status() {
  display_progress("Initialising", 50);
  lcd.setTextSize(2);
  display_progress("Waiting for Network", 75);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
  }
  display_progress("Ready...", 100);
  clear_screen();
}

void display_progress(String title, int percent) {
  int title_pos = (320 - title.length() * 12) / 2;  // Centre title
  int x_pos = 40;
  int y_pos = 105;
  int bar_width = 250;
  int bar_height = 15;
  display_item(title_pos * 1.5, (y_pos - 20) * 1.33, title, GREEN, 2);
  lcd.drawRoundRect(x_pos * 1.5, y_pos * 1.33, bar_width + 2, bar_height, 5, YELLOW);                                // Draw progress bar outline
  lcd.fillRoundRect((x_pos + 2) * 1.5, (y_pos + 1) * 1.33, percent * bar_width / 100 - 2, bar_height - 3, 4, BLUE);  // Draw progress
  lcd.fillRect((x_pos - 30) * 1.5, (y_pos - 20) * 1.33, 320, 16, BLACK);  // Clear titles
  lcd.startWrite();
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

