#include <Streaming.h>

#include <EEPROM.h>
#include <memorysaver.h>
#include <UTFT.h>
#include <Streaming.h>
#include <SPI.h>
#include <SD.h>
File myFile;        //Переменная для работы с файлом с SD карты

#include <DS1302RTC.h>
#include <TimeLib.h>

//Пины модуля реального времени:  RST, DAT, CLK
DS1302RTC RTC(5, 6, 7);

//Переменные для работы с часами реального времени
time_t tLast;
time_t t;
tmElements_t tm;

//Переменные для температурных датчиков
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 12

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

//Хранилище всех авто адресов
unsigned char addresses[4][8];


#include "RussianFontsRequiredFunctions.h"

String V = "1.2-beta";

/*  CC (Cauldron Control) - Это система по управлению котлами на Arduino Mega 2560 с использованием LCD экрана для визуализации и помощи пользователю в ориентировании
    Оригинальная программа была так себе (в силу моего незнания языка), но сейчас я хочу сделать эту работу по максимуму хорошо!

    Создано Ильей Завьяловым (ilyuzhaz@gmail.com), начало разработки: 27.07.2017

    Конец разработки: Все еще в разработке :D


    P.S Скорее всего это читать буду только я сам, но ничего) Проект скорее всего займет долгое время :D
*/

String consolePrefix = "Console >";

UTFT tft(CTE32HR, 38, 39, 40, 41);  //Создаем объект tft с такими выводами (дефолт) и такой моделью

//Шрифты
extern uint8_t SmallRusFont[];
extern uint8_t BigRusFont[];
extern uint8_t SevenSegNumFontMDS[];
extern uint8_t Grotesk16x32[];
extern uint8_t SmallSymbolFont[];
//Картинки
extern unsigned short electroIcon[0x960];
extern unsigned short gasIcon[0x960];
extern unsigned short autoIcon[0x960];
extern unsigned short manualIcon[0x960];
extern unsigned short checkIcon[0x640];
extern unsigned short crossIcon[0x640];

#define POD 0
#define OBR 1
#define TPOL 2
#define UL 3
#define DOM 4
#define SETPOD 5
#define SETDOM 6

float T[7] {
  45.0,   //POD
  20.0,   //OBR
  20.0,   //TPOL
  20.0,   //UL
  19.0,   //DOM
  EEPROM.read(3),   //SETPOD
  EEPROM.read(4),   //SETDOM
};


#define FORCE_AUTO_ADDRESSES true
bool useThermometers = true;    //Переменная, поставив которую в false можно "заморозить" считывание с датчиков

int Tcoord[7] {50, 80, 110, 220, 250, 50, 250}; //Кординаты по Y-ку всех строк (а именно самих чисел), у SETPOD и SETDOM координата совпадает с POD и DOM соответсвенно

float Tprev[7];

String months[12] = {                                                                   //Соответсвия названия месяца к его номеру
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec"
};

//Пины для энкодера
#define pin_A 10     //CLK
#define pin_B 9      //DT
#define pin_btn 8    //SW
//Переменные для хранения значений энкодера
int encoder_A;
int encoder_B;
int encoder_A_prev = 0;
//Две кнопки - переключение котла и режима работы
#define switchCauldronPin 3    //Газ   |  Электро
#define switchModePin 2        //Авто  |  Ручной
//Светодиоды
#define redLed A13
#define blueLed A14
#define greenLed A15

//Переменные для настройки работы котла
int prevValues[3];      //Предыдущие значения последующих переменных (для обновления на экране). Снизу указаны индексы
int hyst = EEPROM.read(0);  //prevValues[0]
//Переменные для хранения выбраного котла и режима. В EEPROM сохраняются цифрами 1 и 0 (0 - gas/auto; 1 - electro/manual)
#define ELECTRO 1
#define GAS 0
#define AUTO 1
#define MANUAL 0
int chosenCauldron = EEPROM.read(1); //prevValues[1]
int chosenMode = EEPROM.read(2); //prevValues[2]
//Переменная для хранении данных о состоянии подключения к "маленькому" устройству
bool isConnectedToSmall;


bool setFontByName(String name) {    //Функция для возможности устанавливать шрифт через консоль
  if (name == "SmallRusFont") tft.setFont(SmallRusFont);
  else if (name == "BigRusFont") tft.setFont(BigRusFont);
  else if (name == "SevenSegNumFontMDS") tft.setFont(SevenSegNumFontMDS);
  else if (name == "Grotesk16x32") tft.setFont(Grotesk16x32);
  else if (name == "SmallSymbolFont") tft.setFont(SmallSymbolFont);
  else return false;
  return true;
}

String consoleMsg = "", serialMsg = "";

bool consoleHooked = false;     //Если консоль "подцеплена", то весь ее output перенаправляется в ESP вместо Serial и vice versa
bool mainScreenUpdates = true;  //Иногда нужно останавливать обновление экрана (например, вход в меню)

String formatValue(int v) {
  return (v < 10 ? "0" + String(v) : String(v));
}

#define RAW true

void console(String msg, bool isRaw = false) {    //Функция для вывода сообщения от лица консоли (автоматический перевод строки)
  if (!isRaw) {
    if (!consoleHooked) Serial << consolePrefix << " " << msg << endl;
    else Serial1 << "~" << consolePrefix << " " << msg << endl;
  }
  else {  //Если требуется вывести "сырые" данные (как например список команд "help")
    if (!consoleHooked) Serial << msg;
    else Serial1 << "~" << msg;
  }
}

#define LOG_NAME (formatValue(day()) + months[month() - 1] + year() % 100 + ".log")

void openLogFile(int type = FILE_READ) {
  if (type == FILE_WRITE) myFile = SD.open(LOG_NAME, FILE_WRITE);
  else myFile = SD.open(LOG_NAME);
  delay(1000);
}

#define WITH_SERIAL true
#define NO_SERIAL false
//Типы сообщений в LOG
#define CRITICAL "*КРИТИЧЕСКОЕ* "
#define WARNING "Внимание! "
#define CONSOLE (consolePrefix + " ")

void log(String msg, bool copyToSerial = false, String type = "") {               //Функция для лога в SD карту. Есть возможность писать/не писать в Serial, а также выводить различные типы сообщений (в том числе и консольные)
  myFile << "[" << formatValue(hour()) << ":" << formatValue(minute()) << ":" << formatValue(second()) << "] ";  //Вывести время лога
  if (type != "") myFile << type;     //Если указан тип сообщения, то его нужно вывести
  myFile << msg << endl;

  if (copyToSerial) {
    if (type != CONSOLE) Serial << msg << endl;
    else console(msg);
  }
}

#define HIDDEN true

void executeInConsole(String consoleMsg, bool hidden = false) {                 //Функция для исполнения команды в консоли
  if (!hidden) {
    console(consoleMsg);

    if (!consoleHooked) log("Выполнение команды (с Serial): " + consoleMsg);
    else log("Выполнение команды: " + consoleMsg);
  }

  if (consoleMsg.indexOf(' ') != -1) {    //Если команда - это несколько слов

    String firstWord = consoleMsg.substring(0, consoleMsg.indexOf(' ')); //Первое слово

    if (firstWord != "log" && firstWord != "printLog") {     //Не нужно писать "Выполнение команды", если ты сам выводишь свое сообщение в LOG или хочешь его вывести
      if (!consoleHooked) log("Выполнение команды  (с Serial): " + consoleMsg);
      else log("Выполнение команды: " + consoleMsg);
    }

    consoleMsg = consoleMsg.substring(consoleMsg.indexOf(' ') + 1); //Обрезаем строку от пробела и до конца, чтобы дальше легче было работать
    if (firstWord == "getFontSize") {                                           /*getFontSize*/
      String prevFont = tft.getFont();
      bool b = setFontByName(consoleMsg);   //Переменая bool для определения был ли выбор успешен или нет.
      if (b) {
        console("Размер " + consoleMsg + " : " + tft.getFontXsize() + " на " + tft.getFontYsize() + " пикселей");
        setFontByName(prevFont);
      }
      else console("Такого шрифта нет!");
    }
    else if (firstWord == "printSerial1") {                                     /*printSerial1*/
      console("Отправка в Serial1");
      Serial1.print(consoleMsg);
    }
    else if (firstWord == "printLog") {                                         /*printLog*/
      int amount = consoleMsg.toInt();
      if (amount != 0) {
        log("Запрос вывести последние " + String(amount) + "сообщений", WITH_SERIAL, CONSOLE);
        /*WIP*/
      }
      else {
        log("Запрос вывести все сообщения за день");
        myFile.close();
        openLogFile();                                 //Открываем для чтения
        if (myFile) {
          console("Файл открыт успешно");
          String LOG = "";
          if (consoleHooked) delay(300);
          console("\nДоступно: " + String(myFile.available()) + " байт\n", RAW);                                //Отступ перед выводом лога
          while (myFile.available()) {            //Двойной while нужен, чтобы разделять отправку сообщений delay-ями, для мобильного устройства
            while (myFile.available()) {                   //Если есть данные для чтения из открытого файла
              char c = myFile.read();
              LOG += c;                                    //Читаем очередной байт из файла и сохраняем в String
              if (c == '\n') break;
            }
            console(LOG, RAW);
            LOG = "";
            if (consoleHooked) delay(400);
          }
          myFile.close();
          console("--Конец--\n", RAW);
        }
        else console("Ошибка открытия файла");
        openLogFile(FILE_WRITE);                      //Открываем для записи опять
        if (!myFile) console("Ошибка открытия для записи");
      }
    }
    else if (firstWord == "log") {                                              /*log*/
      log(consoleMsg, NO_SERIAL, "User: ");
    }
    else if (firstWord == "setTime") {                                          /*setTime*/
      int date[6];          //Массив для хранения всей даты
      String num = "";           //Цифры, которые мы будем один за одним добавлять, чтобы создавать число
      //Разбиваем строку на отдельные int элементы
      for (int i = 0, j = 0; i < consoleMsg.length(); i++) {
        char c = consoleMsg[i];
        if (c != ',') num += c;
        if (c == ',' || i == consoleMsg.length() - 1) {
          date[j++] = num.toInt();
          num = "";
        }
      }
      //Записываем в tm переменную
      tm.Year = y2kYearToTm(date[0]);
      tm.Month = date[1];
      tm.Day = date[2];
      tm.Hour = date[3];
      tm.Minute = date[4];
      tm.Second = date[5];
      t = makeTime(tm);
      //Выставляем время
      if (RTC.set(t) == 0) { //Успешно
        setTime(t);
        console("Время было выставлено успешно!");
        executeInConsole("printTime", HIDDEN);
      }
      else console("Не удалось выставить время!");
    }
    else if (firstWord == "selectFont") {                                       /*selectFont*/
      bool b = setFontByName(consoleMsg);
      if (b)     //Если такой шрифт есть
        console("Шрифт выбран успешно");
      else console("Такого шрифта нет в системе!");
    }
    else if (firstWord == "drawText") {                                         /*drawText*/
      int coord[2] = { 0 };   //Координаты (X,Y)
      int pos = consoleMsg.indexOf(",");
      String text = consoleMsg.substring(0, pos);    //Сам текст

      String num = "";
      for (int i = pos + 1, j = 0; i < consoleMsg.length(); i++) {
        char c = consoleMsg[i];
        if (c != ',') num += c;
        if (c == ',' || i == consoleMsg.length() - 1) {
          coord[j++] = num.toInt();
          num = "";
        }
      }

      console("TEXT: " + text + ", X: " + String(coord[0]) + ", Y: " + String(coord[1]));

      printRus(tft, text, coord[0], coord[1]);        //Вывод текста на экран
    }
    else if (firstWord == "changeValue") {                                      /*changeValue*/
      int pos = consoleMsg.indexOf(',');
      String name = consoleMsg.substring(0, pos);
      String value = consoleMsg.substring(pos + 1);
      float flValue = consoleMsg.substring(pos + 1).toFloat();
      //console("Value: " + String(value, 1) + " substring: " + consoleMsg.substring(pos + 1));

      if (name == "tpod") T[POD] = flValue;
      else if (name == "tobr") T[OBR] = flValue;
      else if (name == "ttpol") T[TPOL] = flValue;
      else if (name == "tul") T[UL] = flValue;
      else if (name == "tdom") T[DOM] = flValue;
      else if (name == "tsetpod") {
        T[SETPOD] = flValue;
        EEPROM.update(3, T[SETPOD]);
      }
      else if (name == "tsetdom") {
        T[SETDOM] = flValue;
        EEPROM.update(4, T[SETDOM]);
      }
      else if (name == "mainScreenUpdates") mainScreenUpdates = bool(value);
      else if (name == "hyst") {
        hyst = value.toInt();
        EEPROM.update(0, hyst);
      }
      else if (name == "chosenCauldron") {
        chosenCauldron = (value == "gas" ? GAS : ELECTRO);
        //console("value: " + value + "; bool expr: " + String((value == "gas" ? GAS : ELECTRO)));
        EEPROM.update(1, chosenCauldron);
      }
      else if (name == "chosenMode") {
        chosenMode = (value == "auto" ? AUTO : MANUAL);
        //console("value: " + value + "; bool expr: " + String((value == "auto" ? AUTO : MANUAL)));
        EEPROM.update(2, chosenMode);
      }
      else if (name == "isConnectedToSmall") isConnectedToSmall = bool(value);
      else if (name == "useThermometers") useThermometers = bool(value);
      else console("Такая переменная не внесена в список");
    }
    else console("Такой команды нет. Используйте \"help\", чтобы получить список команд");
  }
  else {              //Если команда - одно слово
    if (consoleMsg == "clear") {                                                /*clear*/
      for (int i = 0; i < 15; i++) console("\n", RAW);
      console("Очищено");
    }
    else if (consoleMsg == "clearLog") {                                        /*clearLog*/
      myFile.close();
      SD.remove(LOG_NAME);        //Закрываем файл и удаляем его, тем самым очищая
      openLogFile(FILE_WRITE);    //Создания файла
    }
    else if (consoleMsg == "printTime") {                                       /*printTime*/
      console("Системное время: [" + formatValue(day()) + " " + months[month() - 1] + " " + year() + "] " +
              formatValue(hour()) + ":" + formatValue(minute()) + ":" + formatValue(second()));
    }
    else if (consoleMsg == "printTemp") {                                       /*printTemp*/
      int delayTime = (consoleHooked ? 200 : 0);
      console("\nПодача     = " + String(T[POD], 1) + " из " + String(T[SETPOD], 1) + "\n", RAW);
      delay(delayTime);
      console("Обратка    = " + String(T[OBR], 1) + "\n", RAW);
      delay(delayTime);
      console("Теплый пол = " + String(T[TPOL], 1) + "\n", RAW);
      delay(delayTime);
      console("Улица      = " + String(T[UL], 1) + "\n", RAW);
      delay(delayTime);
      console("Дом        = " + String(T[DOM], 1) + " из " + String(T[SETDOM], 1) + "\n", RAW);
    }
    else if (consoleMsg == "help") {                                            /*help*/
      //console("\n\t##################HELP###################\n", true);

      int delayTime = (consoleHooked ? 400 : 0);

      console("\n                help - получить список всех команд\n", RAW);
      delay(delayTime);
      console("\n       clear - \"очищает\" экран консоли, прокручивая ее вниз\n", RAW);
      delay(delayTime);
      console("\n      getFontSize #FontName# - получить размер шрифта FontName\n", RAW);
      delay(delayTime);
      console("\n    printSerial1 #message# - отправить ваше сообщение в Serial1\n", RAW);
      delay(delayTime);
      console("\n printLog #amount# - вывести amount последних сообщений (0 для всех за день)\n", RAW);
      delay(delayTime);
      console("\n     log #message# - вывести в LOG свое сообщение (с пометкой USER)\n", RAW);
      delay(delayTime);
      console("\n                 clearLog - очищает LOG этого дня\n", RAW);
      delay(delayTime);
      console("\n           printTime - вывести полное текущее системное время\n", RAW);
      delay(delayTime);
      console("\n       setTime #ГГ,М,Д,Ч,М,С# - выставить новое системное время\n", RAW);
      delay(delayTime);
      console("\n         selectFont #FontName# - выбрать шрифт FontName\n", RAW);        //DEBUG COMMAND
      delay(delayTime);
      console("\n            drawText #Text,X,Y# - вывести Text на (X,Y)\n", RAW);        //DEBUG COMMAND
      delay(delayTime);
      console("\n changeValue #Name,Value# - меняет значение переменной Name (если она прописана)\n", RAW);        //DEBUG COMMAND
      delay(delayTime);
      console("\n            printTemp - выводит все температуры в консоль\n", RAW);

      //console("\n\t#########################################\n", true);
    }
    else console("Такой команды нет. Используйте \"help\", чтобы получить список команд");
  }
}

void checkConsole() {       //Проверка Serial на различные команды для дебага
  while (Serial.available()) {
    char c = Serial.read();
    consoleMsg += c;
    delay(10); //Иногда сообщение рвется, дадим небольшую задержку
  }
  if (consoleMsg != "") {
    executeInConsole(consoleMsg);

    consoleMsg = "";
  }
}

void serialCommand(String command) {
  Serial1 << command + "*";
}

void sendTempCauldronData() {
  String data = String(T[POD], 1) + "," + String(T[SETPOD], 0) + "," + String(T[OBR], 1) + "," + String(T[TPOL], 1) + "," + String(T[UL], 1) + "," + String(T[DOM], 1) + "," + String(T[SETDOM], 1) + "," +
                String((chosenCauldron == GAS ? "gas" : "electro")) + "," + String((chosenMode == AUTO ? "auto" : "manual")) + "," + String(hyst);
  Serial1 << "tempAndCauldronData`" << data;
}

void checkESPInput() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c != '*') serialMsg += c;
    else break;
    delay(10); //Иногда сообщение рвется, дадим небольшую задержку
  }
  if (serialMsg != "") {
    Serial << "Данные от ESP: " << serialMsg << endl;
    if (serialMsg == "hook") {
      log("Консоль была перенаправлена!", WITH_SERIAL, CONSOLE);
      consoleHooked = true;
      console("Перенаправленная консоль! К вашим услугам!");   //Отправляем сообщение еще раз, на этот раз уже в телефон
    }
    else if (serialMsg == "unhook") {
      console("Консоль была подключена назад");
      consoleHooked = false;
      log("Консоль была подключена назад", WITH_SERIAL, CONSOLE);              //Отправляем сообщение еще раз, на этот раз уже в Serial
    }
    else if (serialMsg.indexOf("Console ") != -1) {                //Если сообщение - консольный запрос (содержит слово Console внутри)
      serialMsg = serialMsg.substring(serialMsg.indexOf(' ') + 1); //Удаляем слово Console и пробел после него из сообщения
      executeInConsole(serialMsg);
    }
    else if (serialMsg == "requestData") {
      sendTempCauldronData();
    }

    serialMsg = "";
  }
}

void redrawTime() {
  tft.setFont(Grotesk16x32);
  tft.setColor(VGA_BLACK);
  tft.setBackColor(VGA_GRAY);
  tft.print(formatValue(hour()) + ":" + formatValue(minute()), tft.getDisplayXSize() - tft.getFontXsize() * 5 - 5, tft.getDisplayYSize() - tft.getFontYsize() - 3);
}

void updateTempData() {
  sensors.requestTemperatures();
  for (int i = 0; i < 4; i++) {
    float tempC = sensors.getTempC(addresses[i]);
    T[i] = tempC;
  }
}

void updateTime() {
  t = now();
  if (t != tLast) {
    tLast = t;
    //Если время изменилось на секунду, то:
    if (second() == 0) {      //Каждую новую минуту
      redrawTime();     //Рисовать новое текущее время в углу
    }

    //TODO: Увеличить до 20

    if (second() % 5 == 0) {    //Каждые пять секунд
      if (useThermometers) updateTempData();     //Считывание температурных датчиков
    }

    if (hour() == 0 && minute() == 0 && second() == 0) {      //Если это новый день, то
      myFile.close();
      openLogFile(FILE_WRITE);   //Открываем новый файл для записи лога
      log("Привет, сегодня новый день!", WITH_SERIAL);
    }
  }
}

#define REDRAW true

void updateMainScreen(bool redraw = false) {
  if (redraw) {             //Рисуем весь главный экран
    //Версия и заглавие
    tft.setFont(SmallRusFont);
    tft.setColor(VGA_BLACK);
    tft.setBackColor(VGA_TRANSPARENT);
    printRus(tft, String("Управление котлами (версия ") + V + String(")"), CENTER, 2);
    //Температуры
    tft.setFont(BigRusFont);
    printRus(tft, "Подача     = " + String(T[POD], 1) + " из " + String(T[SETPOD], 0) + "(~" + String(hyst) + ")" , 30, 50);
    printRus(tft, "Обратка    = " + String(T[OBR], 1), 30, 80);
    printRus(tft, "Теплый пол = " + String(T[TPOL], 1), 30, 110);
    printRus(tft, "Улица      = " + String(T[UL], 1), 30, 220);
    printRus(tft, "Дом        = " + String(T[DOM], 1) + " из " + String(T[SETDOM], 1), 30, 250);

    //Время в углу
    tft.setFont(Grotesk16x32);
    tft.setColor(VGA_BLACK);
    tft.setBackColor(VGA_GRAY);
    tft.print(formatValue(hour()) + ":" + formatValue(minute()), tft.getDisplayXSize() - tft.getFontXsize() * 5 - 5, tft.getDisplayYSize() - tft.getFontYsize() - 3);

    //Картинки и иконки
    tft.drawBitmap(325, 80, 50, 48, (chosenCauldron == GAS ? gasIcon : electroIcon));
    tft.drawBitmap(395, 80, 50, 48, (chosenMode == AUTO ? autoIcon : manualIcon));

    tft.drawBitmap(20, 275, 40, 40, (isConnectedToSmall == true ? checkIcon : crossIcon));

    //Теперь заполняем предыдущие значения переменных
    for (int i = 0; i < 7; i++) Tprev[i] = T[i];      //7 - размер массива
  }
  else {              //Если это не полная перерисовка, а именно обновление
    //Проверка всех температур
    for (int i = 0; i < 7; i++) {                     //7 - размер массива
      if (Tprev[i] != T[i]) {
        tft.setFont(BigRusFont);
        int offset;
        if (i < 5)            //Если это любая температура, кроме двух SET-ов
          offset = 13 * 16;     //13 - количество символов до первого числа, 16 - ширина шрифта
        else                  //Если это SETPOD или SETDOM
          offset = 21 * 16;     //21 - количество символов до первого числа, 16 - ширина шрифта

        int startX = 30 + offset;  //30 - начальная сдвижка текста
        //Выводим обновленное число
        tft.setColor(VGA_BLACK);
        tft.setBackColor(VGA_GRAY);
        if (i != SETPOD) printRus(tft, String(T[i], 1), startX, Tcoord[i]);
        else printRus(tft, String(T[i], 0), startX, Tcoord[i]);

        //Запоминаем новое значение
        Tprev[i] = T[i];
      }
    }

    if (prevValues[0] != hyst) {
      tft.setFont(BigRusFont);
      tft.setBackColor(VGA_GRAY);
      tft.setColor(VGA_BLACK);
      int offset = 25 * 16; //25 - количество символов до hyst на экране, 16 - ширина шрифта
      int startX = 30 + offset;  //30 - начальная сдвижка текста
      printRus(tft, String(hyst) + ") ", startX, Tcoord[POD]);     //Выводим обновленный hyst и закрывающую скобку. Tcoord[POD], т.к. координата по Y совпадает с TPOD
      prevValues[0] = hyst;
    }
    if (prevValues[1] != chosenCauldron) {
      tft.drawBitmap(325, 80, 50, 48, (chosenCauldron == GAS ? gasIcon : electroIcon));
      prevValues[1] = chosenCauldron;

    }
    if (prevValues[2] != chosenMode) {
      tft.drawBitmap(395, 80, 50, 48, (chosenMode == AUTO ? autoIcon : manualIcon));
      prevValues[2] = chosenMode;
    }
    if (prevValues[3] != isConnectedToSmall) {
      tft.drawBitmap(20, 275, 40, 40, (isConnectedToSmall == true ? checkIcon : crossIcon));
      prevValues[3] = isConnectedToSmall;
    }
    //WIP
  }
}

void handleEncoderTurn(int dir) {         //Функция для обработки вращения энкодера
  if (dir == 1) { //Вращение по часовой стрелке
    Serial << "Часовая\n";
  }
  else { //Вращение против часовой стрелки
    Serial << "Нечасовая\n";
  }
  delay(50);    //Для того, чтобы убрать дребезг
}

void encButtonPress() {         //Функция для обработки нажатия на кнопку
  Serial << "Энкодер\n";
  delay(50);    //Для того, чтобы убрать дребезг
}
void switchCauldron() {
  Serial << "Смена котла\n";
  executeInConsole("changeValue chosenCauldron," + String((chosenCauldron == GAS ? "electro" : "gas")));
  log("Меняю котел на " + String((chosenCauldron == GAS ? "газовый" : "электрический")), WITH_SERIAL, CONSOLE); \
  sendTempCauldronData();
  delay(50);    //Для того, чтобы убрать дребезг
}
void switchCauldronMode() {
  Serial << "Смена режима котла\n";
  executeInConsole("changeValue chosenMode," + String((chosenMode == AUTO ? "manual" : "auto")));
  log("Меняю режим работы на " + String((chosenMode == AUTO ? "автоматический" : "ручной")), WITH_SERIAL, CONSOLE);
  sendTempCauldronData();
  delay(150);    //Для того, чтобы убрать дребезг (кнопка работает хуже, нужно больше задержки)
}

bool buttonsPressed[3] = { 0 };   //Массив, для хранения информации, была ли нажата кнопка или нет (0 - энкодер, 1 - switchCauldronPin, 2 - switchModePin)

void checkButtonPress(int pin, int index) {   //Универсальная функция для проверки нажатия на кнопку (index - индекс в массиве buttonsPressed)
  if (digitalRead(pin) == 0 ) {  //Если на кнопка зажата
    if (!buttonsPressed[index]) {      //и не нажималась до этого

      switch (pin) {      //Для разных пинов - своя функция
        case pin_btn: encButtonPress(); break;         //Функция для обработки нажатия на кнопку энкодера
        case switchCauldronPin: switchCauldron(); break;         //Функция для обработки нажатия на кнопку смены котла
        case switchModePin: switchCauldronMode(); break;         //Функция для обработки нажатия на кнопку смены режима котла
      }

      buttonsPressed[index] = true;
    }
  }
  else buttonsPressed[index] = false;       //Как только кнопку отпустили - ее можно нажимать еще раз
}

void checkManualInput() {       //Проверка ввода с энкодера
  encoder_A = digitalRead(pin_A);     //Считываем состояние выхода А энкодера
  encoder_B = digitalRead(pin_B);     //Считываем состояние выхода B энкодера
  if ((encoder_A == 0) && (encoder_A_prev == 1)) //Если состояние изменилось с положительного к нулю (произошел проворот)
    handleEncoderTurn(encoder_B);   //Функция для обработки вращения (выход B сообщит направление)
  encoder_A_prev = encoder_A;   //Сохраняем предыдущее значение для последующей обработки

  //Проверка нажатия всех кнопок
  checkButtonPress(pin_btn, 0);
  checkButtonPress(switchCauldronPin, 1);
  checkButtonPress(switchModePin, 2);

}

void assignAddress(unsigned char *to, unsigned char *from) {
  for (int i = 0; i < 8; i++) to[i] = from[i];
}

void setup() {
  /*Дефолтная инициализация*/
  Serial.begin(9600);
  //Serial1.begin(9600);
  setSyncProvider(RTC.get);         //Автоматическая синхронизация с часами каждые пять минут
  Serial << "Добро пожаловать в CauldronContol от IZ-Software! (v" << V << ")\n\n";
  Serial << "Инициализация...";

  pinMode(pin_A, INPUT);
  pinMode(pin_B, INPUT);
  pinMode(pin_btn, INPUT_PULLUP);     //Подтягиваем кнопки встроенным резистором
  pinMode(switchCauldronPin, INPUT_PULLUP);
  pinMode(switchModePin, INPUT_PULLUP);

  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);          //Светодиоды
  pinMode(blueLed, OUTPUT);

  //Температурные датчики
  Serial << "Обнаружение температурных устройств...";
  sensors.begin();
  Serial.print("Найдено: ");
  Serial.println(sensors.getDeviceCount(), DEC);

  if (FORCE_AUTO_ADDRESSES) {
    for (int i = 0; i < 4; i++) {
      if (!sensors.getAddress(addresses[i], i)) Serial << "Ошибка получения адреса для температурного устройства " << i << endl;
      sensors.setResolution(addresses[i], TEMPERATURE_PRECISION);
    }
  }
  else {
    //Адреса датчиков на реальном устройстве
    unsigned char podT[8] = {
      0x28, 0x04, 0x25, 0xB2, 0x05, 0x00, 0x00, 0x4B
    };

    unsigned char tpolT[8] = {
      0x28, 0xE4, 0x58, 0xB2, 0x05, 0x00, 0x00, 0xE3
    };

    unsigned char ulT[8] = {
      0x28, 0xF4, 0x98, 0xB1, 0x05, 0x00, 0x00, 0x12
    };

    unsigned char obrT[8] = {
      0x28, 0xEB, 0xB3, 0xB2, 0x05, 0x00, 0x00, 0x6D
    };

    assignAddress(addresses[POD],podT);
    assignAddress(addresses[TPOL], tpolT);
    assignAddress(addresses[UL], ulT);
    assignAddress(addresses[OBR], obrT);
  }

  tft.InitLCD(LANDSCAPE);

  Serial << "\nРазмер экрана: " << tft.getDisplayXSize() << " на " << tft.getDisplayYSize() << " пикселей\n\n";;
  tft.clrScr();
  /*tft.setColor(VGA_GRAY);
    tft.fillRect(0, 0, tft.getDisplayXSize() - 1, tft.getDisplayYSize() - 1);*/
  tft.fillScr(VGA_GRAY);
  Serial << "Экран готов к отрисовке\n\n";
  /*Дефолтная инициализация завершена*/

  /*Инициализация часов реального времени*/
  Serial << "Синхронизация с часами реального времени";
  if (timeStatus() == timeSet)
    Serial << " осуществлена!\n\n";
  else
    Serial << " провалилась!\n\n";
  /*Инициализация часов реального времени завершена*/

  /*Инициализация SD карты*/
  Serial << "Инициализация SD карты...";
  if (!SD.begin(SD_CHIP_SELECT_PIN)) {
    Serial << "Инициализация провалилась\n\n";
    tft.setFont(BigRusFont);
    tft.setColor(VGA_BLACK);
    tft.setBackColor(VGA_TRANSPARENT);
    /*printRus(tft, "Не удалось инициализировать", CENTER, tft.getDisplayYSize() / 2 - 8);
      printRus(tft, "SD карту", CENTER, tft.getDisplayYSize() / 2 + 8);*/
  }
  else Serial << "Готово!\n";

  openLogFile(FILE_WRITE);
  if (myFile) {
    Serial << "Файл для записи лога открылся корректно (" + LOG_NAME + ")\n";
    /*myFile << "Был произведен перезапуск в " +
      formatValue(hour()) + ":" + formatValue(minute()) + ":" + formatValue(second()) + "\n";*/
    log("Система была перезагружена!", NO_SERIAL, WARNING);
  }
  else Serial << "Не удалось открыть файл для записи лога (" + LOG_NAME + ")\n";
  /*Инициализация SD карты завершена*/


  /*Начальная отрисовка*/
  updateMainScreen(REDRAW);         //Параметр REDRAW (true) сообщает, что нужно перерисовать все, не смотря на то, что все и так уже отрисовано
  Serial << "Начальный экран отрисован успешно!";
  /*Начальная отрисовка завершена*/

  Serial << "\nФункция Setup завершена!\n\n";
  console("Введите \"help\" для списка команд");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (mainScreenUpdates) updateMainScreen();   //Обновление главного экрана при изменении каких-то переменных (всегда свежая инфа на экране)

  checkConsole();       //Обязательно проверять консоль и реагировать на нее
  checkESPInput();      //Не забудем и про ввод с приложения

  checkManualInput();   //Также ручной ввод с помощью энкодера

  updateTime();         //Обновление времени и обработка событий по времени
}
