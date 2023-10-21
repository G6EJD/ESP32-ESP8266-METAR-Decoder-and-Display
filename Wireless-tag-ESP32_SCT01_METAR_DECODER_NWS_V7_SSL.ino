/*  Version 7 METAR Decoder and display for ESP32 and ST7796 lcd screen

  This software, the ideas and concepts is Copyright (c) David Bird 2022.
  All rights to this software are reserved.

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
// Screen size = 480 x 320 (size ratio relative to 320x240 = 1.5 x 1.33
// Screen Name = ESP32 SCT01
// Drive Type  = ST7796
////////////////////////////////////////////////////////////////////////////////////
String version_num = "METAR ESP Version 7.0";
#include <WiFi.h>
#define  LGFX_AUTODETECT
#define  LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>  // "LGFX"
#define  LGFX_WT32_SC01

static LGFX lcd;                // LGFX

#include <WiFiClientSecure.h>
#include "HTTPClient.h"

const char *ssid      = "yourSSID";
const char *password  = "yourPASSWORD";
const char* host      = "https://aviationweather.gov";
const int   httpsPort = 443;

WiFiClientSecure client;

const int centreX  = 409; // x,y Location of the compass display on screen
const int centreY  = 83;
const int diameter = 60;  // Size of the compass

// Assign human-readable names to common 16-bit color values:
#define BLACK       0x0000
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(1); // 0-3
  lcd.setBrightness(128); // 0-255
  lcd.setColorDepth(16);  // RGB565
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
  display_status();
}

void loop() {
  // Change these METAR Stations to suit your needs see: Use this URL address = ftp://tgftp.nws.noaa.gov/data/observations/metar/decoded/
  // to establish your list of sites to retrieve (you must know the 4-letter
  // site dentification)
  GET_METAR("EGGD", "1 EGGD Bristol/Lulsgate");
  GET_METAR("EGVN", "2 EGVN Brize Norton");
  GET_METAR("EGCC", "3 EGCC Manchester Airport");
  GET_METAR("EGHQ", "4 EGHQ Newquay");
  GET_METAR("EGSS", "5 EGSS Stansted");
}

//----------------------------------------------------------------------------------------------------
void GET_METAR(String Station, String Name) { //client function to send/receive GET request data.
  String metar, raw_metar;
  bool metar_status    = true;
  const int time_delay = 20000;
  display_item(35, 100, "Decoding METAR", GREEN, 3);
  display_item(90, 135, "for " + Station, GREEN, 3);
  // http://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&stationString=EGLL&hoursBeforeNow=1 (example)
  // https://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&stationString=EGLL&hoursBeforeNow=1
  // ftp://tgftp.nws.noaa.gov/data/observations/metar/decoded/EGCC.TXT
  String uri = "/cgi-bin/data/metar.php?dataSource=metars&requestType=retrieve&format=xml&ids=" + station + "&hoursBeforeNow=1";
  //String uri = "/adds/dataserver_current/httpparam?datasource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=1.25&stationString=" + Station;
  Serial.println("Connected, \nRequesting data for : " + Name);
  HTTPClient http;
  http.begin(host + uri);    // Specify the URL and certificate
  int httpCode = http.GET(); // Start connection and send HTTP header
  if (httpCode > 0) {        // HTTP header has been sent and Server response header has been handled
    if (httpCode == HTTP_CODE_OK) raw_metar = http.getString();
    http.end();
    metar = raw_metar.substring(raw_metar.indexOf("<raw_text>", 0) + 10, raw_metar.indexOf("</raw_text>", 0));
  }
  else
  {
    http.end();
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    metar_status = false;
    metar = "Station off-air";
  }
  display_item(265, 230, "Connected", RED, 1);
  Serial.println(metar);
  client.stop();
  clear_screen(); // Clear screen
  display_item(400, 306, "Connected", RED, 1);
  display_item(10, 306, version_num, GREEN, 1);
  display_item(10, 253, Name, YELLOW, 2);
  lcd.drawLine(0, 246, 480, 246, YELLOW);
  display_item(10, 279, metar, YELLOW, 1);
  if (metar_status == true) {
    display_metar(metar);
    lcd.display();
    delay(time_delay);  // Delay for set time
  }
  else
  {
    display_item(105, 133, metar, YELLOW, 2); // Now decode METAR
    lcd.display();
    delay(5000);      // Wait less time if station off-air
  }
  clear_screen();     // Clear screen before moving to next station display
  lcd.startWrite();
}

void display_metar(String metar) {
  int temperature   = 0;
  int dew_point     = 0;
  int wind_speedKTS = 0;
  char str[130]     = "";
  char *parameter;  // a pointer variable
  String conditions_start = " ";
  String temp_strA  = "";
  String temp_strB  = "";
  String temp_Pres  = "";
  String conditions_test = "+-BDFGHIMPRSTUV"; // Test for light or heavy (+/-) BR - Mist, RA - Rain  SH-Showers VC - Vicinity, etc
  // Test metar, press enter after colon to include for testing:metar = "EGDM 261550Z AUTO 26517G23KT 250V290 6999 R04/1200 4200SW -SH SN SCT002TCU NCD FEW222/// SCT090 28/M15 Q1001 RERA NOSIG BLU";
  // Test metar exercises all decoder functions on screen
  metar.toCharArray(str, 130);
  parameter = strtok(str, " "); // Find tokens that are seperated by SPACE
  while (parameter != NULL) {
    //display_item(5,0, String(parameter),RED,2); // Station Name - omitted at V2.01
    temp_strA = strtok(NULL, " ");
    display_item(5, 0, "Date:" + temp_strA.substring(0, 2) + " @ " + temp_strA.substring(2, 6) + "Hr", GREEN, 2); // Date-time

    //----------------------------------------------------------------------------------------------------
    // Process any reported station type e.g. AUTO means Automatic
    temp_strA = strtok(NULL, " ");
    if (temp_strA == "AUTO") {
      //display_item(54,0,"A",CYAN,2); // Omitted at V2.01
      temp_strA = strtok(NULL, " ");
    }

    //----------------------------------------------------------------------------------------------------
    // Process any reported wind direction and speed e.g. 270019KTS means direction is 270 deg and speed 19Kts
    // radians = (degrees * 71) / 4068 from Pi/180 to convert to degrees
    Draw_Compass_Rose(); // Draw compass rose
    if (temp_strA == "/////KT") {
      temp_strA = "00000KT";
    }
    temp_strB = temp_strA.substring(3, 5);
    wind_speedKTS = temp_strB.toInt(); // Knots/sec
    int wind_speedMPH = wind_speedKTS * 1.15077 + 0.5;
    if (temp_strA.indexOf('G') >= 0) {
      temp_strB = temp_strA.substring(8);
    }
    else {
      temp_strB = temp_strA.substring(5);
    } // Now get units of wind speed either KT or MPS
    if (temp_strB == "MPS") {
      temp_strB = "MS";
    }
    if (wind_speedMPH < 18) display_item((centreX - 28), (centreY + 70), (String(wind_speedMPH) + " MPH"), YELLOW, 2);
    else                    display_item((centreX - 35), (centreY + 70), (String(wind_speedMPH) + " MPH"), RED, 2);
    if (temp_strA.indexOf('G') >= 0) {
      lcd.fillRect((centreX - 40), (centreY + 68), 82, 18, BLACK);
      display_item((centreX - 40), (centreY + 70), String(wind_speedKTS) + "g" + temp_strA.substring(temp_strA.indexOf('G') + 1, temp_strA.indexOf('G') + 3) + temp_strB, YELLOW, 2);
    }
    int wind_direction = 0;
    if (temp_strA.substring(0, 3) == "VRB") {
      display_item((centreX - 23), (centreY - 7), "VRB", YELLOW, 2);
    }
    else {
      wind_direction = temp_strA.substring(0, 3).toInt() - 90;
      int dx = (diameter * cos((wind_direction) * 0.017444)) + centreX;
      int dy = (diameter * sin((wind_direction) * 0.017444)) + centreY;
      arrow(dx, dy, centreX, centreY, 5, 5, YELLOW); // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
    }

    //----------------------------------------------------------------------------------------------------
    // get next token, this could contain Veering information e.g. 0170V220
    temp_strA = strtok(NULL, " ");
    if (temp_strA.indexOf('V') >= 0 && temp_strA != "CAVOK") { // Check for variable wind direction
      int startV = temp_strA.substring(0, 3).toInt();
      int endV   = temp_strA.substring(4, 7).toInt();
      // Minimum angle is either ABS(AngleA- AngleB) or (360-ABS(AngleA-AngleB))
      int veering = min_val(360 - abs(startV - endV), abs(startV - endV));
      display_item((centreX - 207), (centreY + 65), "V" + String(veering) + char(247), RED, 2);
      display_item((centreX - 60), (centreY + 65), "v", RED, 2); // Signify 'Variable wind direction
      draw_veering_arrow(startV);
      draw_veering_arrow(endV);
      temp_strA = strtok(NULL, " ");    // Move to the next token/item
    }

    //----------------------------------------------------------------------------------------------------
    // Process any reported visibility e.g. 6000 means 6000 Metres of visibility
    if (temp_strA == "////")  {
      temp_strB = "No Visibility Rep.";
    } else temp_strB = "";
    if (temp_strA == "CAVOK") {
      display_item(5, 27, "Visibility good", WHITE, 1);
      display_item(5, 40, "Conditions good", WHITE, 1);
    }
    else {
      if (temp_strA != "////") {
        if (temp_strA == "9999") {
          temp_strB = "Visibility excellent";
        }
        else {
          String vis = temp_strA;
          while (vis.startsWith("0")) {
            vis = vis.substring(1); // trim leading '0'
          }
          temp_strB = vis + " Metres of visibility";
        }
      }
    }
    display_item(5, 27, temp_strB, WHITE, 1);
    temp_strA = strtok(NULL, " ");

    //----------------------------------------------------------------------------------------------------
    // Process any secondary Runway visibility reports e.g. R04/1200 means Runway 4, visibility 1200M
    if ( temp_strA.startsWith("R") && !temp_strA.startsWith("RA")) { // Ignore a RA for Rain weather report
      // Ignore the report for now
      temp_strA = strtok(NULL, " ");    // If there was a Variable report, move to the next item
    }
    if ( temp_strA.startsWith("R") && !temp_strA.startsWith("RA")) { // Ignore a RA for Rain weather report
      // Ignore the report for now, there can be two!
      temp_strA = strtok(NULL, " ");    // If there was a Variable report, move to the next item
    }

    //----------------------------------------------------------------------------------------------------
    // Process any secondary reported visibility e.g. 6000S (or 6000SW) means 6000 Metres in a Southerly direction (or from SW)
    if (temp_strA.length() >= 5 && temp_strA.substring(0, 1) != "+" && temp_strA.substring(0, 1) != "-" && (temp_strA.endsWith("N") || temp_strA.endsWith("S") || temp_strA.endsWith("E") || temp_strA.endsWith("W")) ) {
      conditions_start = temp_strA.substring(4);
      conditions_start = conditions_start.substring(0, 1);
      if (conditions_start == "N" || conditions_start == "S" || conditions_start == "E" || conditions_start == "W") {
        lcd.fillRect(38, 27, 255, 27, BLACK);
        display_item(38, 27, "/" + temp_strA + " Mts Visibility", WHITE, 1);
      }
      temp_strA = strtok(NULL, " ");
    }

    //----------------------------------------------------------------------------------------------------
    // Process any reported weather conditions e.g. -RA means light rain
    // Test for cloud reports that conflict with weather condition descriptors and ignore them, test for occassions when there is no Conditions report
    // Ignore any cloud reports at this stage BKN, CLR, FEW, NCD, OVC, SCT, SKC, or VV///
    lcd.drawRect(199, 172, 279, 16, YELLOW);
    display_item(206, 176, "Additional Wx Reports:", WHITE, 1);
    conditions_start = temp_strA.substring(0, 1);
    temp_strB = temp_strA.substring(0, 3);
    if ((temp_strA == "//" || conditions_test.indexOf(conditions_start) >= 0)
        && !(valid_cloud_report(temp_strB)) // Don't process a cloud report that starts with the same letter as a weather condition report
        && temp_strB != "NSC"
        && !temp_strA.startsWith("M0")) {
      temp_strB = "";
      if (conditions_start == "-" || conditions_start == "+") {
        if (conditions_start == "-") {
          temp_strB = "Light ";
        } else {
          temp_strB = "Heavy ";
        }
        temp_strA = temp_strA.substring(1); // Remove leading + or - and can't modify same variable recursively
      }
      if (temp_strA.length() == 2) {
        display_item(204, 162, temp_strB + display_conditions(temp_strA), WHITE, 1);
      }
      else {
        display_item(204, 216, temp_strB + display_conditions(temp_strA.substring(0, 2)), WHITE, 1);
        display_item_nxy("/" + display_conditions(temp_strA.substring(2, 4)), WHITE, 1);
        if (temp_strA.length() >= 6) { // sometimes up to three cateries are reported
          display_item_nxy("/" + display_conditions(temp_strA.substring(4, 6)), WHITE, 1);
        }
      }
      parameter = strtok(NULL, " ");
      temp_strA = parameter;
    }

    // Process any reported weather conditions e.g. -RA means light rain
    // Test for cloud reports that conflict with weather condition descriptors and ignore them, test for occassions when there is no Conditions report
    // Ignore any cloud reprts at this stage BKN, CLR, FEW, NCD, OVC, SCT, SKC, or VV///(poor vertical visibility)
    conditions_start = temp_strA.substring(0, 1);
    temp_strB = temp_strA.substring(0, 3);
    if ((temp_strA == "//" || conditions_test.indexOf(conditions_start) >= 0)
        && !(valid_cloud_report(temp_strB)) // Don't process a cloud report that starts with the same letter as a weather condition report
        && temp_strB != "NSC"
        && !temp_strA.startsWith("M0")) {
      temp_strB = "";
      if (conditions_start == "-" || conditions_start == "+") {
        if (conditions_start == "-") {
          temp_strB = "Light ";
        } else {
          temp_strB = "Heavy ";
        }
        temp_strA = temp_strA.substring(1); // Remove leading + or - and can't modify same variable recursively
      }
      if (temp_strA.length() == 2) {
        display_item(170, 230, temp_strB + display_conditions(temp_strA), WHITE, 1);
      }
      else {
        if (temp_strA.length() >= 5) { // sometimes up to three cateries are reported
          display_item(170, 230, "Poor Vert Visibility", WHITE, 1);
        }
        else {
          display_item(170, 230, temp_strB + display_conditions(temp_strA.substring(0, 2)), WHITE, 1);
        }
      }
      parameter = strtok(NULL, " ");
      temp_strA = parameter;
    }

    //----------------------------------------------------------------------------------------------------
    // Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
    lcd.drawLine(0, 53, 343, 53, YELLOW);
    if (temp_strA == "////" || temp_strA == "/////" || temp_strA == "//////")  {
      temp_strA = "No CC Rep.";
      temp_strA = strtok(NULL, " ");
    }
    else
    {
      // BLOCK-1 Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
      if (valid_cloud_report(temp_strA) || temp_strA.startsWith("VV/")) {
        temp_strA = convert_clouds(temp_strA);
        display_item(5, 60, temp_strA, WHITE, 1);
        temp_strA = strtok(NULL, " ");
      }

      // BLOCK-2 Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
      if (valid_cloud_report(temp_strA)) {
        temp_strA = convert_clouds(temp_strA);
        display_item(5, 72, temp_strA, WHITE, 1);
        temp_strA = strtok(NULL, " ");
      }

      // BLOCK-3 Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
      if (valid_cloud_report(temp_strA)) {
        temp_strA = convert_clouds(temp_strA);
        display_item(5, 84, temp_strA, WHITE, 1);
        temp_strA = strtok(NULL, " ");
      }

      // BLOCK-4 Process any reported cloud cover e.g. SCT018 means Scattered clouds at 1800 ft
      if (valid_cloud_report(temp_strA)) {
        temp_strA = convert_clouds(temp_strA);
        display_item(5, 96, temp_strA, WHITE, 1);
        temp_strA = strtok(NULL, " ");
      }
    }

    //----------------------------------------------------------------------------------------------------
    // Process any reported temperatures e.g. 14/12 means Temp 14C Dewpoint 12
    // Test first section of temperature/dewpoint which is 'temperature' so either 12/nn or M12/Mnn
    if (temp_strA.indexOf("/") <= 0) {
      temp_Pres = temp_strA;
      temp_strA = "00/00";
    } // Ignore no temp reports and save the next message, which is air pressure for later processing
    String temp_sign = "";
    if (temp_strA.startsWith("M")) {
      temperature = 0 - temp_strA.substring(1, 3).toInt();
      if (temperature == 0) {
        temp_sign = "-"; // Reports of M00, meaning between -0.1 and -0.9C
      }
    }
    else {
      temperature = temp_strA.substring(0, 2).toInt();
      if (temperature >= 0 && temperature < 10) {
        temp_sign = " "; // Aligns for 2-digit temperatures e.g.  " 1'C" and "11'C"
      }
    }
    if (temperature <= 25) {
      display_item(5, 146, " Temp " + temp_sign + String(temperature) + char(247) + "C", CYAN, 2);
    }
    else {
      display_item(5, 146, " Temp " + temp_sign + String(temperature) + char(247) + "C", RED, 2);
    }

    //----------------------------------------------------------------------------------------------------
    // Test second section of temperature/dewpoint which is 'dewpoint' so either nn/04 or Mnn/M04
    temp_strB = temp_strA.substring(temp_strA.indexOf('/') + 1);
    if (temp_strB.startsWith("M")) {
      dew_point = 0 - temp_strB.substring(1, 3).toInt();
    }
    else {
      dew_point = temp_strB.substring(0, 2).toInt();
    }

    //----------------------------------------------------------------------------------------------------
    // Don't display windchill unless wind speed is greater than 3 MPH and temperature is less than 14'C
    if (wind_speedKTS > 3 && temperature <= 14) {
      temp_sign = "";
      int wind_chill = int(calc_windchill(temperature, wind_speedKTS));
      if (wind_chill >= 0 && wind_chill < 10) {
        temp_sign = " "; // Aligns for 2-digit temperatures e.g.  " 1'C" and "11'C"
      }
      display_item(5, 218, "WindC " + temp_sign + String(wind_chill) + char(247) + "C", CYAN, 2);
    }

    //----------------------------------------------------------------------------------------------------
    // Calculate and display Relative Humidity
    temp_sign = "";
    if (dew_point >= 0 && dew_point < 10) {
      temp_sign = " "; // Aligns for 2-digit temperatures e.g.  " 1'C" and "11'C"
    }
    display_item(5, 169, " Dewp " + temp_sign + String(dew_point) + char(247) + "C", CYAN, 2);
    int RH = calc_rh(temperature, dew_point);
    display_item(5, 193, "Rel.H " + String(RH) + "%", CYAN, 2);

    //----------------------------------------------------------------------------------------------------
    // Don't display heatindex unless temperature > 18C
    if (temperature >= 20) {
      float T = (temperature * 9 / 5) + 32;
      float RHx = RH;
      int tHI = (-42.379 + (2.04901523 * T) + (10.14333127 * RHx) - (0.22475541 * T * RHx) - (0.00683783 * T * T) - (0.05481717 * RHx * RHx)  + (0.00122874 * T * T * RHx) + (0.00085282 * T * RHx * RHx) - (0.00000199 * T * T * RHx * RHx) - 32 ) * 5 / 9;
      display_item(5, 218, "HeatX " + String(tHI) + char(247) + "C", CYAN, 2);
    }
    //where   HI = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH - 0.00683783*T*T - 0.05481717*RH*RH + 0.00122874*T*T*RH + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH
    //tHI = heat index (oF)
    //t = air temperature (oF) (t > 57oF)
    //φ = relative humidity (%)
    //----------------------------------------------------------------------------------------------------
    //Process air pressure (QNH) e.g. Q1018 means 1018mB
    temp_strA = strtok(NULL, " ");
    temp_strA.trim();
    if (temp_Pres.startsWith("Q")) temp_strA = temp_Pres; // Pickup a temporary copy of air pressure, found when no temp broadcast
    if (temp_strA.startsWith("Q")) {
      if (temp_strA.substring(1, 2) == "0") {
        temp_strA = " " + temp_strA.substring(2);
      }
      else {
        temp_strA = temp_strA.substring(1);
      }
      display_item((centreX - 40), (centreY - 80), temp_strA + "hPa", YELLOW, 2);
    }

    //----------------------------------------------------------------------------------------------------
    // Now process up to two secondary weather reports
    temp_strA = strtok(NULL, " "); // Get last tokens, can be a secondary weather report
    Process_secondary_reports(temp_strA, 190);
    temp_strA = strtok(NULL, " "); // Get last tokens, can be a secondary weather report
    Process_secondary_reports(temp_strA, 203);
    parameter = NULL; // Reset the 'str' pointer, so end of report decoding
  }
} // finished decoding the METAR string

//----------------------------------------------------------------------------------------------------

String convert_clouds(String source) {
  String height  = source.substring(3, 6);
  String cloud   = source.substring(0, 3);
  String warning = " ";
  while (height.startsWith("0")) {
    height = height.substring(1); // trim leading '0'
  }
  if (source.endsWith("TCU") || source.endsWith("CB")) {
    display_item(5, 108, "Warning - storm clouds detected", WHITE, 1);
    warning = " (storm) ";
  }
  // 'adjust offset if 0 replaced by space
  if (cloud != "SKC" && cloud != "CLR" && height != " ") {
    height = " at " + height + "00ft";
  } else height = "";
  if (source == "VV///") return "No cloud reported";
  if (cloud == "BKN")    return "Broken" + warning + "clouds" + height;
  if (cloud == "SKC")    return "Clear skies";
  if (cloud == "FEW")    return "Few" + warning + "clouds" + height;
  if (cloud == "NCD")    return "No clouds detected";
  if (cloud == "NSC")    return "No signficiant clouds";
  if (cloud == "OVC")    return "Overcast" + warning + height;
  if (cloud == "SCT")    return "Scattered" + warning + "clouds" + height;
  return "";
}


int min_val(int num1, int num2) {
  if (num1 > num2)return num2;
  else            return num1;
}

float calc_rh(int temp, int dewp) {
  return 100 * (exp((17.271 * dewp) / (237.7 + dewp))) / (exp((17.271 * temp) / (237.7 + temp))) + 0.5;
}

float calc_windchill(int temperature, int wind_speed) {
  float result;
  // Derived from wind_chill = 13.12 + 0.6215 * Tair - 11.37 * POWER(wind_speed,0.16)+0.3965 * Tair * POWER(wind_speed,0.16)
  wind_speed = wind_speed * 1.852; // Convert to Kph
  result = 13.12 + 0.6215 * temperature - 11.37 * pow(wind_speed, 0.16) + 0.3965 * temperature * pow(wind_speed, 0.16);
  if (result < 0 ) {
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
  if (temp_strA == "RADZ")  report = "Recent Rain/Drizzle";
  if (temp_strA == "RERA")  report = "Recent Moderate/Heavy Rain";
  if (temp_strA == "REDZ")  report = "Recent Drizzle";
  if (temp_strA == "RESN")  report = "Recent Moderate/Heavy Snow";
  if (temp_strA == "RESG")  report = "Recent Moderate/Heavy Snow grains";
  if (temp_strA == "REGR")  report = "Recent Moderate/Heavy Hail";
  if (temp_strA == "RETS")  report = "No significant change expected";
  display_item(x_pos, line_pos, report, WHITE, 1);
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
  lcd.setCursor(x, y);
  lcd.setTextColor(txt_colour);
  lcd.setTextSize(txt_size);
  lcd.print(token);
  lcd.setTextSize(2); // Back to default text size
}

void display_item_nxy(String token, int txt_colour, int txt_size) {
  lcd.setTextColor(txt_colour);
  lcd.setTextSize(txt_size);
  lcd.print(token);
  lcd.setTextSize(2); // Back to default text size
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
  arrow(centreX, centreY, dx, dy, 2, 5, RED);   // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
}

void Draw_Compass_Rose() {
  int dxo, dyo, dxi, dyi;
  lcd.drawCircle(centreX, centreY, diameter, GREEN); // Draw compass circle
  // lcd.fillCircle(centreX,centreY,diameter,GREY);  // Draw compass circle
  lcd.drawRoundRect((centreX - 67), (centreY - 64), (diameter + 75), (centreY + diameter / 2 + 40), 10, YELLOW); // Draw compass rose
  lcd.drawLine(0, 139, 342, 139, YELLOW);   // Separating line for relative-humidity, temp, windchill, temp-index and dewpoint
  lcd.drawLine(198, 139, 198, 246, YELLOW); // Separating vertical line for relative-humidity, temp, windchill, temp-index and dewpoint
  for (float i = 0; i < 360; i = i + 22.5) {
    dxo = diameter * cos((i - 90) * 3.14 / 180);
    dyo = diameter * sin((i - 90) * 3.14 / 180);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    lcd.drawLine(dxo + centreX, dyo + centreY, dxi + centreX, dyi + centreY, YELLOW); // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
    dxo = dxo * 0.5;
    dyo = dyo * 0.5;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    lcd.drawLine(dxo + centreX, dyo + centreY, dxi + centreX, dyi + centreY, YELLOW); // u8g.drawLine(centreX,centreY,dx,dy); would be the u8g drawing equivalent
  }
  display_item((centreX - 2), (centreY - 33), "N", GREEN, 1);
  display_item((centreX - 2), (centreY + 26), "S", GREEN, 1);
  display_item((centreX + 30), (centreY - 3), "E", GREEN, 1);
  display_item((centreX - 32), (centreY - 3), "W", GREEN, 1);
}

boolean valid_cloud_report(String temp_strA) {
  if (temp_strA.startsWith("BKN") ||
      temp_strA.startsWith("CLR") ||
      temp_strA.startsWith("FEW") ||
      temp_strA.startsWith("NCD") ||
      temp_strA.startsWith("NSC") ||
      temp_strA.startsWith("OVC") ||
      temp_strA.startsWith("SCT") ||
      temp_strA.startsWith("SKC") ||
      temp_strA.endsWith("CB")    ||
      temp_strA.endsWith("TCU")) {
    return true;
  } else {
    return false;
  }
}

void display_status() {
  display_progress("Initialising", 50);
  lcd.setTextSize(2);
  display_progress("Waiting for IP address", 75);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
  }
  display_progress("Ready...", 100);
  clear_screen();
}

void display_progress (String title, int percent) {
  int title_pos = (320 - title.length() * 12) / 2; // Centre title
  int x_pos = 40; int y_pos = 105;
  int bar_width = 250; int bar_height = 15;
  display_item(title_pos * 1.5, (y_pos - 20) * 1.33, title, GREEN, 2);
  lcd.drawRoundRect(x_pos * 1.5, y_pos * 1.33, bar_width + 2, bar_height, 5, YELLOW); // Draw progress bar outline
  lcd.fillRoundRect((x_pos + 2) * 1.5, (y_pos + 1) * 1.33, percent * bar_width / 100 - 2, bar_height - 3, 4, BLUE); // Draw progress
  delay(2000);
  lcd.fillRect((x_pos - 30) * 1.5, (y_pos - 20) * 1.33, 320, 16, BLACK); // Clear titles
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
