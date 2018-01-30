/*
  .______   .______    __    __   __    __          ___      __    __  .___________.  ______   .___  ___.      ___   .___________. __    ______   .__   __.
  |   _  \  |   _  \  |  |  |  | |  |  |  |        /   \    |  |  |  | |           | /  __  \  |   \/   |     /   \  |           ||  |  /  __  \  |  \ |  |
  |  |_)  | |  |_)  | |  |  |  | |  |__|  |       /  ^  \   |  |  |  | `---|  |----`|  |  |  | |  \  /  |    /  ^  \ `---|  |----`|  | |  |  |  | |   \|  |
  |   _  <  |      /  |  |  |  | |   __   |      /  /_\  \  |  |  |  |     |  |     |  |  |  | |  |\/|  |   /  /_\  \    |  |     |  | |  |  |  | |  . `  |
  |  |_)  | |  |\  \-.|  `--'  | |  |  |  |     /  _____  \ |  `--'  |     |  |     |  `--'  | |  |  |  |  /  _____  \   |  |     |  | |  `--'  | |  |\   |
  |______/  | _| `.__| \______/  |__|  |__|    /__/     \__\ \______/      |__|      \______/  |__|  |__| /__/     \__\  |__|     |__|  \______/  |__| \__|

  Thanks much to @corbanmailloux for providing a great framework for implementing flash/fade with HomeAssistant https://github.com/corbanmailloux/esp-mqtt-rgb-led
*/



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>



/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "Your Wifi Name" //type your WIFI information inside the quotes
#define wifi_password "Your Wifi Password"
#define mqtt_server "Your MQTT Server"
#define mqtt_user "yourMQTTusername" 
#define mqtt_password "yourMQTTpassword"
#define mqtt_port 1883


/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define door_state_topic "ha/garagedoor"
#define door_set_topic "ha/garagedoor/set"

const char* on_cmd = "ACTIVATE";
const char* off_cmd = "IDLE";



/**************************** FOR OTA **************************************************/
#define SENSORNAME "garagedoor"
#define OTApassword "YOUR OTA PASSWORD"
int OTAport = 8266;



/**************************** PIN DEFINITIONS ********************************************/
const int redPin = D1;
const int greenPin = D2;
const int bluePin = D3;
#define DoorPin    D7
#define RelayPin   D6
#define BuzzerPin  D5

int calibrationTime = 1;
bool stateOn = false;
const int BUFFER_SIZE = 300;
char* DoorState = "Closed";
int DoorPinValue = 0;
int DoorPinStatus = 0;
WiFiClient espClient;
PubSubClient client(espClient);



/********************************** START SETUP*****************************************/
void setup() {
  setColor(255, 255, 0);
  Serial.begin(115200);

  pinMode(DoorPin, INPUT);
  pinMode(RelayPin, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);

  Serial.begin(115200);
  delay(10);

  ArduinoOTA.setPort(OTAport);

  ArduinoOTA.setHostname(SENSORNAME);

  ArduinoOTA.setPassword((const char *)OTApassword);

  Serial.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Starting Node named " + String(SENSORNAME));


  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);


  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  setColor(0, 255, 0);
  delay(500);
  setColor(1, 1, 1);
}




/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  setColor(0, 0, 0);
}



/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }

  if (stateOn) {
    // Update lights
// open door
  }


  sendState();
}



/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  }


  return true;
}



/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

//  root["state"] = (stateOn) ? on_cmd : off_cmd;
 


  root["doorstate"] = (String)DoorState;



  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(door_state_topic, buffer, true);
}


/********************************** START SET COLOR *****************************************/
void setColor(int inR, int inG, int inB) {
  analogWrite(redPin, inR);
  analogWrite(greenPin, inG);
  analogWrite(bluePin, inB);

  Serial.println("Setting LEDs:");
  Serial.print("r: ");
  Serial.print(inR);
  Serial.print(", g: ");
  Serial.print(inG);
  Serial.print(", b: ");
  Serial.println(inB);
}

/********************************** START ACTIVATE DOOR *****************************************/

void activateDoor() {
  digitalWrite(BuzzerPin, HIGH);
  delay(500);
  setColor(255, 255, 255);
  delay(500);
  setColor(0, 0, 0);
  digitalWrite(BuzzerPin, LOW);
  digitalWrite(RelayPin, HIGH);
  delay(500);
  setColor(255, 255, 255);
  delay(500);
  setColor(0, 0, 0);
  digitalWrite(BuzzerPin, HIGH);
  digitalWrite(RelayPin, LOW);
  delay(500);
  setColor(255, 255, 255);
  delay(500);
  setColor(0, 0, 0);
  digitalWrite(BuzzerPin, LOW);
  delay(500);
  setColor(255, 255, 255);
  delay(500);
  setColor(0, 0, 0);
  digitalWrite(BuzzerPin, HIGH);
  delay(500);
  setColor(255, 255, 255);
  delay(500);
  setColor(0, 0, 0);
  digitalWrite(BuzzerPin, LOW);
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(door_set_topic);
      setColor(0, 0, 0);
      sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



/********************************** START CHECK SENSOR **********************************/
bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}



/********************************** START MAIN LOOP***************************************/
void loop() {

  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();


DoorPinValue = digitalRead(DoorPin);
    if (DoorPinValue == LOW && DoorPinStatus != 1) {
      DoorState = "closed";
      setColor(0, 255, 0);
      sendState();
      DoorPinStatus = 1;
    }

    else if (DoorPinValue == HIGH && DoorPinStatus != 2) {
      DoorState = "open";
      setColor(255, 0, 0);
      sendState();
      DoorPinStatus = 2;
      // activateDoor();
    }

    delay(100);

  if (stateOn) {
    // Update lights
    activateDoor();
    stateOn = false;
  }
  



}





