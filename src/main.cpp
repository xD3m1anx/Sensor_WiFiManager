#include <FS.h>                   //this needs to be first, or it all crashes and burns...
    #include <ArduinoOTA.h>
    #include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
    #define BLYNK_PRINT Serial

    #include <BlynkSimpleEsp8266.h>

    //needed for library
    #include <DNSServer.h>
    #include <ESP8266WebServer.h>
    #include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

    #include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


    /* --------------------------------------------------------------------------------------------------------------- */

#ifdef BUILTIN_LED_ON
  #define BLINK_BUILTIN(__delay__)    ledBuiltinBlink(__delay__)
#else
  #define BLINK_BUILTIN(any)     ;
  #pragma "Builtin led is off. To led on define 'BUILTIN_LED_ON'"  
#endif

/* --------------------------------------------------------------------------------------------------------------- */


    //define your default values here, if there are different values in config.json, they are overwritten.
    char blynk_token[34] = "TNuB9RM6XeT3LmvlZZh_IdM-vcDJY2ln";

    //flag for saving data
    bool shouldSaveConfig = false;

    //callback notifying us of the need to save config
    void saveConfigCallback () {
      Serial.println("Should save config");
      shouldSaveConfig = true;
    }


    void setup() {
      // put your setup code here, to run once:
      Serial.begin(9600);
      Serial.println();

      ArduinoOTA.setHostname("SensWifiManager");
    ArduinoOTA.onStart([]() {
          //OTA_PRINT.println("Start updating ");
          BLINK_BUILTIN(0);
        }
    );
    ArduinoOTA.onEnd([]() {
          //OTA_PRINT.println("\nEnd");          
        }
    );
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        //OTA_PRINT.printf("Progress: %u%%\r", (progress / (total / 100)));
        BLINK_BUILTIN(0);
        }
    );
    ArduinoOTA.onError([](ota_error_t error) {
          //OTA_PRINT.printf("Error[%u]: ", error);
          if (error == OTA_AUTH_ERROR) {
            //OTA_PRINT.println("Auth Failed");
          } else if (error == OTA_BEGIN_ERROR) {
            //OTA_PRINT.println("Begin Failed");
          } else if (error == OTA_CONNECT_ERROR) {
            //OTA_PRINT.println("Connect Failed");
          } else if (error == OTA_RECEIVE_ERROR) {
            //OTA_PRINT.println("Receive Failed");
          } else if (error == OTA_END_ERROR) {
            //OTA_PRINT.println("End Failed");
          }
        }
    );
    ArduinoOTA.begin();
    //Debug.println("done.");
    BLINK_BUILTIN(1000);

      //clean FS, for testing
      //SPIFFS.format();

      //read configuration from FS json
      Serial.println("mounting FS...");

      if (SPIFFS.begin()) {
        FSInfo fsInfo;
        SPIFFS.info(fsInfo);

        Serial.println("SPIFFS:");
        Serial.printf("totalBytes: %d\n", fsInfo.totalBytes);
        Serial.printf("usedBytes: %d\n", fsInfo.usedBytes);
        Serial.printf("blockSize: %d\n", fsInfo.blockSize);
        Serial.printf("pageSize: %d\n", fsInfo.pageSize);
        Serial.printf("maxOpenFiles: %d\n", fsInfo.maxOpenFiles);
        Serial.printf("maxPathLength: %d\n", fsInfo.maxPathLength);

        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
          //file exists, reading and loading
          Serial.println("reading config file");
          File configFile = SPIFFS.open("/config.json", "r");
          if (configFile) {
            Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject(buf.get());
            json.printTo(Serial);
            if (json.success()) {
              Serial.println("\nparsed json");
    
              //strcpy(mqtt_server, json["mqtt_server"]);
              //strcpy(mqtt_port, json["mqtt_port"]);
              strcpy(blynk_token, json["blynk_token"]);
    
            } else {
              Serial.println("failed to load json config");
            }
          }
        }
        else {
          Serial.println("File not exists");
        }
      } else {
        Serial.println("failed to mount FS");
      }
      //end read
    
    
    
      // The extra parameters to be configured (can be either global or just in the setup)
      // After connecting, parameter.getValue() will get you the configured value
      // id/name placeholder/prompt default length
      //WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
      //WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
      WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 34);

      //WiFiManager
      //Local intialization. Once its business is done, there is no need to keep it around
      WiFiManager wifiManager;

      //set config save notify callback
      wifiManager.setSaveConfigCallback(saveConfigCallback);

      //set static ip
      //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
      //add all your parameters here
      //wifiManager.addParameter(&custom_mqtt_server);
      //wifiManager.addParameter(&custom_mqtt_port);
      wifiManager.addParameter(&custom_blynk_token);

      //reset settings - for testing
      //wifiManager.resetSettings();

      //set minimu quality of signal so it ignores AP's under that quality
      //defaults to 8%
      //wifiManager.setMinimumSignalQuality();
  
      //sets timeout until configuration portal gets turned off
      //useful to make it all retry or go to sleep
      //in seconds
      //wifiManager.setTimeout(120);

      //fetches ssid and pass and tries to connect
      //if it does not connect it starts an access point with the specified name
      //here  "AutoConnectAP"
      //and goes into a blocking loop awaiting configuration
      wifiManager.autoConnect("GarageDoor");
      Serial.println(custom_blynk_token.getValue());
      if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
      }

      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");

      //read updated parameters
      //strcpy(mqtt_server, custom_mqtt_server.getValue());
      //strcpy(mqtt_port, custom_mqtt_port.getValue());
      strcpy(blynk_token, custom_blynk_token.getValue());
    
      //save the custom parameters to FS
      if (shouldSaveConfig) {
        Serial.println("saving config");
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        //json["mqtt_server"] = mqtt_server;
        //json["mqtt_port"] = mqtt_port;
        json["blynk_token"] = blynk_token;

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
          Serial.println("failed to open config file for writing");
        }
        else {
          Serial.println("Allowed to open config file for writing");
        }

        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
        //end save
      }

      Serial.println("local ip");
      Serial.println(WiFi.localIP());
      Blynk.config(blynk_token,"192.168.0.50", 8080);
      bool result = Blynk.connect(180);

      if (!result) {
        Serial.println("BLYNK Connection Fail");
        wifiManager.resetSettings();
        ESP.reset();
        delay (5000);
      }
      else {
        Serial.println("BLYNK Connected");
      }

    }

    void loop() {
      // put your main code here, to run repeatedly:
      Blynk.run();
      ArduinoOTA.handle();
    }