
#define TINY_GSM_MODEM_SIM800
      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024
#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseESP8266.h>
#include <FirebaseFS.h>


/*********************************Include Libraries*********************************/
// Used for connecting to GSM800L module
#include <TinyGsmClient.h>

// Used for connecting to GPS module
#include <TinyGPS++.h>
#include <TinyGPSPlus.h>



// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

/********************************* Database Secret *********************************/

// Project database API key
#define API_KEY "AIzaSyB1OhSjNVxfYpVkF3NErZqImdEIAvZueww"

// Real-time database URL
#define DATABASE_URL "https://vehicle-tracking-system-8d230-default-rtdb.firebaseio.com/"

/********************************* User Authentication *********************************/

#define USER_EMAIL "siddhi.patil211@vit.edu"
#define USER_PASSWORD "Siddhi@123"

/********************************* Firebase Objects *********************************/

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

/********************************* Global Variables *********************************/

// Your GPRS credentials
char apn[]  = "airtelgprs.com";
char user[] = "";
char pass[] = "";

// GSM module pins
#define rxPin 4
#define txPin 2
HardwareSerial sim800(1);
TinyGsm modem(sim800); // GSM modem object

// GPS module pins
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(2);
TinyGPSPlus gps; // GPS object

// Create TinyGSM client instances
TinyGsmClientSecure gsm_client_secure_modem(modem, 0);

// Variable to save user UID
String uid;

// Database main path
String databasePath;

// Database child nodes
String latPath = "/latitude";
String lngPath = "/longitude";
String speedPath = "/speed";
String altPath = "/altitude";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

// Variable to save current epoch time
int timestamp;
bool newData;
// Timer variables (send new readings every ten seconds)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

/********************************* Setup *********************************/

void setup() {
  
  Serial.begin(115200);

  // Initializing GSM
  sim800.begin(9600, SERIAL_8N1, rxPin, txPin);
  Serial.println("SIM800L serial initialize");

  // Initializing GPS
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("neogps serial initialize");
  delay(3000);

  // Assign the api key
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL
  config.database_url = DATABASE_URL;

  //////// Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 3;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID (loading)
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Restarting modem for fresh connectivity to network
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

/********************************* Loop *********************************/

void loop() {

  // GSM connection to sim card
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" OK");
  
  // Send new readings to database after this time interval
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

  // Checks if GPS is working
    while (neogps.available()){
      if (gps.encode(neogps.read())){
        newData = true;
        break;
      }
    }

  // If newData is true
  if(true){
  newData = false;
  
  String latitude, longitude;
  float altitude;
  unsigned long date, time, speed, satellites;
  
  latitude = String(gps.location.lat(), 6); // Latitude in degrees (double)
  longitude = String(gps.location.lng(), 6); // Longitude in degrees (double)
  
  altitude = gps.altitude.meters(); // Altitude in meters (double)
  date = gps.date.value(); // Raw date in DDMMYY format (u32)
  time = gps.time.value(); // Raw time in HHMMSSCC format (u32)
  speed = gps.speed.kmph();
  
  Serial.print("Latitude= "); 
  Serial.print(latitude);
  Serial.print(" Longitude= "); 
  Serial.println(longitude);

    //Get current timestamp
    date = String(date);
    timestamp = date.concat(time);
    Serial.print ("time: ");
    Serial.println (timestamp);
s
    parentPath= databasePath + "/" + String(timestamp);

    json.set(latPath.c_str(), String(latitude));
    json.set(lngPath.c_str(), String(longitude));
    json.set(speedPath.c_str(), String(speed));
    json.set(altPath.c_str(), String(altitude));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
}

/********************************* End *********************************/
