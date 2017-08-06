#include <Streaming.h>

#include <memorysaver.h>
#include <UTFT.h>
#include <Streaming.h>
#include <SPI.h>
#include <SD.h>
File file;        //Переменная для работы с файлом с SD карты

#include <DS1302RTC.h>
#include <TimeLib.h>

//Пины модуля реального времени:  RST, DAT, CLK
DS1302RTC RTC(5, 6, 7);

//Переменные для работы с часами реального времени
time_t tLast;
time_t t;
tmElements_t tm;


#include "RussianFontsRequiredFunctions.h"

String V = "3.0-alpha";

/*  CC (Cauldron Control) - Это система по управлению котлами на Arduino Mega 2560 с использованием LCD экрана для визуализации и помощи пользователю в ориентировании
    Оригинальная программа была так себе (в силу моего незнания языка), но сейчас я хочу сделать эту работу по максимуму хорошо!

    Создано Ильей Завьяловым (ilyuzhaz@gmail.com), начало разработки: 27.07.2017

    Конец разработки: Все еще в разработке :D


    P.S Скорее всего это читать буду только я сам, но ничего) Проект скорее всего займет долгое время :D
*/

String consolePrefix = "Console >";

UTFT tft(CTE32HR, 38, 39, 40, 41);  //Создаем объект tft с такими выводами (дефолт) и такой моделью

extern uint8_t SmallRusFont[];
extern uint8_t BigRusFont[];
extern uint8_t SevenSegNumFontMDS[];
extern uint8_t Grotesk16x32[];

bool setFontByName(String name) {    //Для возможности устанавливать шрифт через консоль
  if (name == "SmallRusFont") tft.setFont(SmallRusFont);
  else if (name == "BigRusFont") tft.setFont(BigRusFont);
  else if (name == "SevenSegNumFontMDS") tft.setFont(SevenSegNumFontMDS);
  else return false;
  return true;
}

String consoleMsg = "", serialMsg = "";

bool consoleHooked = false;   //Если консоль "подцеплена", то весь ее output перенаправляется в ESP вместо Serial и vice versa

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

void analizeConsoleMsg(String consoleMsg) {
  console(consoleMsg);
  if (consoleMsg.indexOf(' ') != -1) {    //Если команда - это несколько слов

    String firstWord = consoleMsg.substring(0, consoleMsg.indexOf(' ')); //Первое слово

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
    else console("Такой команды нет. Используйте \"help\", чтобы получить список команд");
  }
  else {
    if (consoleMsg == "clear") {                                                /*clear*/
      for (int i = 0; i < 15; i++) console("\n", true);
      console("Очищено");
    }
    else if (consoleMsg == "help") {                                            /*help*/
      //console("\n\t##################HELP###################\n", true);

      delay(20);
      console("\n\t     help - получить список всех команд\n", true);
      delay(20);
      console("\n    clear - \"очищает\" экран консоли, прокручивая ее вниз\n", true);
      delay(20);
      console("\n  getFontSize #FontName# - получить размер шрифта FontName\n", true);
      delay(20);
      console("\n printSerial1 #message# - отправить ваше сообщение в Serial1\n", true);
      delay(20);

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
    analizeConsoleMsg(consoleMsg);

    consoleMsg = "";
  }
}

void serialCommand(String command) {
  Serial1 << command + "*";
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
      console("Консоль была подцеплена!");
      consoleHooked = true;
      console("Подцепленная консоль! К вашим услугам!");   //Отправляем сообщение еще раз, на этот раз уже в телефон
    }
    else if (serialMsg == "unhook") {
      console("Консоль была отцеплена");
      consoleHooked = false;
      console("Консоль была отцеплена");              //Отправляем сообщение еще раз, на этот раз уже в Serial
    }
    else if (serialMsg.indexOf("Console ") != -1) {                //Если сообщение - консольный запрос (содержит слово Console внутри)
      serialMsg = serialMsg.substring(serialMsg.indexOf(' ') + 1); //Удаляем слово Console и пробел после него из сообщения
      analizeConsoleMsg(serialMsg);
    }

    serialMsg = "";
  }
}

String formatValue(int v) {
  return (v < 10 ? "0" + String(v) : String(v));  
}

void redrawCurrentTime() {
  tft.setFont(Grotesk16x32);
  tft.setColor(VGA_GRAY);
  tft.fillRect(tft.getDisplayXSize() - tft.getFontXsize() * 5 - 5, tft.getDisplayYSize() - tft.getFontYsize() - 3, tft.getDisplayXSize() - 1, tft.getDisplayYSize());
  tft.setColor(VGA_BLACK);
  tft.setBackColor(VGA_TRANSPARENT);
  tft.print(formatValue(hour()) + ":" + formatValue(minute()), tft.getDisplayXSize() - tft.getFontXsize() * 5 - 5, tft.getDisplayYSize() - tft.getFontYsize() - 3);
}

void updateTime() {
  t = now();
  if (t != tLast) {
    tLast = t;
    //Если время изменилось на секунду, то:
    if (second() == 0) {      //Каждую новую минуту
      redrawCurrentTime();    //Рисовать новое текущее время в углу
    }
  }
}

void setup() {
  /*Дефолтная инициализация*/
  Serial.begin(9600);
  Serial1.begin(9600);
  setSyncProvider(RTC.get);         //Автоматическая синхронизация с часами каждые пять минут
  Serial << "Добро пожаловать в CauldronContol от IZ-Software! (v" << V << ")\n\n";
  Serial << "Инициализация...";
  // put your setup code here, to run once:
  tft.InitLCD(LANDSCAPE);
  Serial << "Готово\n";
  Serial << "Размер экрана: " << tft.getDisplayXSize() << " на " << tft.getDisplayYSize() << " пикселей\n\n";;
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
  Serial << "Готово!\n";

  /*Инициализация SD карты завершена*/


  /*Начальная отрисовка*/
  tft.setFont(SmallRusFont);
  tft.setColor(VGA_BLACK);
  tft.setBackColor(VGA_TRANSPARENT);
  printRus(tft, String("Управление котлами (версия ") + V + String(")"), CENTER, 2);
  tft.setFont(BigRusFont);
  printRus(tft, "Это тест большого шрифта", CENTER, 50);
  tft.setFont(SevenSegNumFontMDS);
  printRus(tft, "25.0", CENTER, 100);
  
  redrawCurrentTime();        //Время в углу
  /*Начальная отрисовка завершена*/

  Serial << "Функция Setup завершена!\n\n";
  console("Введите \"help\" для списка команд");
}

void loop() {
  // put your main code here, to run repeatedly:
  checkConsole();     //Обязательно проверять консоль и реагировать на нее
  checkESPInput();    //Не забудем и про ввод с приложения


  updateTime();       //Обработка событий по времени
}
