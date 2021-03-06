# Enhanced SD Web Server (ESP-12E) (in development)
An Enhanced SD Card Web Server running on an ESP-12E..

- [Enhanced SD Web Server (ESP-12E)](#enhanced-sd-web-server-esp-12e)
    - [Planned Features (And Progress)](#planned-features-and-progress)
    - [About](#about)
        - [Content Type Support](#content-type-support)
            - [Supported MIME Types](#supported-mime-types)
            - [Compressed Pages](#compressed-pages)
        - [Connecting/Running the Web Server](#connectingrunning-the-web-server)
            - [AP Mode](#ap-mode)
            - [STA Mode](#sta-mode)
        - [SD File System Interface](#sd-file-system-interface)
            - [Create Files/Directories](#create-filesdirectories)
            - [Upload Files](#upload-files)
            - [Edit Files/Directories](#edit-filesdirectories)
            - [Delete Files/Directories](#delete-filesdirectories)
            - [Download Files/Directories](#download-filesdirectories)
        - [Generated Pages](#generated-pages)
            - [Index Retrieval](#index-retrieval)
            - [Directory Retrieval](#directory-retrieval)
            - [404 Page](#404-page)
            - [Request Log](#request-log)
        - [Per-Page Authentication](#per-page-authentication)
        - [Web Update](#web-update)
        - [IoT Server](#iot-server)
        - [Server Side Scripting](#server-side-scripting)
            - [Server Side Includes](#server-side-includes)
            - [Server Side Scripts](#server-side-scripts)
        - [*Web Sockets?*](#web-sockets)
        - [*FTP Support?*](#ftp-support)
        - [Configuration Page](#configuration-page)
        - [Recommended SD Card File Structure](#recommended-sd-card-file-structure)
    - [Useful Links and References](#useful-links-and-references)

## Planned Features (And Progress)
- [ ] Server Basis
    - [X] WiFi AP or Station (Automatic Switch)
    - [X] mDNS Hostname
    - [ ] Asynchronous Requests
- [X] Expand Server
    - [X] Interface
        - [X] Web SD File System Interface
            - [X] Create Files/Directories
            - [X] Upload Files/Directories
            - [X] Edit Files/Directories
            - [X] Delete Files/Directories
            - [X] Download Files/Directories
        - [X] Generated Pages
            - [X] 404 Page
            - [X] Index Retrieval
            - [X] Directory Retreival
            - [X] Request Logging
    - [X] Software
        - [X] Config. Page
        - [X] Many Content Types
            - [X] Serve Compressed(.gz) files
        - [X] Per-Page Authentication Support
- [ ] Finalize Server
    - [X] IoT API Server
    - [X] Web Update
    - [ ] Server Side Scripting (CGI)
        - [ ] Run scripts
        - [ ] Server Side Includes (SSI)
        - [ ] Dynamic Web Pages
    - *Web Sockets?*
    - *FTP Support?*

## About
For Async look at TickerBasic


Lights:
    Activity/Serial
    Systems online
    Request in Progress/busy
    Wifi event notification



### Content Type Support
#### Supported MIME Types
#### Compressed Pages

### Connecting/Running the Web Server
Looking at the log sent through the serial monitor is the easiest way of figuring out what is going on.
#### AP Mode
If the server is configured('/config.jsn' or '/admin/') to always start on Access Point mode it will start on this mode right away. Otherwise it will automatically switch to this mode if it fails to connect in station mode. In AP mode the server creates it's own wifi netwok, with configurable ssid and password. This mode is only intended to be used to set up and configure the server as NTP and any other online functions won't work.
#### STA Mode
This is the default mode, the server will connect to an access point and you will be able to access it at it's custom address, set it the admin page or 'config.jsn'. It will look something like this: http://hostname.local

### SD File System Interface
This is the heart of the server, it let's you browse the SD card directory and edit files.
#### Create Files/Directories
By typing the filename and pressing the MkDir and MkFlie buttons you can make new files and directories.
#### Upload Files
By clicking on the choose file button you can upload files to the webserver's SD Card.
#### Edit Files/Directories
If the selected file is a text file it can be edited directly on the webpage with the editor. There is even syntax highlighting for code files. If it is an image a preview will be shown.
#### Delete Files/Directories
By right clicking over an item you can open a context menu, that allows you to delete it.
#### Download Files/Directories
By right clicking over an item you can open a context menu, that allows you to download it.

### Generated Pages
#### Index Retrieval
If you happen to browse into a directory and that directory has a 'index.htm' file, that will be served instead of the directory listing.
#### Directory Retrieval
By navigating to any directory with no 'index.htm' file, you will be shown a directory listing page.
If you want to see the directory listing of a page that contains a 'index.htm' file you can simply pass the argument '?listing=true' with the URL.
#### 404 Page
If you navigate to a page that does not exist the server will show you a 404 error page. This page contains the URI, the method used to get the page and any arguments passed.
#### Request Log
All server activity is loged to the 'server.log' file, located at the root of the SD card.

### Per-Page Authentication
Via the configution page at '/admin/' or by editing '/config.jsn' you can set a list of pages/directories that require an admin login to be accessed. If a directory is set then so will it's 'index.htm'. Admin username and password are also settable via config.

### Web Update
By going to '/update/' you can upload a '.bin' file directly into the flash of your ESP. Allowing for firmware updates wirelessly/remotely.

### IoT Server
You can add a sensor value to it's own log by making a GET request to the server at '/iot/?name=feed_name&value=value'. The log is stored at '/iot/feed_name.csv' and a timestamp is automatically added with every log.

### Server Side Scripting
#### Server Side Includes
#### Server Side Scripts

### *Web Sockets?*
### *FTP Support?*
[Video](https://www.youtube.com/watch?v=SnCIYrGF4s8)

### Configuration Page
At this page you can configure the main settings for the webserver, these settings are stored on the SD card. Making this server SD hotswapable!
Consider this the control panel.

### Recommended SD Card File Structure


## Useful Links and References
- Similliar Projects
    - [SD Web Server Example](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/examples/SDWebServer/SDWebServer.ino) (Basis For This Project)
        - [Video](https://www.youtube.com/watch?v=zJP3Ie3nE7c)
    - [ESP-HTTPD](http://www.esp8266.com/viewtopic.php?f=6&t=376) (Lua ESP Web Server)
    - [SSI Server](https://hackaday.io/project/28626-budget-wi-fi-nest-box-camera/log/71813-esp8266-sd-card-webserver-with-server-side-scripting) (**Incomplete**)
- Web Server Info
    - [Jibble Web Server](http://www.jibble.org/jibblewebserver.php) (Java Web Server)
- ESP Info
    - Misc
        - [Using an SD Card (Hardware)](http://doityourselfchristmas.com/forums/showthread.php?43345-Using-SD-cards-with-ESP8266-s)
        - [Using SPIFFS](http://www.esp8266.com/viewtopic.php?f=29&t=8194)
        - [Encoding/Decoding JSON](https://randomnerdtutorials.com/decoding-and-encoding-json-with-arduino-or-esp8266/)
    - Pinouts
        - [ESP-12 Pinout](https://esp8266.github.io/Arduino/versions/2.0.0/doc/esp12.png)
        - [ESP-01 Pinout](https://os.mbed.com/media/uploads/sschocke/esp8266-pinout_etch_copper_top.png)
    - Programming
        - [ESP-01](https://os.mbed.com/users/sschocke/code/WiFiLamp/wiki/Updating-ESP8266-Firmware)
    - General Guides
        - [Sparkfun Guide](https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/using-the-arduino-addon)
        - [Beginner's Guide](https://github.com/tttapa/ESP8266)
            - [Introduction](https://tttapa.github.io/ESP8266/Chap01%20-%20ESP8266.html)
            - [Hardware](https://tttapa.github.io/ESP8266/Chap02%20-%20Hardware.html)
            - [Software](https://tttapa.github.io/ESP8266/Chap03%20-%20Software.html)
            - [Microcontroller](https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html)
            - [Network Protocols](https://tttapa.github.io/ESP8266/Chap05%20-%20Network%20Protocols.html)
            - [Uploading](https://tttapa.github.io/ESP8266/Chap06%20-%20Uploading.html)
            - [Wi-Fi connections](https://tttapa.github.io/ESP8266/Chap07%20-%20Wi-Fi%20Connections.html)
            - [mDNS](https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html)
            - [Web Servers](https://tttapa.github.io/ESP8266/Chap09%20-%20Web%20Server.html)
            - [Simple Web Server](https://tttapa.github.io/ESP8266/Chap10%20-%20Simple%20Web%20Server.html)
            - [SPIFFS](https://tttapa.github.io/ESP8266/Chap11%20-%20SPIFFS.html)
            - [Uploading to the Server](https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html)
            - [OTA](https://tttapa.github.io/ESP8266/Chap13%20-%20OTA.html)
            - [WebSocket](https://tttapa.github.io/ESP8266/Chap14%20-%20WebSocket.html)
            - [NTP](https://tttapa.github.io/ESP8266/Chap15%20-%20NTP.html)
            - [Data Logging](https://tttapa.github.io/ESP8266/Chap16%20-%20Data%20Logging.html)
            - [Email Notifier](https://tttapa.github.io/ESP8266/Chap17%20-%20Email%20Notifier.html)
            - [Advanced](https://tttapa.github.io/ESP8266/Chap18%20-%20Advanced.html)
            - [Conclusion](https://tttapa.github.io/ESP8266/Chap19%20-%20In%20Conclusion.html)
- GitHub Markdown Info
    - [Formatting and Syntax](https://help.github.com/articles/basic-writing-and-formatting-syntax/)
    - [Emoji Cheat Sheet](https://www.webpagefx.com/tools/emoji-cheat-sheet/)