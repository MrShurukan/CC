#include <ESP8266WiFi.h>
#include <WebSocketClient.h>
#include <Streaming.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 4, false); // RX, TX (1, 2)

boolean handshakeFailed = 0;

const char* ssid     = "D.Misha3";
const char* password = "nissan6585";

char* host = "192.168.1.180";  // 
char path[] = "/";
const int port = 3000;

WebSocketClient webSocketClient;
/*unsigned long previousMillis = 0;
  unsigned long currentMillis;
  unsigned long interval = 300; //interval for sending data to the websocket server in ms*/

// Use WiFiClient class to create TCP connections
WiFiClient client;

void msgMainDevice(String message) {        //Отправка сообщения в mySerial с "*" окончанием
  Serial.println("Вывожу: " + message + "*");
  mySerial.print(message + "*");
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  pinMode(LED_BUILTIN, INPUT);     // Initialize the LED_BUILTIN pin as an output

  delay(10);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Подключаюсь к ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi подключен!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(1000);

  wsconnect();
  //  wifi_set_sleep_type(LIGHT_SLEEP_T);

  msgMainDevice("unhook");    //Нужно убедиться, что по умолчанию консоль присоединилась к Serial
}

String serialMsg = "";

void checkMainDevice() {
  while (mySerial.available()) {
    char c = mySerial.read();
    if (c != '*') serialMsg += c;
    else if (c == '*' || c == '~') break; //Если вдруг пришло несколько сообщений, а мы не успели прочесть ИЛИ если это еще одно сообщение от консоли
    delay(5); //Иногда сообщение рвется, дадим небольшую задержку
  }

  if (serialMsg != "") {
    Serial << "Данные от основного устройства: " << serialMsg << endl;
    if (serialMsg.indexOf('~') != -1) {
      //serialMsg = serialMsg.substring(1);     //Получаем всю команду со второй позиции (после ~)
      client.print(serialMsg);                  //Отправляем вывод консоли к клиенту
    }
    else if (serialMsg.indexOf("tempAndCauldronData`") != -1) {
      client.print(serialMsg);
    }
    serialMsg = "";
  }
}

void analyzeClientMessage(String request) {
  if (request != "") Serial << "Получена команда: " << request << endl;
  if (request == "c") {
    client.print("connected");
  }
  else if (request.indexOf("Console ") != -1) {              //Если сообщение - консольный запрос (содержит слово Console внутри)
    // request = request.substring(request.indexOf(' ') + 1);  //Удаляем слово Console и пробел после него из сообщения
    msgMainDevice(request);
  }
  else if (request == "hook") msgMainDevice("hook"); //Запрос "подцепить" консоль
  else if (request == "unhook") msgMainDevice("unhook"); //Запрос "отцепить" консоль
  else if (request == "requestData") msgMainDevice("requestData"); //Запрос о данных котла и температур
  else if (request.startsWith("setDom`")) {
    // request = request.substring(request.indexOf("`") + 1);
    msgMainDevice(request);
  }
}

String consoleMsg = "";
String mySerialMsg = "";
String serverMsg = "";

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    consoleMsg += c;
    delay(10); //Иногда сообщение рвется, дадим небольшую задержку
  }
  if (consoleMsg != "") {
    Serial.println(consoleMsg);
    if (consoleMsg == "reset") {
      ESP.reset();
    }
    mySerial.print(consoleMsg);

    consoleMsg = "";
  }

  while (mySerial.available()) {
    char c = mySerial.read();
    mySerialMsg += c;
    delay(10); //Иногда сообщение рвется, дадим небольшую задержку
  }
  if (mySerialMsg != "") {
    Serial.println("<" + mySerialMsg +  ">");

    if (mySerialMsg.startsWith("tempAndCauldronData`")) {
      webSocketClient.sendData(mySerialMsg);
    }

    mySerialMsg = "";
  }

  if (client.connected()) {
    bool isAvailable = webSocketClient.getData(serverMsg);
    if (isAvailable && serverMsg.length() > 0) {
      Serial.println("Internet data: " + serverMsg);

      if (serverMsg == "requestData") {
        msgMainDevice("requestData");
      }
    }
    else {
    }

    delay(5);
  }
  else {
    Serial.println("Потерял соединение...");
    wsconnect();
  }
}

// *** Дополнительные функции ***
void wsconnect() {
  // Connect to the websocket server
  if (client.connect(host, port)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    delay(1000);

    if (handshakeFailed) {
      handshakeFailed = 0;
      ESP.restart();
    }
    handshakeFailed = 1;
  }

  // Handshake with the server
  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {

    Serial.println("Handshake failed.");
    delay(4000);

    if (handshakeFailed) {
      handshakeFailed = 0;
      ESP.restart();
    }
    handshakeFailed = 1;
  }
}
