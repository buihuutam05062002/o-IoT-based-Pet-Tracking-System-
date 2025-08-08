#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>

#define RX_GPS 4
#define TX_GPS 5  

SoftwareSerial gpsSerial(RX_GPS, TX_GPS);;
TinyGPSPlus gps;

float lattitude = 1.0, longitude = 1.0;

#define RX_SIM 16  // GPIO16 (RXD2)
#define TX_SIM 17

HardwareSerial SerialAT(2);

char apn[] = "m3-world";
char user[] = "";
char pass[] = "";

const String firebase_HOST = "https://kinhdovido-default-rtdb.asia-southeast1.firebasedatabase.app/";
const String FIREBASE_AUTH = "AIzaSyBVMaHOEF_MV8G3nDGY5ghwBpeoFDJ3_xM";

const unsigned long SEND_INTERVAL = 10000; 
unsigned long lastSendTime = 0;

void setup() {
  gpsSerial.begin(9600);
  SerialAT.begin(115200, SERIAL_8N1, RX_SIM, TX_SIM);
  Serial.begin(115200);

  delay(1000);
  sendAT("AT", 500);
  sendAT("AT+CSQ", 500);
  sendAT("AT+CGMR", 500);
  sendAT("AT+CNMP=38", 500); 

  sendAT("AT+CGDCONT=1,\"IP\",\"" + String(apn) + "\"", 1000); 
  sendAT("AT+NETOPEN", 500);
  sendAT("AT+CGATT=1", 500); 
  sendAT("AT+CGACT=1,1", 500); 
  sendAT("AT+CGPADDR=1", 500); 
  sendAT("ATE0", 1000);
  sendAT("AT+COPS=0", 1000);  

}

void loop() {
  if(gpsSerial.available()){
    int data = gpsSerial.read();
    if(gps.encode(data)){
      lattitude = (gps.location.lat());
      longitude = (gps.location.lng());
        Serial.print("Vị trí: ");
        Serial.print(lattitude, 6);
        Serial.print(", ");
        Serial.println(longitude, 6);
        if(lattitude != 0.0 && longitude != 0.0){
        unsigned long currentTime = millis();
        if (currentTime - lastSendTime >= SEND_INTERVAL) {
           sendDataToFirebase();
           lastSendTime = currentTime;
              }
           }
        }
      }
    }

void sendDataToFirebase() {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["latitude"] = String(lattitude,6);
  jsonDoc["longitude"] = String(longitude,6);

  
  String jsonData;
  serializeJson(jsonDoc, jsonData);
  int dataLength = jsonData.length();
  sendAT("AT+HTTPTERM", 500);
  sendAT("AT+HTTPINIT", 500);
  String firebasePath = "gps_data.json?auth=" + FIREBASE_AUTH;
  String url = firebase_HOST + firebasePath;
  sendAT("AT+HTTPPARA=\"URL\",\"" + url + "\"", 500);
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 500);
  sendAT("AT+HTTPDATA=" + String(dataLength) + ",15000", 8000);
  delay(500);
  SerialAT.print(jsonData);
  delay(1000);
  sendAT("AT+HTTPACTION=4", 8000); 
  delay(2000);
  sendAT("AT+HTTPREAD", 500); 
  sendAT("AT+HTTPTERM", 500);
  

}
void enterCommandMode() {
  delay(1000);
  SerialAT.print("+++");
  delay(1000);
  String response = readResponse(1000);
  if (response.indexOf("OK") != -1 || response.length() > 0) {
    Serial.println("Enter command line");
  } else {
    Serial.println("Failed to enter command mode");
    delay(1000);
    SerialAT.print("+++");
    delay(1000);
  }
}

void sendAT(String command, int delayTime) {
  SerialAT.println(command);
  delay(delayTime);
  String response = readResponse(delayTime);
  Serial.println("Command: " + command + "| Response: " + response);
}

String readResponse(int timeout) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;
      start = millis();
    }
  }
  return response;
}
