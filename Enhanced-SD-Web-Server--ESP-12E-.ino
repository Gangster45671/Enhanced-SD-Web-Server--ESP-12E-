/*
  SDWebServer - Example WebServer with SD Card backend for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Have a FAT Formatted SD Card connected to the SPI port of the ESP8266
  The web root is the SD Card root folder
  File extensions with more than 3 charecters are not supported by the SD Library
  File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter
  index.htm is the default index (works on subfolders as well)

  upload the contents of SdRoot to the root of the SDcard and access the editor by going to http://esp8266sd.local/edit

*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

#define DBG_OUTPUT_PORT Serial
#define JSONBufferSize 800

#define IOT_SERVER                  //comment this out to remove iot server

#define TEMP_LOG                    //comment this out to remove DS18B20 Temperature logging
#ifdef TEMP_LOG
  #define TEMP_LOG_INTERVAL 10           //temperature logging interval in seconds
  #define TEMP_LOG_L "/iot/srv-temp.csv" //temperature log location on SD card
  #define TEMP_LOG_OUT false             //whether to add temperature readings to server log
  //#define CUSTOM_SENS_LOG                 //if this is true, you can add more sensors to log in the digitalClockDisplay() function
  #include <OneWire.h>
  OneWire  ds(2);  // on pin D4 (a 4.7K pullup might be necessary)
  unsigned long previousMillis = 0;
  bool logTime = false;
  byte present = 0;
  //byte type_s = 0;
  byte data[12];
  byte addr[8];
  float celsius;
#endif

#include <TimeLib.h>
#include <WiFiUdp.h>
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
time_t prevDisplay = 0; // when the digital clock was displayed
String lastTimestamp = "NoNTP";
String lastShortTimestamp = "NoNTP";

//config variables
String ssids;               //ap mode ssid
String passwords;           //ap mode password
String sssids;              //station mode ssid
String spasswords;          //station mode pass
String hosts;               //mDNS hostname
bool defaultSTA;            //default wifi mode
bool clearLogonS;           //clear log on boot
String authName;            //admin username
String authPass;            //admin password
String lockedPages[10];     //admin only pages, change this number if you need more than 10
int lArrSize;               //size of locked pages array
String ntpServerName;    //NTP Server  //const static char 
int timeZone;                     //Timezone in GMT+/-

ESP8266WebServer server(80);

static bool hasSD = false;
File uploadFile;
File dataFile;
bool directoryList;
String dataType;
String path;
bool serveMode;

String logfileTemp;

void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
  shortTimeStamp();
  logadd(F("Fail: "), false);
  logadd(msg, true);
  logcommit();
}

bool loadFromSdCard() {
  if (hasSD) {
    directoryList = false;
    dataType = "text/plain";

    if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
    String pathl = path;
    pathl.toLowerCase();
    //text types
    if (pathl.endsWith(".htm")) dataType = "text/html";
    else if (pathl.endsWith(".css")) dataType = "text/css";
    else if (pathl.endsWith(".xml")) dataType = "text/xml";
    else if (pathl.endsWith(".csv")) dataType = "text/csv";
    else if (pathl.endsWith(".js")) dataType = "text/javascript";
    else if (pathl.endsWith(".json") || path.endsWith(".jsn")) dataType = "text/json";
    //image types
    else if (pathl.endsWith(".png")) dataType = "image/png";
    else if (pathl.endsWith(".jpg") || path.endsWith(".jpeg")) dataType = "image/jpg";
    else if (pathl.endsWith(".gif")) dataType = "image/gif";
    else if (pathl.endsWith(".bmp")) dataType = "image/bmp";
    else if (pathl.endsWith(".svg")) dataType = "image/svg+xml";
    else if (pathl.endsWith(".ico")) dataType = "image/x-icon";
    //audio types
    else if (pathl.endsWith(".mid")) dataType = "audio/midi";
    else if (pathl.endsWith(".mp3")) dataType = "audio/mpeg";
    else if (pathl.endsWith(".ogg")) dataType = "audio/ogg";
    else if (pathl.endsWith(".wav")) dataType = "audio/wav";
    //video types
    else if (pathl.endsWith(".mp4")) dataType = "video/mp4";
    //binary data types
    else if (pathl.endsWith(".pdf")) dataType = "application/pdf";
    else if (pathl.endsWith(".zip")) dataType = "application/zip";
    else if (pathl.endsWith(".gz")) {                              //serve compressed versions too!
      //text types
      if (pathl.startsWith("/gz/htm")) dataType = "text/html";
      else if (pathl.startsWith("/gz/css")) dataType = "text/css";
      else if (pathl.startsWith("/gz/xml")) dataType = "text/xml";
      else if (pathl.startsWith("/gz/csv")) dataType = "text/csv";
      else if (pathl.startsWith("/gz/js")) dataType = "text/javascript";
      else if (pathl.startsWith("/gz/jsn")) dataType = "text/json";
      //image types
      else if (pathl.startsWith("/gz/png")) dataType = "image/png";
      else if (pathl.startsWith("/gz/jpg")) dataType = "image/jpg";
      else if (pathl.startsWith("/gz/gif")) dataType = "image/gif";
      else if (pathl.startsWith("/gz/bmp")) dataType = "image/bmp";
      else if (pathl.startsWith("/gz/svg")) dataType = "image/svg+xml";
      else if (pathl.startsWith("/gz/ico")) dataType = "image/x-icon";
      //audio types
      else if (pathl.startsWith("/gz/mid")) dataType = "audio/midi";
      else if (pathl.startsWith("/gz/mp3")) dataType = "audio/mpeg";
      else if (pathl.startsWith("/gz/ogg")) dataType = "audio/ogg";
      else if (pathl.startsWith("/gz/wav")) dataType = "audio/wav";
      //video types
      else if (pathl.startsWith("/gz/mp4")) dataType = "video/mp4";
      //binary data types
      else if (pathl.startsWith("/gz/pdf")) dataType = "application/pdf";
      else dataType = "application/x-gzip";
    }

    dataFile = SD.open(path.c_str());
    if (dataFile.isDirectory()) { //we are serving a directory
      if (!path.endsWith("/")) path += "/";
      if ((SD.exists(path + "index.htm") || SD.exists(path + "INDEX.HTM")) && !(server.argName(0) == "listing" && server.arg(0) == "true")) { //that directory has a index.htm, serve it
        //open index
        String path2 = path;
        if (path2.endsWith("/")) {
          path2 += "index.htm";
        } else {
          path2 += "/index.htm";
        }
        dataType = "text/html";
        dataFile = SD.open(path2.c_str());
      } else {                            //that directory has no index.htm, show a dir listing
        //list directory
        directoryList = true;
        if (!path.endsWith("/")) path += "/";
        serveMode = false;//mode 1 dirlist (false)

      }
    }
    if (!directoryList) {
      if (!dataFile)
        return false;
      serveMode = true;//mode 2 fileserve (true)

    }


    if (path.endsWith("index.htm") || path.endsWith("INDEX.HTM")) {
      path.remove(path.length() - 9);
    }
    return true;
  } else {
    return false;
  }
}

void handleFileUpload() {
  int progress = 0;
  HTTPUpload& upload = server.upload();
  if (server.uri() != "/edit") return;
  if (upload.status == UPLOAD_FILE_START) {
    if (SD.exists((char *)upload.filename.c_str())) SD.remove((char *)upload.filename.c_str());
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: "); DBG_OUTPUT_PORT.println(upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    progress += upload.currentSize;
    DBG_OUTPUT_PORT.print("Upload: WRITE, Bytes: "); DBG_OUTPUT_PORT.print(String(upload.currentSize));
    DBG_OUTPUT_PORT.print(" ("); DBG_OUTPUT_PORT.print(String(upload.totalSize)); DBG_OUTPUT_PORT.println("B)");
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    DBG_OUTPUT_PORT.print("Upload: END, Size: "); DBG_OUTPUT_PORT.println(String(upload.totalSize));
    shortTimeStamp();
    logadd(F("Upload: "), false);
    logadd((char *)upload.filename.c_str(), true);
    logcommit();
  }
}

void deleteRecursive(String path) {
  File file = SD.open((char *)path.c_str());
  if (!file.isDirectory()) {
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) break;
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete() {
  if (server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if (path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
  shortTimeStamp();
  logadd(F("Delete: "), false);
  logadd(path, true);
  logcommit();
}

void handleCreate() {
  if (server.args() == 0) return returnFail("BAD ARGS");
  String path = server.arg(0);
  if (path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if (path.indexOf('.') > 0) {
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if (file) {
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
  shortTimeStamp();
  logadd(F("Create: "), false);
  logadd(path, true);
  logcommit();
}

void printDirectory() {
  if (!server.hasArg("dir")) return returnFail("BAD ARGS");
  String path = server.arg("dir");
  if (path != "/" && !SD.exists((char *)path.c_str())) return returnFail("BAD PATH");
  File dir = SD.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory()) {
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();
  //supa upload speed
  client.setNoDelay(1);
  String output;
  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
      break;


    if (cnt > 0)
      output = ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
  }
  server.sendContent("]");
  dir.close();
  shortTimeStamp();
  logadd(F("Print Directory: ["), false);
  logadd(output, false);
  logadd(F("]"), true);
  logcommit();
}

void handleNotFound() {
  //path processing

  //setup
  dataType = "text/plain";
  path = server.uri();
  if (loadFromSdCard()) {  //process
    //found file

    //do auth
    if (lockedIncludeElement(path)) { //ask for Auth if page in auth list
      if (!server.authenticate(authName.c_str(), authPass.c_str())) {
        return server.requestAuthentication(DIGEST_AUTH, "Admin Zone", "<pre style=\"word-wrap: break-word; white-space: pre-wrap;\">401 Error\nPermission Denied\n\nThis is an Admin zone.</pre>");
      }
    }

    //do serve
    if (serveMode) {
      //doing file serve
      if (server.hasArg("download")) dataType = "application/octet-stream";

      if ((path = "/admin/") && (server.argName(0) == "restart" && server.arg(0) == "true")) {
        shortTimeStamp();
        logadd(F("requested reset from admin page!"), true);
        logcommit();
        server.send(200, "text/plain", "Restarting!");
        delay(200);
        wdt_reset();ESP.restart();while (1) {wdt_reset();}
        //WiFi.forceSleepBegin(); wdt_reset(); ESP.restart(); while (1)wdt_reset();
      }
      if ((path = "/admin/") && (server.argName(0) == "clearLog" && server.arg(0) == "true")) {
        clearLog();
      }

      if (server.streamFile(dataFile, dataType) != dataFile.size()) {
        shortTimeStamp();
        logadd(F("Load: Sent less data than expected!"), true);
      }

      dataFile.close();

      shortTimeStamp();
      logadd(F("Load: "), false);
      logadd((lockedIncludeElement(path)) ? "Auth " : "", false);
      logadd(F("200 "), false);
      logadd(dataType, false);
      logadd(F(" "), false);
      logadd(path, false);
      logadd(F(" "), false);
      if (server.args() >= 1) {
        logadd(F(" Arguments: "), false);
        logadd(String(server.args()), false);
        logadd(F(" -> ["), false);
        for (uint8_t i = 0; i < (server.args() - 1); i++) {
          logadd("{ NAME:" + server.argName(i) + ", VALUE:" + server.arg(i) + "}, ", false);
        }
        logadd("{ NAME:" + server.argName(server.args() - 1) + ", VALUE:" + server.arg(server.args() - 1) + "}", false);
        logadd(F("]"), true);
      } else {
        logadd(F(""), true);
      }
      logcommit();
    } else {
      //doing dir list
      server.send(200, "text/html", printDirectoryHTML(dataFile, path.c_str()));
      dataFile.close();

      shortTimeStamp();
      logadd(F("Load DirList: "), false);
      logadd((lockedIncludeElement(path)) ? "Auth " : "", false);
      logadd(F("200 "), false);
      logadd(path, false);
      logadd(F(" "), false);
      if (server.args() >= 1) {
        logadd(F(" Arguments: "), false);
        logadd(String(server.args()), false);
        logadd(F(" -> ["), false);
        for (uint8_t i = 0; i < (server.args() - 1); i++) {
          logadd("{ NAME:" + server.argName(i) + ", VALUE:" + server.arg(i) + "}, ", false);
        }
        logadd("{ NAME:" + server.argName(server.args() - 1) + ", VALUE:" + server.arg(server.args() - 1) + "}", false);
        logadd(F("]"), true);
      } else {
        logadd(F(""), true);
      }
      logcommit();
    }

    return;
  }

  //didn't find file
  String message = "404 Error\n";
  message += (!hasSD) ? "SDCARD Not Detected\n\n" : "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  shortTimeStamp();
  logadd(F("Load: 404 "), false);
  logadd(server.uri(), false);
  logadd(F(" Method: "), false);
  logadd((server.method() == HTTP_GET) ? "GET" : "POST", false);
  logadd(F(" Arguments: "), false);
  logadd(String(server.args()), false);
  logadd(F(" -> ["), false);
  for (uint8_t i = 0; i < (server.args() - 1); i++) {
    logadd("{ NAME:" + server.argName(i) + ", VALUE:" + server.arg(i) + "}, ", false);
  }
  if (server.args() >= 1) {
    logadd("{ NAME:" + server.argName(server.args()) + ", VALUE:" + server.arg(server.args()) + "}", false);
  }
  logadd(F("]"), true);
  logcommit();
}

void setup(void) {
  //start Serial communication
  DBG_OUTPUT_PORT.begin(115200);
  //DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");

  //start the SD Card
  if (SD.begin(SS)) {
    shortTimeStamp();
    logadd(F("SD Card initialized."), true);
    hasSD = true;
  } else {
    logadd(F("FATAL ERROR: SD Card failed."), true);
    logadd(F(""), true);
    logcommit();
    while (1) delay(500);
  }

  logadd(F("Server Boooting..."), true);
  //load config.jsn
  if (!loadConfig()) {
    logadd(F("FATAL ERROR: Load or Parse Config.jsn failed."), true);
    logadd(F(""), true);
    logcommit();
    while (1) delay(500);
  } else {
    logadd(F("Config file read and parsed."), true);
  }

  if (clearLogonS) {
    clearLog();
    logadd(F("Server Boooting..."), true);
    logadd(F("SD Card initialized."), true);
    logadd(F("Config file read and parsed."), true);
    logcommit();
  }

  //connect to wifi
  if (!defaultSTA) {
    //Wifi AP mode
    ApMode();
  } else {
    //WiFi Station mode
    WiFi.mode(WIFI_STA);
    WiFi.begin(sssids.c_str(), spasswords.c_str());
    logadd(F("Connecting to "), false);
    logadd(sssids, false);
    logadd(F("@"), false);
    logadd(spasswords, true);
    // Wait for connection
    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED && i++ < 40) {//wait 10 seconds
      delay(500);
      DBG_OUTPUT_PORT.print(".");
    }
    DBG_OUTPUT_PORT.println(".");
    if (i == 41) {
      logadd(F("Starting AP, Could not connect to "), false);
      logadd(sssids, true);
      ApMode();
      return;
    }
    logadd(F("Connected! IP address: "), false);
    logadd(WiFi.localIP().toString(), true);
  }

  //setup DNS
  if (MDNS.begin((const char *)hosts.c_str())) {
    MDNS.addService("http", "tcp", 80);
    logadd(F("MDNS responder started"), true);
    logadd(F("You can now connect to http://"), false);
    logadd(hosts, false);
    logadd(F(".local"), true);
  }

  //configure and begin server
  setUpServer();

  //begin UDP/NTP client
  logadd(F("Starting UDP"), true);
  Udp.begin(localPort);
  //logadd(F("Local port: "), false);
  //logadd(String(Udp.localPort()), true);
  logadd(F("waiting for sync"), true);
  setSyncProvider(getNtpTime);
  setSyncInterval(1800);                     //renew connection every 30min
  logcommit();
  
  //log everthing from startup
  logadd(F(""), true);
  logcommit();
}

void loop(void) {
  server.handleClient();

  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }

  #ifdef TEMP_LOG
    if (logTime) {
      unsigned long currentMillis = millis();
      if(currentMillis - previousMillis >= 1000) {  //conversion time
        previousMillis = currentMillis;   
        logTemp(finishGetTemp());
      }
    }
  #endif
}

//low-level universal functions

//log functions
void logadd(String msg, bool newline) {
  msg = String(msg);
  logfileTemp += msg;
  if (newline) {
    logfileTemp += "\n";
    DBG_OUTPUT_PORT.println(msg);
  } else {
    DBG_OUTPUT_PORT.print(msg);
  }
}
void logcommit() {
  File logfile = SD.open("server.log", FILE_WRITE);
  if (logfile) {
    logfile.print(logfileTemp);
    logfile.close();
    //DBG_OUTPUT_PORT.println("Log: saved");
    logfileTemp = "";
  } else {
    DBG_OUTPUT_PORT.println("Log: file does not exist!");
    while (1) delay(500);
  }
}
void clearLog() {
  SD.remove("server.log");
  logadd(F("Log: cleared log"), true);
  logcommit();
}

//sd listing functions
String printDirectoryHTML(File dir, String path) {
  if (!path.endsWith("/")) path += "/";
  String message = "<h2>Directory Listing: ";
  message += path;
  message += "</h2><table><tr><td><i>Filename</i></td><td><i>File Size</i></td></tr>";
  if (!(path == "/")) {
    message += "<tr><td><a href='../'>../</a></td></tr>";
  }
  message += "<title>Directory Listing: ";
  message += path;
  message += "</title>";
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) { //no more files
      break;
    }
    if (entry.isDirectory()) {
      message += "<tr><td><b><a href='";
      message += path;
      //message += "/";
      message += entry.name();
      message += "/'>";
      message += entry.name();
      message += "</a></b></td></tr>";
    } else {
      // files have sizes, directories do not
      message += "<tr><td><a href='";
      message += path;
      //message += "/";
      message += entry.name();
      message += "'>";
      message += entry.name();
      message += "</a></td><td><p>";
      message += entry.size();
      message += "B</p></td></tr>";
    }
    entry.close();
  }
  message += "</table><style>td {padding-top:0px;}</style>";
  return message;
}

//configuration file
bool loadConfig() {
  StaticJsonBuffer<JSONBufferSize> jsonBuffer;
  //DynamicJsonBuffer jsonBuffer(600);
  String json;
  File configFile = SD.open("config.jsn");
  if (configFile) {
    while (configFile.available()) {
      char c = configFile.read();
      json.concat(c);
    }
    configFile.close();
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) return false;

    ssids = (const char *)root["wifi"]["ap"]["ssid"];
    passwords = (const char *)root["wifi"]["ap"]["pass"];
    hosts = (const char *)root["wifi"]["host"];

    sssids = (const char *)root["wifi"]["station"]["ssid"];
    spasswords = (const char *)root["wifi"]["station"]["pass"];

    defaultSTA = root["wifi"]["defaultsta"];
    clearLogonS = root["clearLog"];

    authName = (const char *)root["auth"]["user"]["name"];
    authPass = (const char *)root["auth"]["user"]["pass"];
    for (int i = 0; i < root["auth"]["locked"].size(); i++) {
      String pthVal = root["auth"]["locked"][i];
      //Serial.println(pthVal);
      lockedPages[i] = pthVal;
    }
    lArrSize = root["auth"]["locked"].size();

    //ntp config
    ntpServerName = (const char *)root["ntp"]["server"];
    timeZone = root["ntp"]["timezone"];

    //log configurations
    shortTimeStamp();
    logadd("", true);
    logadd(F("WiFI Options:"), true);
    logadd(F("\t Access Point:"), true);
    logadd(F("\t\t ssid: "), false);
    logadd(ssids, true);
    logadd(F("\t\t pass: "), false);
    logadd(passwords, true);
    logadd(F("\t Station:"), true);
    logadd(F("\t\t ssid: "), false);
    logadd(sssids, true);
    logadd(F("\t\t pass: "), false);
    logadd(spasswords, true);
    logadd(F("\t DefaultSTA: "), false);
    logadd((defaultSTA) ? "YES" : "NO", true);
    logadd(F("\t hostname: "), false);
    logadd(hosts, true);
    logadd(F("Auth Options:"), true);
    logadd(F("\t User:"), true);
    logadd(F("\t\t name: "), false);
    logadd(authName, true);
    logadd(F("\t\t pass: "), false);
    logadd(authPass, true);
    logadd(F("\t Locked Pages:"), true);
    for (int i = 0; i < lArrSize; i++) {
      logadd(F("\t\t "), false);
      logadd(lockedPages[i], true);
    }

    logadd(F("NTP:"), true);
    logadd(F("\t Server: "), false);
    logadd(ntpServerName, true);
    logadd(F("\t Timezone: GMT"), false);
    logadd(String(timeZone), true);
    
    logadd(F("ClearLog: "), false);
    logadd((clearLogonS) ? "YES" : "NO", true);
    logadd(F(""), true);
    logcommit();
    return true;
  } else {
    return false;
  }
}

//switch wifi modes
void ApMode() {
  logadd(F("Stopping HTTP server..."), true);
  server.stop();
  WiFi.mode(WIFI_AP_STA);
  if (passwords.length() < 8 || passwords.length() > 63) {
    passwords = "";
    //password = NULL;
    logadd(F("Invalid WiFi AP Password, ignoring..."), true);
    WiFi.softAP(ssids.c_str());
  } else {
    WiFi.softAP(ssids.c_str(), passwords.c_str());
  }
  delay(500);
  logadd(F("Switched to AP mode"), true);
  //WiFi.begin();
  MDNS.notifyAPChange();
  setUpServer();
  IPAddress myIP = WiFi.softAPIP();
  logadd(F("Hosting to "), false);
  logadd(ssids, false);
  logadd(F("@"), false);
  logadd(passwords, true);
  logadd(F("AP IP address: "), false);
  logadd(myIP.toString(), true);
}

//start HTTP server
void setUpServer() {
  //setup web pages
  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/edit", HTTP_POST, []() {
    returnOK();
  }, handleFileUpload);
  server.onNotFound(handleNotFound);

#ifdef IOT_SERVER
  server.on("/iot", HTTP_GET, iotServer);
  server.on("/iot/", HTTP_GET, iotServer);
#endif
#ifdef TEMP_LOG
  server.on("/temp", HTTP_GET, []() {
    server.send(200, "text/plain", String(celsius)+"C");
   });
   server.on("/temp/", HTTP_GET, []() {
    server.send(200, "text/plain", String(celsius)+"C");
   });
#endif
  server.on("/time", HTTP_GET, []() {
    server.send(200, "text/plain", lastTimestamp);
  });
  server.on("/time/", HTTP_GET, []() {
    server.send(200, "text/plain", lastTimestamp);
  });

  //Update //moved to sd card
  /* server.on("/update/", HTTP_GET, []() {
    server.send(200, "text/html", updateIndex);
    }); */
  server.on("/update/", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      //Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      logadd(F("Update: %s\n"), false);
      logadd(upload.filename.c_str(), true);
      logcommit();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
        logadd(F("Update failed"), true);
        logcommit();
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
        logadd(F("Update failed"), true);
        logcommit();
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        logadd(F("Update Success: %u\nRebooting...\n"), false);
        logadd(String(upload.totalSize), true);
        logcommit();
      } else {
        Update.printError(Serial);
        logadd(F("Update failed"), true);
        logcommit();
      }
      //Serial.setDebugOutput(false);
    }
    yield();
  });

  //begin server
  server.begin();
  logadd(F("HTTP server started"), true);
}

//check if on locked page
boolean lockedIncludeElement(String element) {
  for (int i = 0; i < lArrSize; i++) {
    if (lockedPages[i] == element) {
      return true;
    }
  }
  return false;
}

//iot server
#ifdef IOT_SERVER
void iotServer() {
  if (server.argName(0) == "name" && server.argName(1) == "value") {
    String timestamp = lastTimestamp;
    String lvalue = server.arg(1);
    String lname = server.arg(0);
    
    String dataString = "";
    dataString += timestamp;
    dataString += ", ";
    dataString += lvalue;

    File dataFile = SD.open("/iot/" + lname + ".csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      shortTimeStamp();
      logadd(F("IOT Server: ("), false);
      logadd(lname, false);
      logadd(F(") "), false);
      logadd(dataString, true);
      server.send(200, "text/plain", "SUCCESS");
    }
    // if the file isn't open, pop up an error:
    else {
      shortTimeStamp();
      logadd(F("IOT Server: Failed to open "), false);
      logadd(lname, false);
      logadd(F(".csv"), true);
    }
    logcommit();
  } else {
    returnFail("BAD PATH");
    return;
  }
}
#endif


bool firstT = true;
#ifdef TEMP_LOG
  int logCycle = 0;
#endif
void digitalClockDisplay()
{
  // digital clock display of the time
  lastTimestamp = "";
  printDigits(hour());
  lastTimestamp += ":";
  printDigits(minute());
  lastTimestamp += ":";
  printDigits(second());
  lastTimestamp += " ";
  printDigits(day());
  lastTimestamp += "/";
  printDigits(month());
  lastTimestamp += "/";
  lastTimestamp += year();
  if (firstT) logadd(lastTimestamp, true); logcommit(); firstT = false;

  lastShortTimestamp = "";
  printDigitsS(hour());
  lastShortTimestamp += ":";
  printDigitsS(minute());
  lastShortTimestamp += ":";
  printDigitsS(second());

  #ifdef TEMP_LOG
    if (logCycle >= (TEMP_LOG_INTERVAL-1)) { //it takes one second to convert temperatures!
      startGetTemp();
      #ifdef CUSTOM_SENS_LOG
        customLog();
      #endif
      logCycle = 0;
    } else {
      logCycle++;
    }
  #endif
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    lastTimestamp += '0';
  lastTimestamp += digits;
}

void printDigitsS(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    lastShortTimestamp += '0';
  lastShortTimestamp += digits;
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  logadd(F("Transmiting NTP Request.."), true);
  // get a random server from the pool
  WiFi.hostByName(ntpServerName.c_str(), ntpServerIP);
  logadd(ntpServerName, false);
  logadd(F(": "), false);
  logadd(ntpServerIP.toString(), true);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      logadd(F("Received NTP Response, synced."), true);
      firstT = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      logcommit();
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  shortTimeStamp();
  logadd(F("No NTP Response :-("), true);
  logcommit();
  //while (1) {};
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void shortTimeStamp() {
  logadd(F("["), false);
  logadd(lastShortTimestamp, false);
  logadd(F("] "), false);
}

#ifdef TEMP_LOG
//temp logging
void logTemp(float temp) {
  String dataString = "";
    dataString += lastTimestamp;
    dataString += ", ";
    dataString += String(temp);

    File dataFile = SD.open(TEMP_LOG_L, FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      if (TEMP_LOG_OUT) {
        shortTimeStamp();
        logadd(F("Temp Log: "), false);
        logadd(dataString, false);
        logadd(F("˚C"), true);
      }
    }
    // if the file isn't open, pop up an error:
    else {
      shortTimeStamp();
      logadd(F("Temp Log: Failed to open "), false);
      logadd(TEMP_LOG_L, true);
    }
    logcommit();
}

void startGetTemp() {
  restartT:
  if ( !ds.search(addr)) {
    ds.reset_search();
    goto restartT;
    //delay(250);
    //return;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  //delay(1000);     // maybe 750ms is enough, maybe not   //must be async!
  logTime = true;
}

float finishGetTemp() {

  // we might do a ds.depower() here, but the reset will take care of it.
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  
  for (byte i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  logTime = false;

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time
  celsius = (float)raw / 16.0;
  return celsius;
}

#ifdef CUSTOM_SENS_LOG 
  void customLog() {
    //write your code to read your sensors and log to the SD Card here.
    //IMPORTANT! MUST BE ASYNCHONOUS! THis means that you cannot have delay() or any other function that hangs up the code
    //if this does not execute quicly it will break the HTTP server.
  }
#endif
#endif
