
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x3F,16,2); // set the LCD address to 0x3F for a 16 chars and 2 line display


const char* host = "api.openweathermap.org";
//API
String OPEN_WEATHER_MAP_APP_ID = "78ce99baa2772304ccaa4c1b0842855e";
//ID города
String OPEN_WEATHER_MAP_LOCATION_ID = "480562";
//язык для прогноза
String OPEN_WEATHER_MAP_LANGUAGE = "ru";
//Единицы измеррения
boolean IS_METRIC = true;
//Количество записей
uint8_t MAX_FORECASTS = 15;

//настройки подключения WI-FI
char ssid[] = "DSL-2640U";        
char pass[] = ""; 

//Инициализируем wi-fi клиент
WiFiClient client;
int jsonend = 0;
boolean startJson = false;
int status = WL_IDLE_STATUS;
#define JSON_BUFF_DIMENSION 2500

String text;
String textTemp;

unsigned long lastConnectionTime = 10 * 60 * 1000;     // last time you connected to the server, in milliseconds
const unsigned long postInterval = 10 * 360 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)
unsigned long lastBTN;

//снег
byte snow[8] = 
{
  B00000,
  B10100,
  B01000,
  B10100,
  B00000,
  B00101,
  B00010,
  B00101,
};
//дождь
byte rain[8] = 
{
  B00001,
  B00001,
  B00100,
  B10101,
  B10001,
  B00100,
  B10100,
  B10000,
};
byte sun[8] = 
{
  B00000,
  B10101,
  B00000,
  B01110,
  B01110,
  B00000,
  B10101,
  B00000,
};
byte grom[8] = 
{
  B00000,
  B00000,
  B00000,
  B11111,
  B00110,
  B01100,
  B11000,
  B10000,
};
byte oblako[8] = 
{
  B00100,
  B01110,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
};


int checked_weather = 1;
int cnt = 15;
boolean show = false;

//Подключение к роутеру
void connectWifi() {
  WiFi.begin(ssid,pass);
  lcd.setCursor(0,0);
  lcd.print("Connect wifi");
  Serial.println("Connecting");
  byte i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.setCursor(i,1);
    lcd.print(".");
    Serial.print(".");
    i++;
  }
  Serial.println("");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi connected!");
  Serial.println("WiFi connected!");
  printWiFiStatus();
}
// Вывод информации о подключениее в монитор порта
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// Подлучаем данные по урлу
void makehttpRequest() {
  
  // закрыть любое соединение перед отправкой нового запроса, чтобы разрешить клиенту устанавливать соединение с сервером
  client.stop();
WiFiClient client;
  // успешное подключение
  if (client.connect(host, 80)) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Zapros");
    lcd.setCursor(0,1);
    lcd.print("pogodi");
    // отправляем запрос на сервере
    client.println("GET /data/2.5/forecast?id=" + OPEN_WEATHER_MAP_LOCATION_ID + "&APPID=" + OPEN_WEATHER_MAP_APP_ID + "&mode=json&units=metric&cnt="+cnt+"&lang=ru HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Client Timeout!");
        client.stop();
        return;
      }
    }
    
    char c = 0;
    while (client.available()) {
      //c = client.read();
      // Получаем полный json по скобкам открытия и закрытия
      //Serial.print(c);
      text = client.readStringUntil('\n'); 
    }
    Serial.print(text);
    for(int i = 0; i<text.length(); i++){
      c = text[i];
      if (c == '{') {
        startJson = true;         // set startJson true to indicate json message has started
        jsonend++;
      }
      if (c == '}') {
        jsonend--;
      }
      if (startJson == true) {
        text += c;
      }
      // если jsonend = 0, то мы получили равное количество фигурных скобок
      if (jsonend == 0 && startJson == true) {
        Serial.println("HTTP");
        parseJson(text.c_str(),0);  // разбирать текст строки в функции parseJson
        textTemp = text;
        
        text = "";                // очищаем следующую строку
        startJson = false;        // set startJson to false to indicate that a new message has not yet started
      }
    }
  }
  else {
    // Если соединение не было установлено:
    Serial.println("connection failed");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("cONECT failed!");
  }
}



//to parse json data recieved from OWM
void parseJson(const char * jsonString, int check) {
  //StaticJsonBuffer<4000> jsonBuffer;
  const size_t bufferSize = 2*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 4*JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2*JSON_OBJECT_SIZE(7) + 2*JSON_OBJECT_SIZE(8) + 720;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  // FIND FIELDS IN JSON TREE
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  JsonArray& list = root["list"];
  JsonObject& nowT = list[0];
  JsonObject& later = list[1];

  // including temperature and humidity for those who may wish to hack it in
  
  String city = root["city"]["name"];
  
  float tempNow = nowT["main"]["temp"];
  float humidityNow = nowT["main"]["humidity"];
  String weatherNow = nowT["weather"][0]["description"];
  String dataNow = nowT["dt_txt"];
  String simbolNow = nowT["weather"][0]["icon"];

  float tempLater = later["main"]["temp"];
  float humidityLater = later["main"]["humidity"];
  String weatherLater = later["weather"][0]["description"];
  if(check == 0){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(tempNow);
    lcd.print("C ");
    lcd.print(humidityNow);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print(dataNow);
    drawSimbol(simbolNow);
    Serial.println(simbolNow);
  Serial.println(city);
  Serial.println();
  }
  else{
    float tempNow = list[checked_weather]["main"]["temp"];
    float humidityNow = list[checked_weather]["main"]["humidity"];
    String weatherNow = list[checked_weather]["weather"][0]["description"];
    String dataNow = list[checked_weather]["dt_txt"];
    String simbolNow = list[checked_weather]["weather"][0]["icon"];
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(tempNow);
    lcd.print("C ");
    lcd.print(humidityNow);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print(dataNow);
    drawSimbol(simbolNow);
    Serial.println(simbolNow);
    checked_weather++;
    if(checked_weather >= cnt )checked_weather = 1;
   }
}

void drawSimbol(String simbol){
  if(simbol == "13d" || simbol == "13n"){
    lcd.setCursor(15,0);
    lcd.write(0);
  }
  if(simbol == "11d" || simbol == "11n"){
    lcd.setCursor(15,0);
    lcd.write(3);
  }
  if(simbol == "01d" || simbol == "01n"){
    lcd.setCursor(15,0);
    lcd.write(2);
  }
  if(simbol == "02d" || simbol == "02n" || simbol == "03d" || simbol == "03n" || simbol == "04d" || simbol == "04n"){
    lcd.setCursor(15,0);
    lcd.write(4);
  }
  if(simbol == "09d" || simbol == "09n" || simbol == "10d" || simbol == "10n"){
    lcd.setCursor(15,0);
    lcd.write(1);
  }
}

void setup()
{
  pinMode(D5, INPUT_PULLUP);
  Serial.begin(115200);
  delay(500);
  lcd.init();
  lcd.backlight();
  text.reserve(JSON_BUFF_DIMENSION);
  connectWifi();
  lastBTN = millis();
  lcd.createChar(0, snow);
  lcd.createChar(1, rain);
  lcd.createChar(2, sun);
  lcd.createChar(3, grom);
  lcd.createChar(4, oblako);
}

void loop()
{
  
  if (digitalRead(D5) == HIGH){
    lastBTN = millis();
    show = false;
    Serial.println("asdasd");
    parseJson(textTemp.c_str(),checked_weather);
   }
   Serial.println("milis");
   Serial.println(millis());
   Serial.println("milisBTN");
   Serial.println(lastBTN);
   Serial.println("raznitca");
   Serial.println(millis()-lastBTN);
   if(millis() - lastBTN > 15000 && !show){
    Serial.println("cancel");
    show = true;
    checked_weather = 1;
    parseJson(textTemp.c_str(),0);
   }
  delay(500);
  //OWM requires 10mins between request intervals
  //check if 10mins has passed then conect again and pull
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    makehttpRequest();
  }
}
