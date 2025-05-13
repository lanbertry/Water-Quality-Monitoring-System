#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
#include <LittleFS.h> 

// Insert your network credentials
#define WIFI_SSID "Ryu"
#define WIFI_PASSWORD "lanbert123"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBLwOQI5R_0HCQzNH0cbVxnBqOLDWd-Kb4"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "carlsalvania123@gmail.com" //replace with your gmail
#define USER_PASSWORD "water54321" // Password

// Threshold values
#define TEMP_WARNING_LOW 22
#define TEMP_DANGER_LOW  20
#define TEMP_WARNING_HIGH 34
#define TEMP_DANGER_HIGH  35

#define PH_LOW_WARN  4.5
#define PH_HIGH_WARN 8.5
#define PH_DANGER_LOW 4
#define PH_DANGER_HIGH 9

#define TURBIDITY_WARN 60
#define TURBIDITY_DANGER 80

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://water-quality-monitoring-c4034-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String phPath = "/ph_level";
String turbPath = "/turbidity";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

unsigned long sendDataPrevMillis = 0; 
unsigned long timerDelay = 60000;  //1 min upload delay
unsigned long lastWifiStatusSend = 0;

const long gmtOffset_sec = 8 * 3600;  // UTC+8 (Philippines Time)
const int daylightOffset_sec = 0;  // No daylight saving time in the Philippines
const char *ntpServer = "time.google.com";

float temperature;
float ph_level;
float turbidity;

// Hardware
#define STATUS_LED 13
int RX0 = 33;
int TX0 = 32;

//Declaration of Global Variables
int numData = 0, saveData = 0;
String s;
const byte numChars = 32;
char receivedChars[numChars];

bool newData = false;
bool doneParsing = false;

float n1, n2, n3;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);

    digitalWrite(STATUS_LED, LOW);
      if (millis() - sendDataPrevMillis >= timerDelay) {
        saveToLocalFile(temperature, ph_level, turbidity, timestamp);
        sendDataPrevMillis = millis();  // Update the timestamp of the last data save
      }

    
  }
  digitalWrite(STATUS_LED, HIGH);
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

/***************************************************************************************/
void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '&';
  char rc;

  while (Serial2.available() > 0 && newData == false) {
    rc = Serial2.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    } else {
      receivedChars[ndx] = '\0';  // terminate the string
      numData = ndx;
      ndx = 0;
      newData = true;
    }
  }
}

void showNewData() {
  if (newData == true) {
    Serial.println(receivedChars);
  }
}


int delimiter_count(String data) {
  int found = 0;
  char separator = ',';
  int maxIndex = data.length() - 1;
  for (int i = 0; i < maxIndex; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
    }
  }
  return found;
}

void parseData() {

  char *strings[3];  // an array of pointers to the pieces of the above array after strtok()
  char *ptr = NULL;
  byte index = 0;
  ptr = strtok(receivedChars, ",");  // delimiters, comma

  while (ptr != NULL) {
    strings[index] = ptr;
    index++;
    ptr = strtok(NULL, ",");
  }
  if (index == 3) {
    // convert string data to numbers
    n1 = atof(strings[0]);  //ph
    n2 = atof(strings[1]);  // turbidity
    n3 = atof(strings[2]);  // Water temp
  }
  temperature = n3;
  ph_level = n1;
  turbidity = n2;

  doneParsing = true;
  index = 0;
}

void saveToLocalFile(float temp, float ph, float turb, int time) {
  File file = LittleFS.open("/data.txt", FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  String alert_status = getAlertStatus(temp, ph, turb);

  // Save with alert_status
  file.printf("%.2f,%.2f,%.2f,%d,%s\n", ph, turb, temp, time, alert_status.c_str());  // Add alert_status

  saveData++;
  Serial.printf("%.2f,%.2f,%.2f,%d,%s     This is the correct data save\n", ph, turb, temp, time, alert_status.c_str());
  Serial.printf("%d data save locally \n", saveData);  // Corrected
  Serial2.print("WIFI:" + String(WiFi.status() == !WL_CONNECTED ? 1 : 0) + 
                 ":" + String(saveData) + "&");
  file.close();
}

void uploadBufferedData() {
  if (!LittleFS.exists("/data.txt")) {
    Serial.println("No buffered data file found.");
    return;
  }
  
  File file = LittleFS.open("/data.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open buffered data file");
    return;
  }

  Serial.println("Starting upload of buffered data...");
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim(); // Remove any trailing whitespace or newline characters
    
    // Variables to hold parsed data
    float ph, turb, temp;
    int timestamp;
    char alertStatus[20]; // Buffer for alert status
    
    // Parse the line with alert status
    int parsed = sscanf(line.c_str(), "%f,%f,%f,%d,%19[^,\n]", &ph, &turb, &temp, &timestamp, alertStatus);
    
    if (parsed == 5) { // We expect 5 values now (ph, turb, temp, timestamp, alertStatus)
      String parentPath = databasePath + "/" + String(timestamp);
      
      // Clear previous JSON data
      json.clear();
      
      // Add all data points including alert status
      json.set(tempPath.c_str(), String(temp));
      json.set(phPath.c_str(), String(ph));
      json.set(turbPath.c_str(), String(turb));
      json.set(timePath, String(timestamp));
      json.set("/alert_status", String(alertStatus)); // Add alert status
      
      Serial.printf("Attempting to upload: %.2f,%.2f,%.2f,%d,%s\n", 
                  ph, turb, temp, timestamp, alertStatus);
      
      if (Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json)) {
        Serial.println("Upload successful");
        saveData--;
        
        // Update Nano with current status
        Serial2.print("WIFI:" + String(WiFi.status() == WL_CONNECTED ? 1 : 0) + 
                     ":" + String(saveData) + "&");
        Serial.printf("Remaining saves: %d\n", saveData);
      } else {
        Serial.println("Upload failed: " + fbdo.errorReason());
        break; // Stop on failure to preserve remaining data
      }
      
      // Small delay between uploads to prevent overwhelming the connection
      delay(500);
    } else {
      Serial.printf("Failed to parse line: %s (parsed %d items)\n", line.c_str(), parsed);
    }
  }

  file.close();
  
  // Only delete file if we successfully uploaded all data
  if (saveData <= 0) {
    LittleFS.remove("/data.txt");
    Serial.println("All buffered data uploaded successfully");
  } else {
    Serial.printf("Partial upload complete, %d items remaining\n", saveData);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX0, TX0);
  
  pinMode(STATUS_LED, OUTPUT);
    if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;  // Exit setup if LittleFS fails to initialize
  }
      LittleFS.remove("/data.txt");
  Serial.println("LittleFS mounted successfully.");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  initWiFi();

  


  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
  // databasePath = "/SensorData/" + "/readings";
}

String getAlertStatus(float temp, float ph, float turb) {
  if (temp <= TEMP_DANGER_LOW || temp >= TEMP_DANGER_HIGH ||
      ph <= PH_DANGER_LOW || ph >= PH_DANGER_HIGH ||
      turb >= TURBIDITY_DANGER) {
    return "DANGER";
  }

  if ((temp <= TEMP_WARNING_LOW || temp >= TEMP_WARNING_HIGH) ||
      (ph <= PH_LOW_WARN || ph >= PH_HIGH_WARN) ||
      (turb >= TURBIDITY_WARN)) {
    return "WARNING";
  }

  return "NORMAL";
}


void loop() {
  // put your main code here, to run repeatedly:
  recvWithEndMarker();
  showNewData();

  /*   if (newData == true) {
    parseData();
    newData = false;
  } */

  if (newData) {
  parseData();
  newData = false;

  timestamp = getTime();
 if (WiFi.status() != WL_CONNECTED) {
      // Check if 1 minute has passed since last data save
      if (millis() - sendDataPrevMillis >= timerDelay) {
        saveToLocalFile(temperature, ph_level, turbidity, timestamp);
        sendDataPrevMillis = millis();  // Update the timestamp of the last data save
      }
    }
 }
  

  // Send WiFi status every 5 seconds
  if (millis() - lastWifiStatusSend > 5000) {
    lastWifiStatusSend = millis();
    if (WiFi.status() == WL_CONNECTED) {
    Serial2.print("WIFI:1:" + String(saveData) + "&");  // Added saveData
    digitalWrite(STATUS_LED, HIGH);

    } else {
    Serial2.print("WIFI:0:" + String(saveData) + "&");  // Added saveData
    digitalWrite(STATUS_LED, LOW);

    }
  }

 if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
  uploadBufferedData();  // Flush buffer if any
 }


  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    String alertStatus = getAlertStatus(temperature, ph_level, turbidity);

    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(temperature));
    json.set(phPath.c_str(), String(ph_level));
    json.set(turbPath.c_str(), String(turbidity));
    json.set(timePath, String(timestamp));
    json.set("alert_status", alertStatus);
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}