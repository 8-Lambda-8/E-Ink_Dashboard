#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>


#include <ArduinoOTA.h>
//#include <ArduinoJson.h>
#include "select.h"
//#include "myImages.h"

ADC_MODE(ADC_VCC);

// Display : HTE029A1_V33
//Chip: IL3820

const String host = "Dashboard";
const char* version = __DATE__ " / " __TIME__;
const char* mqtt_server = "MQTT";


const String TopicStatus = "/"+host+"/Status";
const String TopicVersion = "/"+host+"/Version";
const String TopicVoltage = "/"+host+"/Voltage";
const String TopicText = "/"+host+"/Text";
const String TopicData =  "/"+host+"/Data";

//StaticJsonDocument<200> doc;

long mill, mqttConnectMillis, deepSleepMillis;
int sleepSeconds = 60;
WiFiClient espClient;
PubSubClient client(espClient);

HTTPClient http;

char* str2ch(String str){
    if(str.length()!=0){
        char *p = const_cast<char*>(str.c_str());
        return p;
    }
    return const_cast<char*>("");
}

unsigned char* str2uch(String str){
    if(str.length()!=0){
        char *p = const_cast<char*>(str.c_str());
        return (unsigned char*) p;
    }
    return (unsigned char*) const_cast<char*>("");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  for (uint16_t i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg += (char)payload[i];
  }
  Serial.println();
  
  String topicStr = String(topic);  

  topicStr.replace("/"+host+"/","");

  if(topicStr=="imageUrl"){
    http.begin(msg);
    //http.setURL
    int httpCode = http.GET();
    Serial.printf("Http Code: %d \n",httpCode);
    Serial.println(http.getSize());

    Serial.println(http.getString().length());
    Serial.println(http.getString().charAt(0));
    Serial.print("FF:");
    Serial.println(http.getString().charAt(0)==0xFF);
    //Display_clear();
    Display_picture(str2uch(http.getString()));
    http.end();   

  }else if(topicStr=="sleepSeconds"){
    sleepSeconds = msg.toInt();    
  }
  
  Serial.println("endCallback");
}

void reconnect() {
  Serial.println();
  
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = host+"-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str(),"test1","test1",str2ch(TopicStatus),0,true,"OFFLINE")) {
    Serial.println("connected");
    
    client.publish(str2ch(TopicVersion),version,true);  
    client.publish(str2ch(TopicStatus),"ONLINE",true);
    Serial.print("Sub to= ");
    Serial.println("/"+host+"/#");  
    client.subscribe(str2ch("/"+host+"/#"));
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  epd.Init(lut_full_update);
  Serial.println("after Epaper init");
  //Display_clear();
  /* Display_picture(LambdaStartup);
  delay(500); */

  Serial.println(version);  
  
  ESP.wdtDisable();
  ESP.wdtEnable(5000);

  WiFiManager wifiManager;
  WiFi.hostname(host);
  wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 0, 1), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(str2ch(host));
  Serial.println("after wifi Auto Connect");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  ArduinoOTA.begin();
  ArduinoOTA.setHostname(str2ch(host));
  ArduinoOTA.setRebootOnSuccess(true);

  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);  

  Serial.println();
  Serial.println();
  
  /* Display_clear();
  Display_picture(LambdaImage); */
  //Display_myString(str2ch("HelloWorld"), 112);

}

long loops = 0;

void loop() {
  ArduinoOTA.handle();

  if ((millis()-mqttConnectMillis)>10000 && !client.connected()) {
    reconnect();
    mqttConnectMillis = millis();
  }
  client.loop();

  if((millis()-mill)>5000){
    
    //Serial.println("5s");
    loops = 0;

    mill = millis();
    Serial.println();

    char buf[16];
    sprintf(buf, "%d", ESP.getVcc());
    client.publish(str2ch(TopicVoltage),str2ch(buf),true);
    
  }
  if((millis()-deepSleepMillis)>30000){
    Serial.print("go Sleep for ");
    Serial.print(sleepSeconds);
    Serial.println(" Seconds");
    ESP.deepSleep(sleepSeconds*1000*1000,WAKE_RF_DEFAULT);
  }

  loops++;

}