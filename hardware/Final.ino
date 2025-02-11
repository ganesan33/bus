#include <Wire.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define BUTTON_PIN 14
#define WIFI_SSID "Kaizoku"
#define WIFI_PASSWORD "12345678"
#define API_KEY "AIzaSyChakVlq7qyjL3iZaZ-xspXsfMjbNYRxBY"
#define FIREBASE_PROJECT_ID "cbms2-o"
#define USER_EMAIL "sentinels0216@gmail.com"
#define USER_PASSWORD "Chikom0216"

LiquidCrystal_I2C lcd(0x27,16,2);
Adafruit_PN532 nfc(21, 22);
RTC_DS3231 rtc;
FirebaseData fbdo; 
FirebaseAuth auth;
FirebaseConfig config;

uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t buffer[16];
uint8_t ub[16];
uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; 
uint8_t uidLength;
uint8_t data[16]; 
uint8_t res[32];        
uint8_t resl = 0; 
unsigned long lastResetTime = 0; 
String s,rn="CBMS";
int val;

void A_update(String path){

  FirebaseJson content;
  FirebaseJson payload;
  FirebaseJsonData jsonData;

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("buses/"+path).c_str())) {
    payload.setJsonData(fbdo.payload().c_str());
    Serial.println("Fetched");
    if (payload.get(jsonData, "fields/availability/stringValue")) {
      Serial.println(jsonData.stringValue);
      val = (jsonData.stringValue).toInt();
    } 
  }
  if (val==0){
    val=0;
  }
  else{
    val-=1;
  }
  content.set("fields/availability/stringValue", String(val));
  if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("buses/"+path).c_str(), content.raw(), "availability")){
    Serial.println("Data updated");
  }
  else{
    Serial.println(fbdo.errorReason());
  }
}

bool Admin(String rn){

 FirebaseJson cont;
  cont.set("fields/Roll no./stringValue", rn);

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("27c/"+rn).c_str(), cont.raw())) {
      Serial.println("Data created");
      return true;
  }
  else{
        Serial.println(fbdo.errorReason());
         return false; 
      }
 
  cont.clear();
}

void reset(String path){

  FirebaseJson content;

  DateTime now = rtc.now();
  if ((now.hour() == 9|| now.hour() == 16 ||now.hour() == 18 || now.hour() == 20) && now.minute() == 0 && now.second() == 0) {
    if (millis() - lastResetTime > 1000) {  
      content.set("fields/availability/stringValue", String(60));
      if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", ("buses"+path).c_str(), content.raw(), "availability")){
        Serial.println("Data updated");
      }
      else{
        Serial.println(fbdo.errorReason());
      }
      lastResetTime = millis();  
    }
  }

}

String Read(int bn){

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, bn, MIFARE_CMD_AUTH_A, keyA)) {
    if (nfc.mifareclassic_ReadDataBlock(bn, buffer)) {
      Serial.print("Successfully read Block: ");
      Serial.println(bn);
      for (uint8_t i = 0; i < 16; i++) {
        s += (char)buffer[i]; 
      }
    } 
    else {
          Serial.print("Failed to read Block: ");
          Serial.println(bn);
    }
  }
  else {
        Serial.print("Authentication failed for Block: ");
        Serial.println(bn);
  }
  return s;

}

void Write(int bn, const char* w){

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, bn, MIFARE_CMD_AUTH_A, keyA)){
    snprintf((char*)data, 16, w);
    if (nfc.mifareclassic_WriteDataBlock(bn, data)){
      Serial.print("Data written to Block: ");
      Serial.println(bn);
    } 
    else {
          Serial.print("Failed to write to Block: ");
          Serial.println(bn);
    }
  }
  else {
        Serial.print("Authentication failed for Block: ");
        Serial.println(bn);
  }
}

void setup() {

  lcd.init();                
  lcd.backlight();

  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("PN532 MIFARE Classic Reader");
  nfc.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!nfc.SAMConfig()) {
    Serial.println("Failed to initialize PN532!");
    while (1);
  }
  Serial.println("Waiting for card...");
  
}

void loop() {

  reset("27c");

  lcd.setCursor(0,0);
  lcd.print(rn);
  String v = "Availability: ";
  lcd.setCursor(0,1);
  lcd.print(v+String(val));
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("Card detected!");

    if(digitalRead(BUTTON_PIN) == LOW){
      Write(4,"231501171\0");
      delay(2000);
    } 
      
    rn = Read(4);
    delay(1500);
    Serial.println(rn);
    if(Admin(rn)){
    A_update("27c");
    }
  } 
  else { 
    Serial.println("Waiting for a card...");
  }

}
