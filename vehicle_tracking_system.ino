#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

static const int RXPin = 4, TXPin = 5; // GPIO 4=D2(conneect Tx of GPS) and GPIO 5=D1(Connect Rx of GPS
static const uint32_t GPSBaud = 9600;  // if Baud rate 9600 didn't work in your case then use 4800

TinyGPSPlus gps; // The TinyGPS++ object

SoftwareSerial ss(RXPin, TXPin); // The serial connection to the GPS device

float speed;      // Variable  to store the speed
float altitude;   // Variable  to store the altitude
float sats;       // Variable to store no. of satellites response
String direction; // Variable to store orientation or direction of GPS
float latitude, longitude;
int year, month, date, hour, minute, second;
String date_str, time_str;

int pm;

// Insert your network credentials
const char *ssid = "abcd";
const char *password = "iotpassword";

// Insert Firebase project API Key
#define API_KEY "apikey"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "kuldeep.aher212@vit.edu"
#define USER_PASSWORD "*****"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://nodemcu-gps-****-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String latPath = "/latitude";
String altPath = "/altitude";
String longPath = "/longitude";
String speedPath = "/speed";
String timePath = "/timestamp";
String directionPath = "/direction";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Variable to save current epoch time
String timestamp;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

// // Initialize WiFi
// void initWiFi() {
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   Serial.print("Connecting to WiFi ..");
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print('.');
//     delay(1000);
//   }
//   Serial.println(WiFi.localIP());
//   Serial.println();
// }

void setup()
{
  Serial.begin(115200);
  ss.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  Serial.println();
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
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/data";
}

void loop()
{
  while (ss.available() > 0)
  {
    // sketch displays information every time a new sentence is correctly encoded.
    if (gps.encode(ss.read()))
    {
      displayInfo();
      timeStmp();
      postToFB();
      delay(10000);
    }
  }
}

void postToFB()
{
  // Send new readings to database
  if (Firebase.ready())
  {
    sendDataPrevMillis = millis();

    // //Get current timestamp
    timestamp = date_str + time_str;
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    json.set(latPath.c_str(), latitude);
    json.set(longPath.c_str(), longitude);
    json.set(speedPath.c_str(), speed);
    json.set(altPath.c_str(), altitude);
    json.set(directionPath.c_str(), String(direction));
    json.set(timePath.c_str(), timestamp);
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}

void displayInfo()
{
  Serial.print(gps.location.isValid());
  // if (gps.location.isValid()) {
  latitude = (gps.location.lat()); // Storing the Lat. and Lon.
  longitude = (gps.location.lng());
  Serial.print("LAT:  ");
  Serial.println(latitude, 6); // float to x decimal places
  Serial.print("LONG: ");
  Serial.println(longitude, 6);
  speed = gps.speed.kmph(); // get speed
  altitude = gps.altitude.meters();
  sats = gps.satellites.value();                         // get number of satellites
  direction = TinyGPSPlus::cardinal(gps.course.value()); // get the direction
  Serial.println(direction);
  // }
}

void timeStmp()
{
  if (gps.date.isValid())

  {
    date_str = "";
    date = gps.date.day();
    month = gps.date.month();
    year = gps.date.year();
    if (year < 10)
      date_str += '0';
    date_str += String(year);
    date_str += "/";
    if (month < 10)
      date_str += '0';
    date_str += String(month);
    date_str += "/";
    if (date < 10)
      date_str = '0';
    date_str += String(date);
    date_str += "/";
  }

  if (gps.time.isValid())

  {
    time_str = "";
    hour = gps.time.hour();
    minute = gps.time.minute();
    second = gps.time.second();
    minute = (minute + 30);
    if (minute > 59)
    {
      minute = minute - 60;
      hour = hour + 1;
    }
    hour = (hour + 5);
    if (hour > 23)
      hour = hour - 24;
    if (hour >= 12)
      pm = 1;
    else
      pm = 0;
    hour = hour % 12;
    if (hour < 10)
      time_str = '0';
    time_str += String(hour);
    time_str += ":";
    if (minute < 10)
      time_str += '0';
    time_str += String(minute);
    time_str += ":";
    if (second < 10)
      time_str += '0';
    time_str += String(second);
    if (pm == 1)
      time_str += "PM";
    else
      time_str += "AM";
  }
}