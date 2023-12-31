// OTA Implementation
#include "UpdateOTA.h"

UpdateOTA::UpdateOTA (int sketchSize, int sketchSpace, String sketchMD5) {
	// Sketch Info
	_sketchSize = sketchSize;
	_sketchSpace = sketchSpace;
	_sketchMD5 = sketchMD5;
}

void UpdateOTA::init() {
  #if defined(NO_OTA)
      bool otaEnabled = false;
  #else
      bool otaEnabled = true;
  #endif

  #if defined(OTA_PASSWORD)
      char otaPassword[] = OTA_PASSWORD;
  #else
      char otaPassword[] = "";
  #endif
    // Set up OTA
    if (otaEnabled) {
        // Start OTA once connected
        Serial.println("Setting up OTA");
        // Port defaults to 3232
        // ArduinoOTA.setPort(3232);
        // Hostname defaults to esp3232-[MAC]
        ArduinoOTA.setHostname(mdnsName);
        // No authentication by default
        if (strlen(otaPassword) != 0) {
            ArduinoOTA.setPassword(otaPassword);
            Serial.printf("OTA Password: %s\n\r", otaPassword);
        } else {
            Serial.printf("\r\nNo OTA password has been set! (insecure)\r\n\r\n");
        }
        ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                    type = "filesystem";
                Serial.println("Start updating " + type);
                // Stop the camera since OTA will crash the module if it is running.
                // the unit will need rebooting to restart it, either by OTA on success, or manually by the user
                Serial.println("Stopping Camera");
                esp_err_t err = esp_camera_deinit();
                critERR = "<h1>OTA Has been started</h1><hr><p>Camera has Halted!</p>";
                critERR += "<p>Wait for OTA to finish and reboot, or <a href=\"control?var=reboot&val=0\" title=\"Reboot Now (may interrupt OTA)\">reboot manually</a> to recover</p>";
            })
            .onEnd([]() {
                Serial.println("\r\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });
        ArduinoOTA.begin();
    } 
	
    // Gather static values used when dumping status; these are slow functions, so just do them once during startup
    _sketchSize = ESP.getSketchSize();
    _sketchSpace = ESP.getFreeSketchSpace();
    _sketchMD5 = ESP.getSketchMD5();
}