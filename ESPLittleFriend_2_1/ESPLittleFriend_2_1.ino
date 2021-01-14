#include <ESP8266WiFi.h>
#include <WebSocketClient.h>
#include <Streaming.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 4, false); // RX, TX (1, 2)

const char* ssid     = "D.Misha3";
const char* password = "nissan6585";

char* host = "192.168.1.180";
char* path = "/";
int   port = 3000;

// Переменная, которая хранит подключены ли мы к серверу или нет
bool isConnected = false;

// Создаём обработчик websocket-ов
WebSocketClient webSocketClient;

// Объект WiFi client для создания подключений
WiFiClient client;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  pinMode(LED_BUILTIN, INPUT);

  // Задержка, чтобы всё успело инициализироваться
  delay(10);

  // Подготавливаем GPIO2 для работы SoftwareSerial
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  // Подключение к WiFi
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
  Serial.println("IP устройства: ");
  Serial.println(WiFi.localIP());

  delay(1000);

  // Говорим устройству, что мы по умолчанию отключены от сервера
  msgMainDevice("disconnected");
  
  // Подключаемся к серверу
  wsconnect_loop();

  // Нужно убедиться, что по умолчанию консоль присоединилась к Serial (OBSOLETE)
  // msgMainDevice("unhook");
}

// Создаём буферную переменную для хранения сообщений от разных источников
//String serialMsg = "";   // Serial
//String mySerialMsg = "";  // SoftwareSerial
//String serverMsg = "";    // WebSocket

String buff = "";

// Счетчик, который проверяет если устройство вовремя отвечает
int timeoutCounter = -1;
const int loopDelay = 50;
void loop() {
  // Чтение данных с Serial
  if (parse(Serial, buff)) {
    // Запрос перезапуска с Serial
    if (buff == "reset") {
      // TODO: Производить нормальное отсоединение от сервера
      webSocketClient.sendData("disconnect");
      msgMainDevice("disconnected");
      // disconnect_ws(client);
      
      //char* status_code = [0b1111101000,;
      //webSocketClient.sendData(status_code, WS_OPCODE_CLOSE);
      delay(100);
      ESP.reset();
    }

    // В остальных случаях передаём сообщение на основное устройство
    mySerial << buff;
  }

  // Чтение данных с SoftwareSerial
  if (parse(mySerial, buff)) {
    Serial << "<" << buff << ">\n";

    // Если устройство прислало нам информацию о котле - отправить её на сервер
    if (buff.startsWith("`")) {
      webSocketClient.sendData(buff);
      timeoutCounter = -1;
    }
  }

  // Чтение данных с WebSocket
  // Проверяем, что подсоединение не было разорвано
  if (client.connected()) {
    if (!isConnected) {
      msgMainDevice("connected");
      isConnected = true;
    }
    
    bool isAvailable = webSocketClient.getData(buff);
    if (isAvailable) {
      Serial << "Данные от сервера: " << buff << " (" << buff.length() << " байт)\n";

      /*if (buff == "ping") {
        webSocketClient.sendData("pong");
      }*/
      if (buff == "requestData") {
        webSocketClient.sendData("ok");
        msgMainDevice("requestData");
        // Ставим timeout таймер на 2 секунды
        if (timeoutCounter == -1) timeoutCounter = 2000 / loopDelay;
      }
      // Просьба сервера отправить что-то на основное устройство
      else if (buff.startsWith("send ")) {
        // Срезаем "send "
        buff = buff.substring(String("send ").length());

        mySerial << buff;
      }
      else if (buff == "resetBoard") {
        msgMainDevice("resetBoard");
        
        webSocketClient.sendData("disconnect");
        ESP.reset();
      }
      else if (buff == "switchCauldron") {
        msgMainDevice("switchCauldron");
      }
      else if (buff == "switchCauldronMode") {
        msgMainDevice("switchCauldronMode");
      }
    }
  }
  // Если соединение было разорвано, нужно переподключиться
  else {
    if (isConnected) {
      msgMainDevice("disconnected");
      isConnected = false;
      delay(100);
    }
    
    // Подключаемся не в цикле, чтобы можно было читать данные от других источников
    Serial << "Нет соединения, переподключаюсь...\n"; 
    wsconnect();
  }

  if (timeoutCounter > 0) {
    timeoutCounter--;
    // Serial << timeoutCounter << '\n';
  }
  else if (timeoutCounter == 0) {
    webSocketClient.sendData("mainDeviceDoesntRespond");
    timeoutCounter = -2;
  }
  delay(loopDelay);
}

/* *** Вспомогательные функции *** */
void msgMainDevice(String message) {
  Serial << "Отправляю (осн. у-во): " << message << "*\n";
  mySerial << message + "*";
}

// Реализует побайтовое чтение с потока
bool parse(Stream& source, String& buff) {
  // Если в источнике нет данных, выходим
  if (!source.available()) return false;

  // Очищаем буфер от прошлых сообщений
  buff = "";

  while (source.available()) {
    char c = source.read();
  
    // Звездочка - символ конца сообщения. Если мы достигли звезды, нужно прерваться, даже если есть ещё данные
    if (c == '*') break;
    
    buff += c;
    delay(10); //Иногда сообщение рвется, дадим небольшую задержку
  }

  return true;
}

// Пробует подключиться к серверу до тех пор, пока соединение не будет успешным
void wsconnect_loop() {
  Serial << "\nНачинаю подключение к серверу...\n";

  while (true) {
    bool isSuccessful = wsconnect();

    if (isSuccessful) break;
    Serial << "\nПодключение не удалось\n";
    delay(500);
  }
}

bool wsconnect() {
  // Connect to the websocket server
  if (client.connect(host, port)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    return false;
  }

  // Handshake with the server
  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");

    // Представляемся серверу
    webSocketClient.sendData("ESP");

    return true;
  } else {

    Serial.println("Handshake failed.");
    return false;
  }
}

// Отсоединяет сокет по протоколу WS
void disconnect_ws(Client& socket_client) {
  // Отправляем 0x8700 серверу и говорим, что мы отключаемся.
  socket_client.write((uint8_t) 0x87);
  socket_client.write((uint8_t) 0x00);
  
  socket_client.flush();
  delay(10);
  socket_client.stop();
}
