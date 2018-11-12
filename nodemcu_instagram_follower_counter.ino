#include <OneWire.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h> //This library you can add via Include Library > Manage Library > 
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>


//-----------SETTINGS-------------
const char* SSID = "ASUS";            // wifi SSID name
const char* PASSWORD = "D@m1945Year"; // wifi PASSWORD

#define MAX_FOLLOWER  1000                      // the amount of follower per hour when LED became a red
#define BTN_SWITCH_SCREEN_PIN  12               // PIN number for switch screen button
#define MAX_SCREEN_NUMBER  3                    // max amount of screen (actually start from 0)
#define LED_PIN 13                              // PIN number for led strip which used for represent total followers income per day
#define INSTAGRAM_PROXY_API "<INSERT URL HERE>" // your instgram proxy api endpoint used for retrieving data from fucking https instagram api to your app

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, LED_PIN, NEO_GRB + NEO_KHZ800);
OneWire ds(0); // for PIN 10 (we need 4.7 kOm)  OneWire DS18S20, DS18B20, DS1822
LiquidCrystal_I2C lcd(0x27, 16, 2); // use i2c_scanner sketch to find correct I2C address, nodemcu v1 use 0x27
//-----------SETTINGS-------------

HTTPClient http;

byte activeScreen = 3;
byte oldScreen = -1;

// The next 5 values retrieved from instagram proxy API
String dailyFollowerIncome;           // total follower income per day
String lastPostDate;        // last post date
String lastPostLikes;       // last post like amount
String lastPostComments;    // last post comments amount
String currentTime;         // current time in string format

float  roomCelsius;         // temperature retrieved from DS18S20

unsigned long min_step = 1;    // pause between instagramm api requests in minutes

int color;
int midFollower, maxFollower;
int R = 30, G = 30, B = 70;

float delta_f = 0.0;
float K = 0.97, k;

byte failedRequestCount; 
int i = 0, j = 0, httpCode;
unsigned long newFollowerCount = 0;
boolean wifiConnected = false , isActivatePrinting = false, start_flag = true;
const char compare[] = "erCount";
unsigned long temperatureCheckMillis = 0, statisticRequestCheckMillis = 0, screenSwitchCheckMillis, updateLedStripMillis = 0, wifiNextAttemptCheckMillis = 0, perMinuteCheckMillis = 0;

unsigned long totalHourSubs = 0, totalDaySubs = 0, deltaHour1, prevFollowerCount;

byte minuteCounter = 0, hourCounter = 0;

unsigned long deltaHour[60], deltaDay[24];

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  lcd.init();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight
  lcd.setCursor(0, 0);
  lcd.print("Init..."); // Start Print text to Line 1
  //TOOD WHAT THE FUCK IS THIS I DONT KNOW
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);

  Serial.begin(9600);
  // настраиваем пин №13 в режим выхода,
  // т.е. в режим источника напряжения
  //pinMode(13, OUTPUT);
  pinMode(BTN_SWITCH_SCREEN_PIN, INPUT);
  lcd.setCursor(0, 0);
  lcd.print("Loading..."); // Start Print text to Line 1
  cleanDeltaHour();
  cleanDeltaDay();
}



void loop() {
  measureTemperature();
  Serial.setDebugOutput(true);
  ESP.wdtFeed(); // implicity restart wifi watchdog
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    wifiNextAttemptCheckMillis = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if (millis() - wifiNextAttemptCheckMillis > 10000) break;
    }
    if (WiFi.status() != WL_CONNECTED) {
      lcd.clear();
      lcd.print("Connect failed");
      delay(500);
      wifiConnected = false;
    } else {
      lcd.clear();
      lcd.print("WiFi connected");
      delay(500);
      wifiConnected = true;
    }
  }

  boolean btn = digitalRead(BTN_SWITCH_SCREEN_PIN);

  if (btn == HIGH) {
    Serial.printf("Btn pressed \n");
    showNextSreenEvery10seconds(btn);
  }

  sendStatisticRequestEvery60seconds();
  showNextSreenEvery10seconds(false);
  switchActiveScreen();
  // Init first screen and don't update it.
  // Fill strip white color with low brightness
  mapStatisticToRgbStrip();
  colorWipe(strip.Color(R, G, B), 10);
  delay(150);
}

void showNextSreenEvery10seconds(boolean forceSwitchScreen) {
  if (forceSwitchScreen || (millis() - screenSwitchCheckMillis > 10000 && wifiConnected)) {
    Serial.print("Show next screen \n");
    screenSwitchCheckMillis = millis();
    activeScreen = activeScreen + 1;
    if (activeScreen > MAX_SCREEN_NUMBER) {
      activeScreen = 0;
    }
    isActivatePrinting = true;
  }
}

void switchActiveScreen() {
  if (!isActivatePrinting) {
    return;
  }
  switch (activeScreen) {
    case 0:
      Serial.print("print screen 1");
      Serial.print("\n");
      printLcdScreen1();
      break;
    case 1:
      Serial.print("print screen 2");
      Serial.print("\n");
      printLcdScreen2();
      break;
    case 2:
      Serial.print("print screen 3");
      Serial.print("\n");
      printLcdScreen3();
      break;
    default:
      Serial.print("print screen default");
      Serial.print("\n");
      printLcdDefault();
  }
  isActivatePrinting = false;
}

void printLcdScreen1() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Page: HappyEndHis:)");
  lcd.setCursor(0, 1);
  lcd.print("Followers:");
  lcd.setCursor(10, 1);
  lcd.print(dailyFollowerIncome);
}

void printLcdScreen2() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Day follow:");
  lcd.print(dailyFollowerIncome);
  lcd.setCursor(0, 1);
  lcd.print("Hour follow:");
  lcd.print(totalHourSubs);
}

void printLcdScreen3() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Post:");
  lcd.setCursor(6, 0);
  lcd.print(lastPostDate);

  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.setCursor(2, 1);
  lcd.print(lastPostLikes);

  lcd.setCursor(7, 1);
  lcd.print("C:");
  lcd.setCursor(9, 1);
  lcd.print(lastPostComments);
}

void printLcdDefault() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time: " + currentTime);
  lcd.setCursor(1, 1);
  lcd.print("Room's ");
  lcd.print((char)223);                  // print Celsius string 
  lcd.print("C:" + String(roomCelsius));
  ;

}


/******************************


        WIFI PART CODE


 ******************************/

/**
 * message format 14279;1541392981;40;0
 **/
void sendStatisticRequestEvery60seconds() {
  if (statisticRequestCheckMillis == 0 || (millis() - statisticRequestCheckMillis > 60000) && wifiConnected) {
    String payload = "";
    i = 0;
    httpCode = 0;
    payload = "";
    http.begin(INSTAGRAM_PROXY_API);
    httpCode = http.GET();
    if (httpCode > 0) {
      failedRequestCount = 0;
      payload = http.getString();
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      failedRequestCount++;
      if (failedRequestCount > 10) {
        lcd.setCursor(0, 4);
        lcd.print("Service");
        lcd.setCursor(1, 3);
        lcd.print("Unavailable");
      }
      return;
    }
    http.end();
    Serial.printf("Payload %s\n", payload.c_str());

    int index1 = payload.indexOf(';');
    int index2 = payload.indexOf(';', index1 + 1);
    int index3 = payload.indexOf(';', index2 + 1);
    int index4 = payload.indexOf(';', index3 + 1);

    dailyFollowerIncome        = payload.substring(0, index1);
    newFollowerCount   = dailyFollowerIncome.toInt();
    lastPostDate     = payload.substring(index1 + 1, index2);
    lastPostLikes    = payload.substring(index2 + 1, index3);
    lastPostComments = payload.substring(index3 + 1, index4);
    currentTime      = payload.substring(index4 + 1);
    Serial.printf("Payload subs %s\n", dailyFollowerIncome.c_str());

    int timeSplitterIndex = currentTime.indexOf(':');
    
    if (statisticRequestCheckMillis == 0) {
      midFollower = 127;
      maxFollower = 255;
      prevFollowerCount = newFollowerCount;
      start_flag = false;
    }

    statisticRequestCheckMillis = millis();
  }
}


/******************************


      Temperature Part


 ******************************/

void measureTemperature() {
  if (temperatureCheckMillis == 0 ||  millis() - temperatureCheckMillis > 60000) {
    byte i,  present = 0, type_s = 1;
    byte data[12];
    byte addr[8];
    if ( !ds.search(addr)) {
      ds.reset_search();
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44); // начинаем преобразование, используя ds.write(0x44,1) с "паразитным" питанием
    delay(1000); // 750 может быть достаточно, а может быть и не хватит
    present = ds.reset();  // мы могли бы использовать тут ds.depower(), но reset позаботится об этом
    ds.select(addr);
    ds.write(0xBE);
    for ( i = 0; i < 9; i++) { // нам необходимо 9 байт
      data[i] = ds.read();
    }
    // конвертируем данный в фактическую температуру
    // так как результат является 16 битным целым, его надо хранить в
    // переменной с типом данных "int16_t", которая всегда равна 16 битам,
    // даже если мы проводим компиляцию на 32-х битном процессоре
    int16_t raw = (data[1] << 8) | data[0];
    raw = raw << 3; // разрешение 9 бит по умолчанию
    if (data[7] == 0x10) {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
    if (!(((float)raw / 16.0) < 0.0)) {
      roomCelsius = (float)raw / 16.0;
      //      Serial.print(" Temperature = ");
      //      Serial.print(roomCelsius);
      //      Serial.print(" Celsius \n");
    }
    temperatureCheckMillis = millis();
  }
}

void calculateStatisticPerInterval1() {
  if (millis() - perMinuteCheckMillis > 60000 && wifiConnected) {
    Serial.print("subs: "); Serial.print(newFollowerCount); Serial.print(", ");
    Serial.print("prev: "); Serial.print(prevFollowerCount); Serial.print(", ");
    int deltaSubscribers = newFollowerCount - prevFollowerCount;
    Serial.printf("deltaHour: %d,", deltaSubscribers);
    if (deltaSubscribers < 0){
        deltaHour[minuteCounter] = 0;
    }else{
      deltaHour[minuteCounter] = deltaSubscribers;
    }

    prevFollowerCount = newFollowerCount;
    totalHourSubs = 0;
    for (int i = 0; i < 59; i++) {

      totalHourSubs += deltaHour[i];
    }
    deltaDay[hourCounter] = totalHourSubs;
    totalDaySubs = 0;
    for (int i = 0; i < 23; i++) {
      totalDaySubs += deltaDay[i];
    }
    minuteCounter++;
    if (minuteCounter > 59) {
      minuteCounter = 0;
      hourCounter++;
      cleanDeltaHour();
      if (hourCounter > 23) {
        hourCounter = 0;
        totalDaySubs = 0;
        cleanDeltaDay();
      }
    }
    perMinuteCheckMillis = millis();
    Serial.print("totalHourSubs: "); Serial.print(totalHourSubs); Serial.print(", ");
    Serial.print("totalDaySubs: "); Serial.print(totalDaySubs); Serial.println(".");
  }

}

void mapStatisticToRgbStrip() {
  if ((updateLedStripMillis == 0 || (millis() - updateLedStripMillis > 60000 * min_step)) && wifiConnected) {
    if (newFollowerCount - prevFollowerCount > 0) {
      calculateStatisticPerInterval1();
      delta_f = delta_f * K + totalDaySubs * (1 - K);
      Serial.print("delta: "); Serial.print(totalDaySubs); Serial.print(", ");
      Serial.print("delta_f: "); Serial.print(delta_f); Serial.print(", ");
      calculateRgbStreepColor();
      // отладка
      Serial.print("R: "); Serial.print(R); Serial.print(", ");
      Serial.print("G: "); Serial.print(G); Serial.print(", ");
      Serial.print("B: "); Serial.println(B);
      colorWipe(strip.Color(R, G, B), 10);
      updateLedStripMillis = millis();
    }
  }
}

void cleanDeltaHour() {
  for (int i = 0; i < 59; i++) {
    deltaHour[i] = 0;
  }
}

void cleanDeltaDay() {
  for (int i = 0; i < 23; i++) {
    deltaDay[i] = 0 ;
  }
}


/******************************


           LED Part


 ******************************/


void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void calculateRgbStreepColor() {
  color = delta_f * 10;
  Serial.println("color: " + String(color));
  if (color <= midFollower) {
    k = map(color, 0, midFollower, 0, 255);
    B = 255 - k;
    G = k;
    R = 10;
  }
  if (color > midFollower) {
    k = map(color, midFollower, maxFollower, 0, 255);
    if (k > 255) k = 255;
    B = 10;
    G = 255 - k;
    R = k;
  }
}
