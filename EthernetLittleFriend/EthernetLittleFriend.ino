#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>

//#include <ESP8266WiFi.h>
#include <Streaming.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(6, 7); // RX, TX

#define led 13

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 

IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 150);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(3000);

void msgMainDevice(String message) {        //Отправка сообщения в mySerial с "*" окончанием
  Serial.print(message + "*");
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  delay(10);

  // prepare GPIO2
  /*pinMode(2, OUTPUT);
  digitalWrite(2, 0);*/

  pinMode(led, OUTPUT);

  // Connect to WiFi network
  /*Serial.println();
  Serial.println();
  Serial.print("Подключаюсь к ");
  Serial.println(ssid);

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);*/

  /*while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi подключен!");*/

  Ethernet.begin(mac, ip);

  //Start the server
  mySerial << "Запускаю сервер\n";
  server.begin();
  mySerial << "Сервер запущен на ";

  // Print the IP address
  mySerial.println(Ethernet.localIP());

  msgMainDevice("unhook");    //Нужно убедиться, что по умолчанию консоль присоединилась к Serial
}

EthernetClient client;

String serialMsg = "";

void checkMainDevice() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '*') serialMsg += c;
    else if (c == '*' || c == '~') break; //Если вдруг пришло несколько сообщений, а мы не успели прочесть ИЛИ если это еще одно сообщение от консоли
    delay(5); //Иногда сообщение рвется, дадим небольшую задержку
  }

  if (serialMsg != "") {
    mySerial << "Данные от основного устройства: " << serialMsg << endl;
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

void analizeClientMessage(String request) {
  if (request != "") mySerial << "Получена команда: " << request << endl;
  if (request == "c") {
    client.print("connected");
  }
  else if (request.indexOf("Console ") != -1) {              //Если сообщение - консольный запрос (содержит слово Console внутри)
    //request = request.substring(request.indexOf(' ') + 1);  //Удаляем слово Console и пробел после него из сообщения
    msgMainDevice(request);
  }
  else if (request == "hook") msgMainDevice("hook"); //Запрос "подцепить" консоль
  else if (request == "unhook") msgMainDevice("unhook"); //Запрос "отцепить" консоль
  else if (request == "requestData") msgMainDevice("requestData"); //Запрос о данных котла и температур
}

void loop() {
  checkMainDevice();
  client = server.available();
  if (client) {
    mySerial.println("Клиент подсоединился.");

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
    mySerial.println("Клиент отсоединился.");
    msgMainDevice("unhook");    //На случай, если соединение разорвалось
    client.stop();
  }
}
