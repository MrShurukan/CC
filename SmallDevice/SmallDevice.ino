#include <math.h>
#include <EEPROM.h>

#include <Streaming.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <AltSoftSerial.h>
//#include <Nextion.h>

#include <string.h>

AltSoftSerial antenna;

//Nextion nxt;

String Time;

int x = 297;
int y = 44;
int width = 10;
int height = 136;

// Data wire is plugged into port 4 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

//#include <DHT.h>

int t, h;

int starcnt;
int timeOfTheDay;

String buff;
//int NCNT;

int cnt, set;// = (EEPROM.read(0) * 20);

bool pressedBeforeEnc = false;

/*String dayTemp = String(EEPROM.read(0)) + String(EEPROM.read(1));
  String nightTemp = String(EEPROM.read(2)) + String(EEPROM.read(3));*/

int dayTemp, nightTemp;

int dayHour, nightHour;

bool isConnected = false, isDone = false, isArrived = false, sentBefore = false, updatedBefore = false, sendSerial = true;//, ignoreFirstTime = true;

int data[8] = {
  0, 0, EEPROM.read(20), EEPROM.read(21), 1, 1, 0, 0
};

int Data[2] = {
  data[2], data[3]
};

//DHT dht(3, DHT11);

void(* resetFunc) (void) = 0;

int getTemperature(DeviceAddress a) {
  int t = int(sensors.getTempC(a) * 100);
  //antenna.println(t);
  int lastDigit = t % 10;
  if (lastDigit >= 5) t += 10 - lastDigit;
  else t -= lastDigit;
  t = t / 10;
  return t;
}

/*void btn() {
  if (set == 300) set = 500;
  else set = 300;
  sensors.requestTemperatures();
  int h = dht.readHumidity();
  int t = getTemperature(insideThermometer);
  antenna << String(t) << "*" << String(h) << "*" << String(set) << "*";
  }*/

bool onFirstPage = true;
bool ignoreMain = false;

void updateSecondScreen();
void updateThirdScreen();

void updateMainScreen();

void setup() {
  /*EEPROM.write(3, 1);
    EEPROM.write(4, 1);*/
  /*EEPROM.write(0, 21);
    EEPROM.write(1, 0);       Looks like a piece of garbage, lulz
    EEPROM.write(2, 19);
    EEPROM.write(3, 0);
    EEPROM.write(4, 6);
    EEPROM.write(5, 22);*/

  /*EEPROM.put(0, 210);
  EEPROM.put(4, 190);
  EEPROM.put(8, 600);
  EEPROM.put(12, 2200);*/

  EEPROM.get(0, dayTemp);
  EEPROM.get(4, nightTemp);
  EEPROM.get(8, dayHour);
  EEPROM.get(12, nightHour);
//  Data[0] = EEPROM.read(13);
//  Data[1] = EEPROM.read(14);
  set = 300;
  // attachInterrupt(0, btn, FALLING);
  // put your setup code here, to run once:
  antenna.begin(2400);
  //antenna << "Test1" << "\n";
  Serial.begin(9600);
  //Serial << "\n";
  while (antenna.available() > 0) antenna.read();
  // antenna.print("Test2\n");
  //  dht.begin();
  // antenna.print("Test3\n");
  sensors.begin();
  // antenna.print("Test4\n");
  //nxt.init();
  // antenna.print("Test5\n");
  sensors.getAddress(insideThermometer, 0);
  // antenna.print("Test6\n");
  sensors.setResolution(insideThermometer, 12);
  // antenna.print("Test7\n");
  //  antenna.print("Test8\n");
  //updateThirdScreen();
  updateMainScreen();
//  Serial << "data[2] and data[3]" << data[2] << " " << data[3] << endl;
}

void sendFF() {
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);
}

float tVal, tValPrev = 9999;

void updateMainScreen() {
  //int TT = 160;
  float T = float(t) / 10.0;
  int tempTVal;
  tVal = 345 + (T - 15) * 14;
  tempTVal = round(tVal);
  if (tValPrev != tVal) {
    if (tempTVal <= 360) {
      Serial << "z0.val=" << tempTVal;
      sendFF();
    }
    else {
      Serial << "z0.val=" << tempTVal - 360;
      sendFF();
    }
    tValPrev = tVal;
  }
  int tempData3;
  if (data[3] == 1) tempData3 = 0;        //Инверсия
  else tempData3 = 1;

  int currHours = Time.substring(0, Time.indexOf(':')).toInt();
  int currMinutes = Time.substring(Time.indexOf(':') + 1).toInt();
  //  Serial << "\nTIME: " << currHours << ":" << currMinutes << endl;
  //  Serial << "DAYTIME: " << dayHour << endl;
  //  Serial << "NIGHTTIME: " << nightHour << endl;
  int dayHours = (dayHour / 100);
  int nightHours = (nightHour / 100);
  int dayMinutes = (dayHour % 100);
  int nightMinutes = (nightHour % 100);
  //  Serial << "dayHours: " << dayHours << endl;
  //  Serial << "nightHours: " << nightHours << endl;
  //  Serial << "dayMinutes: " << dayMinutes << endl;
  //  Serial << "nightMinutes: " << nightMinutes << endl;

  if (currHours >= dayHours && currHours <= nightHours) {
//    Serial << "Yiii\n";
    if (currHours == dayHours) {
      if (currMinutes >= dayMinutes) {
        timeOfTheDay = 0;
//        Serial << "IT'SA DAY\n";
      }
      else {
        timeOfTheDay = 1;
//        Serial << "IT'SA NIGHT\n";
      }
    }
    else if (currHours == nightHours) {
      if (currMinutes < nightMinutes) {
        timeOfTheDay = 0;
//        Serial << "IT'SA DAY\n";
      }
      else {
        timeOfTheDay = 1;
//        Serial << "IT'SA NIGHT\n";
      }
    }
    else {
      timeOfTheDay = 0;
//      Serial << "IT'SA DAY\n";
    }
  }
  else {
    timeOfTheDay = 1;
//    Serial << "IT'SA NIGHT\n";
  }

  Serial << "qDay.picc=" << /*data[1];*/timeOfTheDay; sendFF();
  Serial << "qGas.picc=" << String(data[2]); sendFF();
  Serial << "qAuto.picc=" << String(tempData3); sendFF();
  Serial << "qFlame.picc=" << !(bool(data[4])); sendFF();
  Serial << "qConn.picc=" << String(!isConnected); sendFF();
  if (!isConnected) {
    Serial << "tTime.pco=RED";
    sendFF();
  }
  else {
    Serial << "tTime.pco=BLACK";
    sendFF();
  }
  Serial << "tTime.txt=\"" << Time << "\""; sendFF();
  if (onFirstPage) Serial << "j0.val=" << data[5]; sendFF();
  Serial << "tSet.txt=\"SET=" << String(data[6] / 10) << "." << String(data[6] % 10) << "\""; sendFF();
  Serial << "tPod.txt=\"POD=" << String(data[7] / 10) << "." << String(data[7] % 10) << "\""; sendFF();
}

void updateSecondScreen() {
  //antenna << "\n" << String(dayTemp);
  Serial << "tDayTemp.txt=\"" << String(dayTemp / 10) + "," + String(dayTemp % 10) << "`C\""; sendFF();
  Serial << "tNightTemp.txt=\"" << String(nightTemp / 10) + "," + String(nightTemp % 10) << "`C\""; sendFF();
}

void updateThirdScreen() {
  Serial << "tDayTime.txt=\"";
  if (dayHour / 100 < 10) Serial << "0";
  Serial << String(dayHour / 100) << ":";
  if ((dayHour % 100) / 10 == 0) Serial << "0";
  Serial << String(dayHour % 100);
  Serial << "\""; sendFF();

  Serial << "tNightTime.txt=\"";
  if (nightHour / 100 < 10) Serial << "0";
  Serial << String(nightHour / 100) << ":";
  if ((nightHour % 100) / 10 == 0) Serial << "0";
  Serial << String(nightHour % 100);
  Serial << "\""; sendFF();
}

int msg[7];
int index = 0;

void flushMsg() {
  for (int i = 0; i < 7; i++) msg[i] = 0;
}

void loop() {
  // put your main code here, to run repeatedly
  //Serial.print("LOOP");
  while (Serial.available() > 0) {
    msg[index++] = char(Serial.read());
  }
  index = 0;
  //antenna.println(msg[2]);
  if (msg[1] == 2) {
    if (msg[2] == 3) {
      if ((dayHour - 10) % 100 == 90) dayHour = ((dayHour / 100 - 1) * 100) + 50;
      else dayHour -= 10;
    }
    if (msg[2] == 4) {
      if ((dayHour + 10) % 100 == 60) dayHour = ((dayHour / 100 + 1) * 100);
      else dayHour += 10;
    }
    if (msg[2] == 5) {
      if ((nightHour - 10) % 100 == 90) nightHour = ((nightHour / 100 - 1) * 100) + 50;
      else nightHour -= 10;
    }
    if (msg[2] == 6) {
      if ((nightHour + 10) % 100 == 60) nightHour = ((nightHour / 100 + 1) * 100);
      else nightHour += 10;
    }
    updateThirdScreen();
  }
  else if (msg[1] == 1) {
    if (msg[2] == 1) {
      //antenna.begin(9600);
      EEPROM.put(0, dayTemp);
      delay(100);
      EEPROM.put(4, nightTemp);
      delay(100);
      EEPROM.put(8, dayHour);
      delay(100);
      EEPROM.put(12, nightHour);
      delay(200);
      resetFunc();
      /*sendSerial = true;
        onFirstPage = true;*/
    }
    else if (msg[2] == 3) dayTemp--;
    else if (msg[2] == 5) dayTemp++;
    else if (msg[2] == 4) nightTemp--;
    else if (msg[2] == 6) nightTemp++;
    updateSecondScreen();
  }
  else if (msg[1] == 0) { //страница
    if (msg[2] == 8) {
      //resetFunc();
      //updateSecondScreen();
      onFirstPage = false;
      sendSerial = false;
      isConnected = false;
    }
    if (msg[2] == 5) {  //компонент
      Serial << "qGas.picc=2";
      sendFF();
      if (data[2] < 1) Data[0] = 1;
      else Data[0] = 0;
      EEPROM.update(20, Data[0]);
      EEPROM.update(21, Data[1]);/*
      sensors.requestTemperatures();
      //        h = dht.readHumidity();
      t = getTemperature(insideThermometer);
      antenna << t << "*";
      if (timeOfTheDay == 1) antenna << nightTemp;
      else antenna << dayTemp;
      antenna << "*" << Data[0] << "*" << Data[1] << "*" << dayHour << "*" << nightHour << "*" << "notupd" << "*" << timeOfTheDay << "*";*/

      //cnt = 0;
    }
    else if (msg[2] == 6) {
      Serial << "qAuto.picc=2";
      sendFF();
      if (data[3] < 1) Data[1] = 1;
      else Data[1] = 0;/*
      sensors.requestTemperatures();
      //        h = dht.readHumidity();
      t = getTemperature(insideThermometer);
      antenna << t << "*";
      if (timeOfTheDay == 1) antenna << nightTemp;
      else antenna << dayTemp;
      antenna << "*" << Data[0] << "*" << Data[1] << "*" << dayHour << "*" << nightHour << "*" << "notupd" << "*" << timeOfTheDay << "*";*/

      //cnt = 0;
    }
  }
  flushMsg();
  if (isArrived) {
    if (!updatedBefore) {
      updateMainScreen();
      /*updateSecondScreen();
        updateThirdScreen();*/
      updatedBefore = true;
    }
  }
  else updatedBefore = false;
  /*if (digitalRead(pin_btn) == 0) {
    if (!pressedBeforeEnc) {
      if (set == 300) set = 500;
      else if (set == 500) set = 280;
      else set = 300;
      //EEPROM.write(0, set);
      sensors.requestTemperatures();
      h = dht.readHumidity();
      t = getTemperature(insideThermometer);
      antenna << t << "*" << h << "*" << set << "*" << Data[0] << "*" << Data[1] << "*";
      pressedBeforeEnc = true;
    }
    }
    else pressedBeforeEnc = false;*/
  /*
    encoder_A = digitalRead(pin_A);     // считываем состояние выхода А энкодера
    encoder_B = digitalRead(pin_B);     // считываем состояние выхода B энкодера*/


  if (!isConnected) {
    if (sendSerial) {
      if (antenna.available() > 0) {
        if (antenna.readStringUntil('*') == "?" ) {
          antenna.print("!*");
          starcnt = 0;
          isConnected = true;
          updateMainScreen();
          updateSecondScreen();
          updateThirdScreen();
          //antenna.println("CONNECTED!");
//          Serial << "\n" << "Connected!";
        }
      }
    }
  }
  else {
    if (sendSerial) {
      if (!sentBefore) {
        sensors.requestTemperatures();
        //        h = dht.readHumidity();
        t = getTemperature(insideThermometer);
        antenna << t << "*";
        if (timeOfTheDay == 1) antenna << nightTemp;
        else antenna << dayTemp;
        antenna << "*" << Data[0] << "*" << Data[1] << "*" << dayHour << "*" << nightHour << "*" << "notupd" << "*" << timeOfTheDay << "*";
        //updateMainScreen();
        sentBefore = true;
        ignoreMain = false;
      }
    }
    cnt++;
    if (sendSerial) {
      if (!isArrived) {
        if (antenna.available() > 0) {
          buff = antenna.readStringUntil('*');
//          Serial << "\n" << starcnt << " " << buff << "*";
          delay(50);
          if (buff.startsWith("?")) resetFunc();
          if (starcnt != 8) data[starcnt] = buff.toInt();
          else Time = buff; //antenna.println(Time);
          starcnt++;
          if (starcnt == 9) {
            Data[0] = data[2];
            Data[1] = data[3];
            EEPROM.update(20, Data[0]);
            EEPROM.update(21, Data[1]);
            if (ignoreMain) updateMainScreen();
            isArrived = true;
            starcnt = 0;
//            Serial << "\n" << "Got data!";
          }
        }
        //antenna.println(starcnt);
      }
    }
  }
  delay(50);
  if (cnt == 200) {      //10 сек
    cnt = 0;
    if (!isArrived) {
      isConnected = false;
      updateMainScreen();
      /*updateSecondScreen();
        updateThirdScreen();*/
      //antenna.println(" DISCNTD!");
//      Serial << "\n" << "Disconnected!";
    }
    isArrived = false;
    sentBefore = false;
  }
}

