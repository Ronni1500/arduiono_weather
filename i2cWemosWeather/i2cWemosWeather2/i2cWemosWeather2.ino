
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>



LiquidCrystal_I2C lcd(0x3F,16,2); // set the LCD address to 0x3F for a 16 chars and 2 line display


const char* host = "api.openweathermap.org";
//API KEY
String API_KEY = "78ce99baa2772304ccaa4c1b0842855e";
//ID города
String ID_COUNTRY = "480562";
//язык для прогноза
String LANG = "ru";
//Единицы измеррения
boolean IS_METRIC = true;
//Кол-во записей в прогнозе (каждые 3 часа)
int cnt = 20;

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
byte snow[8] = {B00000,B10100,B01000,B10100,B00000,B00101,B00010,B00101,};
//дождь
byte rain[8] = {B00001,B00001,B00100,B10101,B10001,B00100,B10100,B10000,};
//Солнце
byte sun[8] = {B00000,B10101,B00000,B01110,B01110,B00000,B10101,B00000,};
//Гроза
byte grom[8] = {B00000,B00000,B00000,B11111,B00110,B01100,B11000,B10000,};
//Облачность
byte oblako[8] = {B00100,B01110,B11111,B11111,B00000,B00000,B00000,B00000,};


int checked_weather = 1;
int currentListDetailt = 0;
boolean show = false;
boolean btn_press = false;
boolean success_weather = false;

// Текущая погода или для показа следующего в прогнозе
float tempNow;
int humidityNow;
String weatherNow;
String dataNow;
String simbolNow = "";

// Список погоды
int listTempMorning [3];
int listTempDay [3];
int listTempEvening [3];
int listTempNight [3];
String currentSimbolMorning = "", currentSimbolDay = "", currentSimbolEvening = "", currentSimbolNight = "";
int currentList = 0;
char * listTempName[] = {"Today","Tomorrow","After tomorrow"};

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
  //printWiFiStatus();
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
  // успешное подключение
  if (client.connect(host, 80)) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ZAPROS POGODI");    
    // отправляем запрос на сервере
    client.println("GET /data/2.5/forecast?id=" + ID_COUNTRY + "&APPID=" + API_KEY + "&mode=json&cnt="+cnt+"&units=metric&lang=ru HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("HET OTBETA!");
        lcd.setCursor(0,1);
        lcd.print("PEREZAGRUZITE");
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
        success_weather = true;
      }
    }
  }
  else {
    // Если соединение не было установлено:
    Serial.println("connection failed");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connect failed!");
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
  // Список погоды
  JsonArray& list = root["list"];
  // Первый прогноз
  JsonObject& nowT = list[0];
  // Следующий прогноз
  JsonObject& later = list[1];
  
  nextWeather(list);
  // запоминаем необходимые параметры
  tempNow = nowT["main"]["temp"];
  humidityNow = round(nowT["main"]["humidity"]);
  String simbolNow2 = nowT["weather"][0]["icon"];  
  simbolNow = simbolNow2;
  if(check == 0){
    currentWeather();
    listDay(currentList);
  }
  else{
    tempNow = list[checked_weather]["main"]["temp"];
    humidityNow = round(list[checked_weather]["main"]["humidity"]);
    String simbolNow2 = list[checked_weather]["weather"][0]["icon"];
    simbolNow = simbolNow2;
    currentWeather();
    checked_weather++;
    if(checked_weather >= cnt )checked_weather = 1;
   }
}
void currentWeather(){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(tempNow);
    lcd.print("C ");
    lcd.print(humidityNow);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print(dataNow);
    drawSimbol(simbolNow, 0);
}

void listDay(int i){
  lcd.setCursor(0,1);
  if(listTempMorning[i] == 999){
    lcd.print("* ");
  }else{
    lcd.print(listTempMorning[i]);
    lcd.print(" ");
  }
  if(listTempDay[i] == 999){
    lcd.print("* ");
  }else{
    lcd.print(listTempDay[i]);
    lcd.print(" ");
  }
  if(listTempEvening[i] == 999){
    lcd.print("* ");
  }else{
    lcd.print(listTempEvening[i]);
    lcd.print(" ");
  }
  if(listTempNight[i] == 999){
    lcd.print("*");
  }else{
    lcd.print(listTempNight[i]);
  }
}
void listDayDetail(int i){
  lcd.setCursor(0,1);
  if(i == 0){
    lcd.print("Morning: ");
   if(listTempMorning[0] == 999){
      lcd.print("* ");
    }else{
      lcd.print(listTempMorning[0]);
      lcd.print("C ");
      drawSimbol(currentSimbolMorning,1);
    }
  }
 
  if(i == 1){
    lcd.print("Day: ");
   if(listTempDay[0] == 999){
      lcd.print("* ");
    }else{
      lcd.print(listTempDay[0]);
      lcd.print("C ");
      drawSimbol(currentSimbolDay,1);
    }
  }
  if(i == 2){
    lcd.print("Evening: ");
   if(listTempEvening[0] == 999){
      lcd.print("* ");
    }else{
      lcd.print(listTempEvening[0]);
      lcd.print("C ");
      drawSimbol(currentSimbolEvening,1);
    }
  }
  if(i == 3){
    lcd.print("Night: ");
   if(listTempNight[0] == 999){
      lcd.print("* ");
    }else{
      lcd.print(listTempNight[0]);
      lcd.print("C");
      drawSimbol(currentSimbolNight,1);
    }
  }
}

void nextWeather (JsonArray& list){
//  memset(listTempMorning, 0, sizeof(listTempMorning));
//  memset(listTempDay, 0, sizeof(listTempDay));
//  memset(listTempEvening, 0, sizeof(listTempEvening));
//  memset(listTempNight, 0, sizeof(listTempNight));
  int a = 0;
  int b = 0;
  int c = 0;
  int d = 0;
  boolean simbolMorning = false;
  boolean simbolDay = false;
  boolean simbolEvening = false;
  boolean simbolNight = false;
  for(int i = 0; i < cnt; i++){
    String dateTemp = list[i]["dt_txt"];
    float temp = list[i]["main"]["temp"];
    String simbolNow = list[i]["weather"][0]["icon"];
    
    if(getTime(dateTemp) == "09:00:00" && a < 3){
      if(!simbolMorning){
        currentSimbolMorning = simbolNow;
        simbolMorning = true;
      }
      listTempMorning[a] = temp;
      a+=1;
      continue;
    }
    if(getTime(dateTemp) == "15:00:00" && b < 3){
      if(!simbolDay){
        currentSimbolDay = simbolNow;
        simbolDay = true;
      }
      listTempDay[b] = temp;   
      b+=1;
      continue;
    }
    if(getTime(dateTemp) == "18:00:00" && c < 3){
      if(!simbolEvening){
        currentSimbolEvening = simbolNow;
        simbolEvening = true;
      }
      listTempEvening[c] = temp;     
      c+=1;
      continue;
    }
    if(getTime(dateTemp) == "00:00:00" && d < 3){
      if(!simbolNight){
        currentSimbolNight = simbolNow;
        simbolNight = true;
      }
      listTempNight[d] = temp;
      d+=1;
    }
    //нет значений утра
    if((getTime(dateTemp) == "12:00:00" || getTime(dateTemp) == "15:00:00" || getTime(dateTemp) == "18:00:00" || getTime(dateTemp) == "21:00:00") && a == 0) {   
//      listTempMorning[a] = 999; 
      a+=1;
      if(!simbolMorning){
//        currentSimbolMorning = "-";
        simbolMorning = true;
      }
    }
    // Нет значений дня
    if((getTime(dateTemp) == "18:00:00" || getTime(dateTemp) == "21:00:00") && b == 0){
//      listTempDay[b] = 999;  
      b+=1;
      if(!simbolDay){
//        currentSimbolDay = "-";
        simbolDay = true;
      }
    }
    // Нет значений вечера
    if(getTime(dateTemp) == "21:00:00" && c == 0){ 
//      listTempEvening[c] = 999;
      c+=1;  
      if(!simbolEvening){
//        currentSimbolEvening = "-";
        simbolEvening = true;
      }    
    }
  }
}

//Получить время из строки
String getTime(String str_time){
  String current_time;
  boolean flag = false;
  for(int i = 0; i < str_time.length(); i++){
    if(flag) current_time += str_time[i];
    if(str_time[i] == ' ') flag =true;
  }
  return current_time;
}

// Смволы погоды
void drawSimbol(String simbol, int row){
  if(simbol == "13d" || simbol == "13n"){
    lcd.setCursor(15,row);
    lcd.write(0);
  }
  if(simbol == "11d" || simbol == "11n"){
    lcd.setCursor(15,row);
    lcd.write(3);
  }
  if(simbol == "01d" || simbol == "01n"){
    lcd.setCursor(15,row);
    lcd.write(2);
  }
  if(simbol == "02d" || simbol == "02n" || simbol == "03d" || simbol == "03n" || simbol == "04d" || simbol == "04n"){
    lcd.setCursor(15,row);
    lcd.write(4);
  }
  if(simbol == "09d" || simbol == "09n" || simbol == "10d" || simbol == "10n"){
    lcd.setCursor(15,row);
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
  //Нажата кнопка переключения
  if (digitalRead(D5) == HIGH){
    if(!success_weather){
      makehttpRequest();
     }
     else{
        lastBTN = millis();
        show = false;
    //    parseJson(textTemp.c_str(),checked_weather);
        currentList+=1;
        lcd.clear();
//        currentWeather();
        lcd.setCursor(0,0);        
        if(currentList < 3){
          lcd.print(listTempName[currentList]);
          listDay(currentList);      
        }
        else{
          currentList = 0;
          lcd.print(listTempName[currentList]);
          listDay(currentList);
        }
     }
      delay(200);
      btn_press = true;
   }
   // Бездействие вораживаем обратно
   if(millis() - lastBTN > 15000 && !show){
     show = true;
     btn_press = false;
     checked_weather = 1;
     currentWeather();
     listDay(0);
   } 
  if (millis() - lastConnectionTime > postInterval) {
    // note the time that the connection was made:
    lastConnectionTime = millis();
    makehttpRequest();

//    for(int i = 0; i < 3; i++){
//      Serial.println(listTempMorning[i]);
//    }
//    Serial.println(" ");
//    for(int i = 0; i < 3; i++){
//      Serial.println(listTempDay[i]);
//    }
//    Serial.println(" ");
//    for(int i = 0; i < 3; i++){
//      Serial.println(listTempEvening[i]);
//    }
//    Serial.println(" ");
//    for(int i = 0; i < 3; i++){
//      Serial.println(listTempNight[i]);
//    }
//    Serial.println(" ");
  }
  else if(show){
    delay(5000);
    if(currentListDetailt < 4){
      lcd.clear();
      currentWeather();
      listDayDetail(currentListDetailt);
      currentListDetailt++;
    }
    else{
      currentListDetailt = 0;
    }
  }

}
