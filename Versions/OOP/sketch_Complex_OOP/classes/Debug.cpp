
#include "Debug.h"

#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include "time.h"
// Upstream version string
#include "src/version.h"

extern void serialDump();

Debug::Debug() {

}

Debug::init() {

}

// Counters for info screens and debug
int8_t streamCount = 0;          // Number of currently active streams
unsigned long streamsServed = 0; // Total completed streams
unsigned long imagesServed = 0;  // Total image requests
// This will be displayed to identify the firmware
char myVer[] PROGMEM = __DATE__ " @ " __TIME__;
// Critical error string; if set during init (camera hardware failure) it will be returned for all http requests
String critERR = "";
// Debug flag for stream and capture data
bool debugData;


void Debug::debugOn() {
    debugData = true;
    Serial.println("Camera debug data is enabled (send 'd' for status dump, or any other char to disable debug)");
}


void Debug::debugOff() {
    debugData = false;
    Serial.println("Camera debug data is disabled (send 'd' for status dump, or any other char to enable debug)");
}


// Serial input (debugging controls)
void Debug::handleSerial() {
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'd' ) {
            serialDump();
        } else {
            if (debugData) debugOff();
            else debugOn();
        }
    }
    while (Serial.available()) Serial.read();  // chomp the buffer
}


void Debug::printLocalTime(bool extraData=false) {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
    } else {
        Serial.println(&timeinfo, "%H:%M:%S, %A, %B %d %Y");
    }
    if (extraData) {
        Serial.printf("NTP Server: %s, GMT Offset: %li(s), DST Offset: %i(s)\r\n", ntpServer, gmtOffset_sec, daylightOffset_sec);
    }
}

void Debug::setupDebug() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println("====");
    Serial.print("esp32-cam-webserver: ");
    Serial.println(myName);
    Serial.print("Code Built: ");
    Serial.println(myVer);
    Serial.print("Base Release: ");
    Serial.println(baseVersion);
    Serial.println();

    if (critERR.length() == 0) {
        Serial.printf("\r\nCamera Ready!\r\nUse '%s' to connect\r\n", httpURL);
        Serial.printf("Stream viewer available at '%sview'\r\n", streamURL);
        Serial.printf("Raw stream URL is '%s'\r\n", streamURL);
        #if defined(DEBUG_DEFAULT_ON)
            debugOn();
        #else
            debugOff();
        #endif
    } else {
        Serial.printf("\r\nCamera unavailable due to initialisation errors.\r\n\r\n");
    }

    // Info line; use for Info messages; eg 'This is a Beta!' warnings, etc. as necesscary
    // Serial.print("\r\nThis is the 4.1 beta\r\n");

    // As a final init step chomp out the serial buffer in case we have recieved mis-keys or garbage during startup
    while (Serial.available()) Serial.read();
}