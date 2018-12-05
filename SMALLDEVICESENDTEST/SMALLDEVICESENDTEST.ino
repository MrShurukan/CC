#include <math.h>
#include <EEPROM.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Streaming.h>

RF24 module(2, 5);  // CE, CS
const byte addresses[][6] = {"1Node", "2Node"};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(250000);
  module.begin();
  Serial << "Модуль "<< (module.isChipConnected() ? "подключен к шине" : "отключен от шины") << " SPI\n";

  module.openWritingPipe(addresses[1]); // 2Node
  module.openReadingPipe(1, addresses[0]); // 1Node
  module.setPALevel(RF24_PA_MAX);
  module.startListening();
}

void moduleSend(String s) {
  delay(300);
  module.stopListening();
  //Serial << "Отправляю: " << s << "\n";
  module.write(s.c_str(), s.length());
  module.startListening();
}

bool isConnected = false;

int lastSum;
String lastMessage, myLastMessage;

#define DATA 0
#define SUM 1
#define CONFIRMATION 2

int waitingFor = DATA;
void sendPacket(String str) {
  lastSum = crc(str);
  Serial << "Sending '" << str << "'; sum is " << lastSum << endl;

  moduleSend(str);
  myLastMessage = str;
  waitingFor = SUM;
}

#define FAILED -1
#define SUCCESS 0

int analyzeData(String sum) {
  waitingFor = DATA;
  if (sum.toInt() == lastSum) {
    Serial << "Success! Sum is correct!\n";
    return SUCCESS;
  }
  else {
    Serial << "Failed, sum is wrong\n";
    return FAILED;
  }
}

unsigned long long lastMessageTime;
//int someData = 0;

bool freezePackets = false;

void disconnect(bool notify = true) {
  isConnected = false;
  freezePackets = false;
  if (notify) moduleSend("***d");
  waitingFor = DATA;
}

void loop() {
  // put your main code here, to run repeatedly:
  if (module.available()) {
    char r_message[100];
    module.read(&r_message, sizeof(r_message));

    Serial << "Got message: '" << r_message << "';\n";
    Serial << "We are waiting for " << (waitingFor == DATA ? "data" : (waitingFor == SUM ? "sum" : "confirmation")) << endl;

    String message = String(r_message);

    if (message.startsWith("***")) {
      Serial << "It is a special command!\n";
      Serial << "waitingFor: " << (waitingFor != CONFIRMATION) << " isConnected: " << isConnected << endl; 
      if (message == "***d") {
        disconnect(false);
        Serial << "Got emergency code, disconnected!\n";  
        return;
      }
      else if ((waitingFor != CONFIRMATION) && isConnected) {
        Serial << "We got out of sync, reconnecting!\n";
        disconnect();
        return;
      }
    }
    
    if (waitingFor == SUM) {
      if (analyzeData(message) == SUCCESS) {
        moduleSend("***s");
        freezePackets = false;
      }
      else {
        delay(100);
        moduleSend("***f");
        Serial << "New attempt...\n";
        freezePackets = true;
        delay(300);
        sendPacket(myLastMessage);
      }
      lastMessageTime = millis();
    }
    else if (waitingFor == CONFIRMATION) {
      if (message == "***s") {
        waitingFor = DATA;
        Serial << "Message was sent successfuly\n";
        // Now we may use lastMessage;
        Serial << "We may now use: " << lastMessage << endl;

        String header = lastMessage.substring(0, lastMessage.indexOf(":"));
        message = lastMessage.substring(lastMessage.indexOf(":") + 1);

        Serial << "The header is: " << header << endl;

        if (header == "BigData") {
          Serial << ">>> We got data from big: " << message << endl;
          sendPacket("SmallData:SOMEDATA");
        }
      }
      else if (message == "***f") {
        Serial << "Message wasn't sent successfuly\n";
      }
      else {
        Serial << "Out of sync, trying to reconnect\n";
        moduleSend("***d");
        disconnect();
      }
      lastMessageTime = millis();
    }
    else if (waitingFor == DATA) {
      if (isConnected) {
        int sum = crc(message);
        Serial << sum << endl;
        delay(300);
        moduleSend(String(sum));
        waitingFor = CONFIRMATION;
        lastMessage = message;
      }
      else
        if (message == "***?") {
          isConnected = true;
          moduleSend("***!");
          Serial << "We are now connected!\n";
        }
      lastMessageTime = millis();
    }
  }

  if (!isConnected) {
    //if (millis() % 1000 == 0) moduleSend("***?");
  }
  else {
    if (millis() - lastMessageTime >= 10000) {
      Serial << "Disconnected!\n";
      disconnect(false);
      waitingFor = DATA;
      return;
    }
    //if (millis() % 8000 == 0 && !freezePackets) sendPacket("BigData:" + String(millis()));
  }

//  delay(20);
}

int crc(String str) {
  int sum = 0;
  //Serial << "\nCounting sum"; 
  for (int i = 0; i < str.length(); i++) {
    //Serial << " (" << str[i] << " ; " << (int)(str[i]) * 2 << ") "; 
    sum += str[i] * 2;
  }
  //Serial << "\n";

  /*String s_sum = String(sum);
  sum = 0;
  for (int i = 0; i < s_sum; i++) {
    sum += str[i] * 2;
  }*/
  
  return sum;
}

/*int letterSum(String str) {
  int sum = 0;
  for (int i = 0; i < str.length(); i++) {
    Serial << " (" << str[i] << " ; " << (int)(str[i]) * 2 << ") "; 
    sum += str[i] * 2;
  }
}*/

