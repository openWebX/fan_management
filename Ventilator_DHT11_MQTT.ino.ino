

/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  Error examples by Todd Treece for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


/****************************************************************************
  CONFIG HERE!
****************************************************************************/

#define THING_NAME      "Fan Buero"
#define THING_LOCATION  "buero"

/************************** Web Server ************************************/

#define WEBSERVER_PORT  80

/*************************** IR Stuff *************************************/

#define IR_LED 4
IRsend irsend(IR_LED); 

/*************************** DHT Sensor ***********************************/

#define DHTPIN D1     
#define DHTTYPE DHT11   

/************************* WiFi Access Point ******************************/

#define WLAN_SSID       "closedWeb"
#define WLAN_PASS       "isjMu6Uaumau7"

/************************* MQTT Setup *************************************/

#define AIO_SERVER      "192.168.1.100"
#define AIO_SERVERPORT  1883                   
#define AIO_USERNAME    ""
#define AIO_KEY         ""

#define MQTT_FAN_ACTION  "myhome/fan/" THING_LOCATION "/action"
#define MQTT_FAN_STATE   "myhome/fan/" THING_LOCATION "/state"
#define MQTT_TEMP_STATE  "myhome/fan/" THING_LOCATION "/temperature"
#define MQTT_HUM_STATE   "myhome/fan/" THING_LOCATION "/humidity"

/***************************************************************************/




/*
 * Initialize storage variables
 * 
 */
float curr_temp = 0;
float curr_humi = 0;
char *curr_lamp;
char *curr_fan;


DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;


WiFiClient client;

MDNSResponder mdns;
ESP8266WebServer server(WEBSERVER_PORT);




Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish fan_state = Adafruit_MQTT_Publish(&mqtt, MQTT_FAN_STATE);
Adafruit_MQTT_Publish fan_temp = Adafruit_MQTT_Publish(&mqtt, MQTT_TEMP_STATE);
Adafruit_MQTT_Publish fan_humi = Adafruit_MQTT_Publish(&mqtt, MQTT_HUM_STATE);
Adafruit_MQTT_Publish fan_trigger = Adafruit_MQTT_Publish(&mqtt, MQTT_FAN_ACTION);
Adafruit_MQTT_Subscribe fan_action = Adafruit_MQTT_Subscribe(&mqtt, MQTT_FAN_ACTION);





/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void handleRoot() {
  server.send(200, "text/html",
              "<html>" \
                "<head><title>Fan management</title><style>*, html, body {font-size:11px;font-family: arial, verdana, helvetica, sans-serif;} h1 {font-size:18px;background:gray;color:white;padding:3px;} p {margin-bottom:10px;}p a {background:green;color:white;padding:5px;text-decoration:none;}</style></head>" \
                "<body>" \
                  "<h1>" \
                  THING_NAME \
                  "</h1>" \
                  "<h2>Hello, you can send " \
                      "signals from here!</h2>" \
                  "<p><a href=\"ir?lamp_on=1\">Turn lamp on</a></p>" \
                  "<p><a href=\"ir?lamp_off=1\">Turn lamp off</a></p>" \
                  "<p><a href=\"ir?fan_off=1\">Turn fan off</a></p>" \
                  "<p><a href=\"ir?fan_low=1\">Turn fan low</a></p>" \
                  "<p><a href=\"ir?fan_medium=1\">Turn fan medium</a></p>" \
                  "<p><a href=\"ir?fan_high=1\">Turn fan high</a></p>" \
                "</body>" \
              "</html>");
}

void handleIr() {
  if (server.argName(0) != "") {
    char s[server.argName(0).length() + 1];
    server.argName(0).toCharArray(s, server.argName(0).length() + 1);
    if (! fan_trigger.publish(s)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }    
  }
  handleRoot();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}


void actioncallback(char *data, uint16_t len) {
  String state = "{\n";
  
  Serial.print("Hey we're in a action callback, the value is: ");
  Serial.println(data);
  if (strcmp(data, "lamp_on") == 0) {
     Serial.println("Turning lamp on!");
     uint16_t rawData[191] = {1208, 478,  1200, 476,  338, 1338,  394, 1290,  340, 1340,  392, 1286,  342, 1338,  340, 1342,  340, 1338,  392, 1288,  342, 1342,  340, 7156,  1178, 522,  1178, 476,  336, 1340,  340, 1344,  338, 1342,  1206, 478,  1202, 476,  1202, 476,  1202, 502,  1178, 476,  1202, 478,  1202, 6586,  1224, 472,  1202, 476,  336, 1340,  340, 1336,  342, 1344,  336, 1370,  364, 1288,  340, 1340,  1210, 474,  334, 1344,  338, 1340,  338, 7156,  1208, 470,  1200, 502,  364, 1288,  342, 1338,  392, 1288,  340, 1342,  340, 1340,  340, 1340,  1202, 478,  336, 1342,  342, 1338,  340, 7152,  1208, 472,  1204, 474,  338, 1338,  340, 1344,  338, 1340,  340, 1340,  340, 1342,  340, 1340,  1206, 500,  366, 1286,  342, 1342,  340, 7150,  1206, 476,  1202, 500,  364, 1288,  340, 1340,  340, 1342,  340, 1340,  340, 1340,  340, 1342,  1206, 474,  336, 1340,  340, 1338,  340, 7156,  1206, 472,  1198, 502,  366, 1290,  338, 1340,  340, 1336,  396, 1318,  314, 1338,  342, 1340,  1202, 480,  336, 1342,  340, 1338,  340, 7474,  1178, 496,  1202, 500,  314, 1338,  340, 1344,  338, 1340,  342, 1338,  340, 1340,  342, 1342,  1204, 474,  336, 1342,  338, 1342,  342};
     irsend.sendRaw(rawData, 191, 38);
     curr_lamp = "on";
     delay(2000);
  }
  if (strcmp(data, "lamp_off") == 0) {
     Serial.println("Turning lamp off!");
     uint16_t rawData[191] = {1172, 506,  1248, 428,  394, 1282,  396, 1290,  392, 1286,  396, 1306,  370, 1288,  396, 1284,  396, 1286,  394, 1288,  394, 1308,  370, 7106,  1170, 502,  1252, 424,  394, 1312,  396, 1282,  396, 1262,  1256, 424,  1254, 450,  1230, 424,  1254, 424,  1254, 424,  1282, 398,  1256, 6540,  1256, 458,  1228, 426,  394, 1286,  396, 1288,  396, 1284,  396, 1286,  1254, 422,  396, 1290,  420, 1282,  370, 1284,  396, 1310,  370, 7098,  1196, 506,  1230, 426,  394, 1286,  392, 1286,  394, 1288,  394, 1286,  1252, 426,  394, 1284,  420, 1262,  396, 1282,  396, 1284,  396, 7100,  1196, 480,  1254, 424,  394, 1310,  370, 1286,  396, 1284,  396, 1290,  1264, 414,  394, 1306,  372, 1308,  370, 1290,  394, 1286,  394, 7098,  1194, 506,  1228, 450,  370, 1308,  396, 1286,  398, 1258,  394, 1288,  1254, 422,  394, 1290,  420, 1280,  372, 1308,  370, 1288,  396, 7098,  1196, 484,  1250, 450,  370, 1286,  394, 1308,  398, 1284,  370, 1288,  1250, 424,  424, 1282,  370, 1312,  396, 1260,  396, 1282,  396, 7424,  1228, 440,  1252, 424,  392, 1288,  396, 1284,  392, 1310,  370, 1290,  1252, 426,  394, 1284,  396, 1286,  394, 1286,  396, 1284,  394};
     irsend.sendRaw(rawData, 191, 38);
     curr_lamp = "off";
     delay(2000);
  }
  if (strcmp(data, "fan_off") == 0) {
     Serial.println("Turning fan_off!");
     uint16_t rawData[191] = {1204, 472,  1198, 478,  400, 1282,  398, 1280,  394, 1288,  398, 1282,  396, 1284,  396, 1286,  396, 1284,  394, 1286,  394, 1288,  390, 7446,  1198, 484,  1198, 480,  364, 1314,  364, 1314,  366, 1320,  1198, 480,  1198, 482,  1198, 482,  1198, 480,  1198, 480,  1200, 480,  1198, 6630,  1228, 458,  1198, 478,  366, 1314,  368, 1314,  366, 1314,  362, 1316,  362, 1322,  1196, 480,  336, 1342,  338, 1344,  362, 1320,  336, 7496,  1224, 462,  1196, 480,  336, 1346,  338, 1342,  338, 1342,  338, 1346,  338, 1346,  1196, 480,  336, 1342,  362, 1322,  364, 1314,  338, 7494,  1200, 488,  1200, 478,  364, 1316,  366, 1314,  364, 1318,  364, 1316,  338, 1344,  1198, 478,  366, 1318,  362, 1318,  366, 1314,  366, 7470,  1226, 458,  1200, 478,  366, 1312,  368, 1316,  366, 1314,  368, 1312,  366, 1318,  1200, 478,  366, 1312,  368, 1310,  368, 1314,  394, 7438,  1258, 426,  1228, 450,  366, 1316,  392, 1286,  368, 1312,  368, 1314,  396, 1286,  1200, 476,  394, 1286,  396, 1286,  394, 1286,  368, 7448,  1232, 440,  1198, 478,  394, 1286,  394, 1288,  370, 1310,  394, 1286,  392, 1292,  1228, 450,  394, 1286,  394, 1284,  382, 1300,  394};
     irsend.sendRaw(rawData, 191, 38);
     curr_fan = "off";
     delay(2000);
  }
  if (strcmp(data, "fan_high") == 0) {
     Serial.println("Turning fan high!");
     uint16_t rawData[191] = {1256, 418,  1196, 480,  416, 1262,  420, 1264,  418, 1264,  416, 1262,  418, 1290,  394, 1260,  418, 1264,  416, 1266,  416, 1290,  394, 7402,  1316, 380,  1246, 430,  418, 1266,  416, 1264,  418, 1264,  1198, 506,  1172, 482,  1198, 482,  1196, 482,  1196, 482,  1196, 482,  1196, 6618,  1282, 390,  1196, 478,  418, 1268,  416, 1260,  446, 1236,  418, 1266,  416, 1262,  418, 1264,  418, 1264,  418, 1262,  418, 1266,  1244, 6568,  1284, 390,  1196, 480,  416, 1264,  418, 1262,  418, 1260,  418, 1264,  420, 1286,  394, 1262,  416, 1264,  420, 1262,  416, 1264,  1198, 6616,  1258, 438,  1172, 482,  414, 1268,  416, 1262,  416, 1288,  392, 1266,  416, 1262,  418, 1262,  418, 1264,  420, 1262,  416, 1264,  1198, 6618,  1256, 416,  1196, 480,  416, 1290,  392, 1264,  416, 1262,  416, 1266,  442, 1262,  394, 1260,  418, 1266,  418, 1260,  420, 1264,  1198, 6616,  1258, 438,  1172, 504,  394, 1262,  420, 1260,  418, 1262,  420, 1264,  418, 1262,  418, 1286,  394, 1264,  416, 1266,  414, 1264,  1198, 6616,  1254, 442,  1172, 480,  418, 1262,  416, 1290,  392, 1260,  420, 1262,  418, 1264,  444, 1236,  416, 1262,  418, 1266,  416, 1290,  1172};  // UNKNOWN 34CFD84F
     irsend.sendRaw(rawData, 191, 38);
     curr_fan = "high";
     delay(2000);
  }
  if (strcmp(data, "fan_medium") == 0) {
     Serial.println("Turning fan medium!");
     uint16_t rawData[191] = {1268, 404,  1282, 394,  454, 1224,  460, 1220,  458, 1220,  464, 1218,  462, 1218,  462, 1216,  462, 1220,  456, 1222,  460, 1218,  434, 7390,  1322, 380,  1278, 398,  426, 1252,  458, 1218,  462, 1224,  1280, 402,  1276, 402,  1276, 402,  1276, 404,  1276, 402,  1250, 430,  1280, 6528,  1264, 410,  1280, 398,  458, 1216,  428, 1256,  428, 1250,  456, 1222,  462, 1218,  432, 1248,  462, 1220,  1282, 400,  430, 1246,  460, 7358,  1294, 380,  1250, 428,  430, 1246,  460, 1218,  462, 1220,  458, 1222,  462, 1218,  458, 1224,  458, 1222,  1280, 400,  428, 1248,  458, 7360,  1296, 378,  1278, 402,  454, 1220,  458, 1220,  460, 1222,  462, 1220,  458, 1220,  460, 1222,  462, 1220,  1280, 402,  454, 1220,  460, 7358,  1268, 408,  1274, 402,  428, 1246,  436, 1246,  460, 1222,  460, 1218,  462, 1220,  460, 1224,  462, 1216,  1282, 400,  428, 1248,  434, 7384,  1294, 380,  1278, 400,  458, 1218,  462, 1218,  462, 1222,  462, 1218,  462, 1218,  434, 1250,  462, 1220,  1254, 428,  428, 1246,  462, 7358,  1270, 404,  1284, 420,  428, 1224,  460, 1218,  462, 1218,  460, 1222,  458, 1220,  432, 1248,  460, 1222,  1280, 400,  426, 1248,  462};  // UNKNOWN DDE60E3C
     irsend.sendRaw(rawData, 191, 38);
     curr_fan = "medium";
     delay(2000);
  }
  if (strcmp(data, "fan_low") == 0) {
     Serial.println("Turning fan low!");
     uint16_t rawData[191] = {1260, 442,  1248, 428,  396, 1282,  420, 1260,  420, 1260,  396, 1284,  422, 1258,  394, 1288,  420, 1260,  394, 1284,  396, 1286,  420, 7402,  1256, 444,  1246, 430,  394, 1286,  392, 1288,  398, 1284,  1250, 454,  1198, 458,  1248, 430,  1250, 430,  1248, 430,  1248, 454,  1224, 6564,  1232, 440,  1222, 456,  394, 1284,  396, 1284,  396, 1288,  1222, 454,  396, 1284,  394, 1286,  422, 1256,  420, 1262,  1254, 428,  1252, 6560,  1170, 498,  1256, 424,  394, 1290,  392, 1286,  394, 1312,  1230, 426,  392, 1288,  394, 1284,  396, 1286,  420, 1264,  1248, 430,  1222, 6590,  1230, 464,  1200, 452,  420, 1260,  394, 1286,  394, 1288,  1250, 428,  394, 1286,  394, 1284,  396, 1286,  424, 1260,  1250, 428,  1250, 6564,  1230, 440,  1248, 428,  394, 1284,  422, 1282,  372, 1290,  1222, 456,  392, 1284,  396, 1286,  422, 1258,  396, 1288,  1252, 426,  1254, 6560,  1170, 502,  1252, 424,  394, 1286,  396, 1284,  396, 1288,  1252, 424,  394, 1288,  394, 1284,  396, 1284,  420, 1262,  1224, 458,  1248, 6564,  1232, 438,  1254, 422,  396, 1288,  420, 1258,  396, 1286,  1254, 424,  420, 1262,  394, 1284,  396, 1284,  396, 1288,  1222, 456,  1248};
     irsend.sendRaw(rawData, 191, 38);
     curr_fan = "low";
     delay(2000);
  }

  state += "\"lamp\":\"";
  state += curr_lamp;
  state += "\",\n";
  state += "\"fan\":\"";
  state += curr_fan;
  state += "\",\n";
  state += "\"temp\":";
  state += curr_temp;
  state += ",\n";
  state += "\"humidity\":";
  state += curr_humi;
  state += "\n}";

  char statechar[100];
  state.toCharArray(statechar, 100);

  Serial.println(state);
  if (! fan_state.publish(statechar)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

}

void setup() {

  
  
  Serial.begin(115200);
  delay(10);

  Serial.println("openWeb Fan/MQTT/HTTP Service");
  
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/ir", handleIr);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  fan_action.setCallback(actioncallback);
  mqtt.subscribe(&fan_action);
  irsend.begin();

  dht.begin();

  
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");

}



void loop() {
  
  // Connect to MQTT Server
  MQTT_connect();

  // Wait for messages
  mqtt.processPackets(1000);

  // No ping? Ris- and reconnect next run...
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  // handle Webserver stuff
  server.handleClient();

  // check and update temp and humidity
  checkTemperature();
  checkHumidity();
}



// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}


void checkTemperature() {
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    if (curr_temp != event.temperature) {
      Serial.print(F("\nSending temperature val "));
      if (! fan_temp.publish(event.temperature)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      curr_temp = event.temperature;
      Serial.print("Current temp: ");
      Serial.print(curr_temp);
      Serial.println(" °C");
    }
    
  }
}

void checkHumidity() {
  // Get humidity event and print its value.
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    if (curr_humi != event.relative_humidity) {
      Serial.print(F("\nSending humidity val "));
      if (! fan_humi.publish(event.relative_humidity)) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
      curr_humi = event.relative_humidity;
      Serial.print("Current humidity: ");
      Serial.print(curr_humi);
      Serial.println(" %");
    }
    
  }
}

