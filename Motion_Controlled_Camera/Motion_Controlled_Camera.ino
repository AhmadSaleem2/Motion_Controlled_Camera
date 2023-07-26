#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>
#include <ETH.h>
static const uint8_t MotionSensor_INPUT_PIN = 0;
static const uint8_t Camera_OUTPUT_PIN   = 2;
#define ETH_PHY_POWER 5
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
static bool eth_connected = false;
int previoudMotionSensorState; 
HTTPClient http;

const char* host = "esp32";
const char* ssid = "WRLaccess";
const char * hostName = "esp-async";
const char* password =  "flycam01";
String apiKey = "c29928bf-4283-46d4-8a5c-a21b79eac40a";

IPAddress local_IP(192, 168, 10, 11);
//Set your Gateway IP address
IPAddress gateway(192, 168, 10, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

const char* OTAuserName = "WhatsRunning";
const char* OTApassword =  "123qweE#";

AsyncWebServer server(80);

/* setup function */
void setup(void) {

  Serial.begin(115200);
  initEthernet();
  initWiFI();

  pinMode(Camera_OUTPUT_PIN, OUTPUT);
  pinMode(MotionSensor_INPUT_PIN, INPUT);
  resetTodefaultConfig();

  AsyncElegantOTA.begin(&server, OTAuserName, OTApassword);

  server.on("/startRecording", HTTP_GET, [](AsyncWebServerRequest *request) {startRecordingAPI(request);}); 
  server.on("/stopRecording", HTTP_GET, [](AsyncWebServerRequest *request) {stopRecordingAPI(request);}); 
  server.on("/resetToDefault",  HTTP_GET, [](AsyncWebServerRequest *request) {authorize(request, &resetToDefaultAPI);});
  server.begin();
  Serial.println("HTTP server started");
}

void initEthernet(){
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_LAN8720, ETH_CLK_MODE);
  ETH.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
}

void WiFiEvent(WiFiEvent_t event)//Ethernet config
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}


void initWiFI(){
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);  //Connect to the WiFi network
  int i=0;
  while (WiFi.status() != WL_CONNECTED || (i>10 && eth_connected)) {  //Wait for connection
    delay(500);
    i++;
    Serial.println("Waiting to connect...");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //Print the local IP
}


void loop(void) {
  CheckStateChange();
  //Serial.println("Loop!!!");
  //delay(200);
}

void authorize (AsyncWebServerRequest *request, void (*func)(AsyncWebServerRequest *request)){
  if(request->hasHeader("api-key") and String(request->header("api-key").c_str())==apiKey)
  {
    func(request);
  }
  else {
    request->send(401, "text/plain", "Api-Key incorect or not provided!");
  }
}

void startRecordingAPI(AsyncWebServerRequest *request){
  Serial.println("startRecordingAPI!!!!");
  //digitalWrite(Camera_OUTPUT_PIN, HIGH);
  MotionDetected();
  request->send(200, "text/plain", "Recording started!");
}

void stopRecordingAPI(AsyncWebServerRequest *request){
  Serial.println("stopRecordingAPI");
  digitalWrite(Camera_OUTPUT_PIN, LOW);
  request->send(200, "text/plain", "Recording stoped!");
}

void resetToDefaultAPI(AsyncWebServerRequest *request) {
  //add code to reset to default
  resetTodefaultConfig();
  request->send(200, "text/plain", "Reset to default");
}

void resetTodefaultConfig()
{
  digitalWrite(Camera_OUTPUT_PIN, LOW);
  previoudMotionSensorState = LOW;
}

void CheckStateChange(){
  int currentState = digitalRead(MotionSensor_INPUT_PIN);
  //Serial.println("Loop!!!");
  if (previoudMotionSensorState == LOW and currentState == HIGH){
    MotionDetected();
    previoudMotionSensorState = HIGH;
  }
  if (previoudMotionSensorState == HIGH and currentState == LOW){
    MotionStopped();
    previoudMotionSensorState = LOW;
  }
}

void MotionDetected()
{
  String path = "http://192.168.10.4/orbit.php?cmd=CO";
  digitalWrite(Camera_OUTPUT_PIN, HIGH);
  //MakeGetCall(path);
  //xTaskCreate(MakeGetCall, "Task1", 1000, NULL, 0, &TaskHandle_1);

}

void MotionStopped()
{
  String path = "http://192.168.10.4/orbit.php?cmd=OFF&id=192.168.10.24&sid=&uid=3D1DA3AF&ulen=4&date=3682/02/16&time=16:14:33&md5=559E7A739B8D885878DE14D84A13A439&mac=04:91:62:FA:F3:7F";
  digitalWrite(Camera_OUTPUT_PIN, LOW);
  //MakeGetCall(path);
}

void MakeGetCall(String path){
  Serial.println(WiFi.status());
  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      Serial.println("Reached here");
      // Your Domain name with URL path or IP address with path
      http.begin(path.c_str());
      
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      
      // Send HTTP GET request
      //vTaskDelay(2);
      int httpResponseCode = http.GET();
      Serial.println(httpResponseCode);
      //vTaskDelay(2);
      //delay(100);
      if (httpResponseCode>0) {
        Serial.println("Reached here");
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        http.end();
     //   return payload;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        http.end();
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
   // return "{}";
     //vTaskDelete(TaskHandle_1);
}
