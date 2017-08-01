#include <ESP8266WiFi.h>
#include <Streaming.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(14, 12, false, 256); // RX, TX

#define led 13

const char* ssid = "D.Misha3";
const char* password = "nissan6585";

IPAddress ip(192, 168, 1, 181);
IPAddress gateway(192, 168, 1, 150);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(1337);

void msgMainDevice(String message) {        //Отправка сообщения в mySerial с "*" окончанием
  mySerial.print(message + "*");
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  delay(10);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

  pinMode(led, OUTPUT);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //Start the server
  Serial << "Starting server\n";
  server.begin();
  Serial << "Server started! IP is ";

  // Print the IP address
  Serial.println(WiFi.localIP());

  msgMainDevice("unhook");    //Нужно убедиться, что по умолчанию консоль присоединилась к Serial
}

WiFiClient client;

String serialMsg = "";

void checkMainDevice() {
  while (mySerial.available()) {
    char c = mySerial.read();
    if (c != '*') serialMsg += c;
    else if (c == '*' || c == '~') break; //Если вдруг пришло несколько сообщений, а мы не успели прочесть ИЛИ если это еще одно сообщение от консоли
    delay(5); //Иногда сообщение рвется, дадим небольшую задержку
  }

  if (serialMsg != "") {
    Serial << "Recieved data from Main Device: " << serialMsg << endl;
    if (serialMsg.indexOf('~') != -1) {
      //serialMsg = serialMsg.substring(1);     //Получаем всю команду со второй позиции (после ~)
      client.print(serialMsg);                  //Отправляем вывод консоли к клиенту
    }
    serialMsg = "";
  }
}

void analizeClientMessage(String request) {
  if (request != "") Serial << "Recieved command: " << request << endl;
  if (request == "c") {
    client.print("connected");
  }
  else if (request.indexOf("Console ") != -1) {              //Если сообщение - консольный запрос (содержит слово Console внутри)
    //request = request.substring(request.indexOf(' ') + 1);  //Удаляем слово Console и пробел после него из сообщения
    msgMainDevice(request);
  }
  else if (request == "hook") msgMainDevice("hook"); //Запрос "подцепить" консоль
  else if (request == "unhook") msgMainDevice("unhook"); //Запрос "отцепить" консоль
}

void loop() {
  checkMainDevice();
  client = server.available();
  if (client) {
    Serial.println("Client connected.");

    while (client.connected()) {
      checkMainDevice();
      String message = "";
      while (client.available()) {
        char c = client.read();
        if (c != '*') message += c;
        else break; //Если вдруг пришло несколько сообщений, а мы не успели прочесть
      }
      analizeClientMessage(message);
    }
    Serial.println("Client disconnected.");
    msgMainDevice("unhook");    //На случай, если соединение разорвалось
    client.stop();
  }
}
