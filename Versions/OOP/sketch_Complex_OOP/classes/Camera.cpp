
#include "Camera.h"
#include <esp_camera.h>

void Camera::Camera() {
  
}

void Camera::init() {
  /* Pin definitions for some common ESP-CAM modules
  *   Select the module to use in myconfig.h
  *   Defaults to AI-THINKER CAM module
  */
  //#include "camera_pins.h"
  #if defined(CAMERA_MODEL_AI_THINKER)
    // AI Thinker
    // https://github.com/SeeedDocument/forum_doc/raw/master/reg/ESP32_CAM_V1.6.pdf
    #define PWDN_GPIO_NUM     32
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM      0
    #define SIOD_GPIO_NUM     26
    #define SIOC_GPIO_NUM     27
    #define Y9_GPIO_NUM       35
    #define Y8_GPIO_NUM       34
    #define Y7_GPIO_NUM       39
    #define Y6_GPIO_NUM       36
    #define Y5_GPIO_NUM       21
    #define Y4_GPIO_NUM       19
    #define Y3_GPIO_NUM       18
    #define Y2_GPIO_NUM        5
    #define VSYNC_GPIO_NUM    25
    #define HREF_GPIO_NUM     23
    #define PCLK_GPIO_NUM     22
    #define LED_PIN           33 // Status led
    #define LED_ON           LOW // - Pin is inverted.
    #define LED_OFF         HIGH //
    #define LAMP_PIN           4 // LED FloodLamp.
  #else
    #error "Camera model not selected, did you forget to uncomment it in myconfig?"
  #endif


  // Camera config structure
  camera_config_t config;

  // Names for the Camera. (set these in config.h)
  #if defined(CAM_NAME)
      char myName[] = CAM_NAME;
  #else
      char myName[] = "HIT camera server";
  #endif

  // This will be set to the sensors PID (identifier) during initialisation
  //camera_pid_t sensorPID;
  int sensorPID;

  // Camera module bus communications frequency.
  // Originally: config.xclk_freq_mhz = 20000000, but this lead to visual artifacts on many modules.
  // See https://github.com/espressif/esp32-camera/issues/150#issuecomment-726473652 et al.
  #if !defined (XCLK_FREQ_MHZ)
      unsigned long xclk = 8;
  #else
      unsigned long xclk = XCLK_FREQ_MHZ;
  #endif

  // initial rotation
  // can be set in myconfig.h
  #if !defined(CAM_ROTATION)
      #define CAM_ROTATION 0
  #endif
  int myRotation = CAM_ROTATION;

  // minimal frame duration in ms, effectively 1/maxFPS
  #if !defined(MIN_FRAME_TIME)
      #define MIN_FRAME_TIME 0
  #endif
  int minFrameTime = MIN_FRAME_TIME;

}

Camera::void StartCamera() {
    // Populate camera config structure with hardware and other defaults
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = xclk * 1000000;
    config.pixel_format = PIXFORMAT_JPEG;
    // Low(ish) default framesize and quality
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;

    #if defined(CAMERA_MODEL_ESP_EYE)
        pinMode(13, INPUT_PULLUP);
        pinMode(14, INPUT_PULLUP);
    #endif

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        delay(100);  // need a delay here or the next serial o/p gets missed
        Serial.printf("\r\n\r\nCRITICAL FAILURE: Camera sensor failed to initialise.\r\n\r\n");
        Serial.printf("A full (hard, power off/on) reboot will probably be needed to recover from this.\r\n");
        Serial.printf("Meanwhile; this unit will reboot in 1 minute since these errors sometime clear automatically\r\n");
        // Reset the I2C bus.. may help when rebooting.
        periph_module_disable(PERIPH_I2C0_MODULE); // try to shut I2C down properly in case that is the problem
        periph_module_disable(PERIPH_I2C1_MODULE);
        periph_module_reset(PERIPH_I2C0_MODULE);
        periph_module_reset(PERIPH_I2C1_MODULE);
        // And set the error text for the UI
        critERR = "<h1>Error!</h1><hr><p>Camera module failed to initialise!</p><p>Please reset (power off/on) the camera.</p>";
        critERR += "<p>We will continue to reboot once per minute since this error sometimes clears automatically.</p>";
        // Start a 60 second watchdog timer
        esp_task_wdt_init(60,true);
        esp_task_wdt_add(NULL);
    } else {
        Serial.println("Camera init succeeded");

        // Get a reference to the sensor
        sensor_t * s = esp_camera_sensor_get();

        // Dump camera module, warn for unsupported modules.
        sensorPID = s->id.PID;
        switch (sensorPID) {
         //   case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
          //  case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
            case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
           // case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
            default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
        }

        // OV3660 initial sensors are flipped vertically and colors are a bit saturated
        // if (sensorPID == OV3660_PID) {
        //     s->set_vflip(s, 1);  //flip it back
        //     s->set_brightness(s, 1);  //up the blightness just a bit
        //     s->set_saturation(s, -2);  //lower the saturation
        // }

        // M5 Stack Wide has special needs
        // #if defined(CAMERA_MODEL_M5STACK_WIDE)
        //     s->set_vflip(s, 1);
        //     s->set_hmirror(s, 1);
        // #endif

        // Config can override mirror and flip
        #if defined(H_MIRROR)
            s->set_hmirror(s, H_MIRROR);
        #endif
        #if defined(V_FLIP)
            s->set_vflip(s, V_FLIP);
        #endif

        // set initial frame rate
        #if defined(DEFAULT_RESOLUTION)
            s->set_framesize(s, DEFAULT_RESOLUTION);
        #endif

        /*
        * Add any other defaults you want to apply at startup here:
        * uncomment the line and set the value as desired (see the comments)
        *
        * these are defined in the esp headers here:
        * https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h#L149
        */

        //s->set_framesize(s, FRAMESIZE_SVGA); // FRAMESIZE_[QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)]);
        //s->set_quality(s, val);       // 10 to 63
        //s->set_brightness(s, 0);      // -2 to 2
        //s->set_contrast(s, 0);        // -2 to 2
        //s->set_saturation(s, 0);      // -2 to 2
        //s->set_special_effect(s, 0);  // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
        //s->set_whitebal(s, 1);        // aka 'awb' in the UI; 0 = disable , 1 = enable
        //s->set_awb_gain(s, 1);        // 0 = disable , 1 = enable
        //s->set_wb_mode(s, 0);         // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        //s->set_exposure_ctrl(s, 1);   // 0 = disable , 1 = enable
        //s->set_aec2(s, 0);            // 0 = disable , 1 = enable
        //s->set_ae_level(s, 0);        // -2 to 2
        //s->set_aec_value(s, 300);     // 0 to 1200
        //s->set_gain_ctrl(s, 1);       // 0 = disable , 1 = enable
        //s->set_agc_gain(s, 0);        // 0 to 30
        //s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        //s->set_bpc(s, 0);             // 0 = disable , 1 = enable
        //s->set_wpc(s, 1);             // 0 = disable , 1 = enable
        //s->set_raw_gma(s, 1);         // 0 = disable , 1 = enable
        //s->set_lenc(s, 1);            // 0 = disable , 1 = enable
        //s->set_hmirror(s, 0);         // 0 = disable , 1 = enable
        //s->set_vflip(s, 0);           // 0 = disable , 1 = enable
        //s->set_dcw(s, 1);             // 0 = disable , 1 = enable
        //s->set_colorbar(s, 0);        // 0 = disable , 1 = enable
    }
    // We now have camera with default init
}

void Camera::setupCamera() {

    // Start (init) the camera 
    StartCamera();

}
