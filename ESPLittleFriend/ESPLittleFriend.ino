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

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
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
}

WiFiClient client;

void analizeMessage(String message) {
  if (message != "") Serial << "Recieved command: " << message << endl;
  if (message == "c") {
    client.print("connected");
    Serial << "Sent: connected";
  }
  if (message == "turnOn") digitalWrite(led, HIGH);
  if (message == "turnOff") digitalWrite(led, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  while (Serial.available()) {
    mySerial.write(Serial.read());
  }
  client = server.available();
  if (client) {
    Serial.println("Client connected.");

    while (client.connected()) {

      while (mySerial.available()) {
        Serial.print(mySerial.read());
      }
      while (Serial.available()) {
        mySerial.write(Serial.read());
      }

      String message = "";
      while (client.available()) {
        char c = client.read();
        if (c != '\n' && c != '*') message += c;
        if (c == '*') break; //Если вдруг пришло несколько сообщений, а мы не успели прочесть
      }
      analizeMessage(message);
    }
    Serial.println("Client disconnected.");
    client.stop();
  }
}
