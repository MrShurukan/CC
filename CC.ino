#include <Streaming.h>

#include <memorysaver.h>
#include <UTFT.h>
#include <Streaming.h>
#include "RussianFontsRequiredFunctions.h"

String V = "1.1-alpha";

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

bool setFontByName(String name) {    //Для возможности устанавливать шрифт через консоль
  if (name == "SmallRusFont") tft.setFont(SmallRusFont);
  else if (name == "BigRusFont") tft.setFont(BigRusFont);
  else if (name == "SevenSegNumFontMDS") tft.setFont(SevenSegNumFontMDS);
  else return false;
  return true;
}

String consoleMsg = "", serialMsg = "";

void console(String msg) {    //Функция для вывода сообщения от лица консоли (автоматический перевод строки)
  Serial << consolePrefix << " " << msg << endl;
}


void checkConsole() {       //Проверка Serial на различные команды для дебага
  while (Serial.available()) {
    char c = Serial.read();
    consoleMsg += c;
    delay(1); //Иногда сообщение рвется, дадим небольшую задержку
  }
  if (consoleMsg != "") {
    console(consoleMsg);
    if (consoleMsg.indexOf(' ') != -1) {    //Если команда - это несколько слов
      
      String firstWord = consoleMsg.substring(0, consoleMsg.indexOf(' ')); //Первое слово
      
      consoleMsg = consoleMsg.substring(consoleMsg.indexOf(' ') + 1); //Обрезаем строку от пробела и до конца, чтобы дальше легче было работать
      if (firstWord == "getFontSize") {                                           /*getFontSize*/
        String prevFont = tft.getFont();
        bool b = setFontByName(consoleMsg);   //Переменая bool для определения был ли выбор успешен или нет.
        if (b) {
          console("The size of " + consoleMsg + " is " + tft.getFontXsize() + " by " + tft.getFontYsize());
          setFontByName(prevFont);
        }
        else console("No such font!");
      }
      else if (firstWord == "printSerial1") {                                     /*printSerial1*/
        console("Sending to Serial1");
        Serial1.print(consoleMsg);
      }
      else console("Sorry, no such command. Use \"help\" to get command list");
    }
    else {
      if (consoleMsg == "clear") {                                                /*clear*/
        for (int i = 0; i < 15; i++) Serial << endl;
        console("cleared");
      }
      else if (consoleMsg == "help") {                                            /*help*/
        Serial << "\n\t******************HELP*******************\n";

        Serial << "\n\t     help - get list of all commands\n";
        Serial << "\n  clear - \"clears\" the console screen by scrolling it down\n";
        Serial << "\n   getFontSize *FontName* - get size of the FontName font\n";
        Serial << "\n    printSerial1 *message* - print your message to Serial1\n";

        Serial << "\n\t*****************************************\n";
      }
      else console("Sorry, no such command. Use \"help\" to get command list");
    }

    consoleMsg = "";
  }
}

void serialCommand(String command) {
  Serial1 << command + "*";
}

void checkESPInput() {
  while(Serial1.available()) {
    char c = Serial1.read();
    if (c != '*') serialMsg += c;
    else break;
    delay(1); //Иногда сообщение рвется, дадим небольшую задержку
  }
  if (serialMsg != "") {
    /*if (serialMsg == "?") {
      Serial1 << "!";
      Serial << "Connection with ESP was (re)established!"; 
    }
    else if (serialMsg == "ConnectionCheck") serialCommand("confirm");*/
    
    serialMsg = "";
  }
}

void setup() {
  /*Дефолтная инициализация*/
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial << "Welcome to CauldronControl by IZ-Software! (v" << V << ")\n\n";
  Serial << "Initializing...";
  // put your setup code here, to run once:
  tft.InitLCD(LANDSCAPE);
  Serial << "Done\n";
  Serial << "The screen size is " << tft.getDisplayXSize() << " by " << tft.getDisplayYSize() << " pixels\n";;
  tft.clrScr();
  tft.setColor(VGA_GRAY);
  tft.fillRect(0, 0, tft.getDisplayXSize() - 1, tft.getDisplayYSize() - 1);
  Serial << "Screen is now ready to draw menus\n";
  /*Дефолтная инициализация завершена*/

  tft.setFont(SmallRusFont);
  tft.setColor(VGA_BLACK);
  tft.setBackColor(VGA_TRANSPARENT);
  printRus(tft, String("Управление котлами (версия ") + V + String(")"), CENTER, 2);
  tft.setFont(BigRusFont);
  printRus(tft, "Это тест большого шрифта", CENTER, 50);
  tft.setFont(SevenSegNumFontMDS);
  printRus(tft, "25.0", CENTER, 100);
  Serial << "Setup function is done\n\n";
  console("Type \"help\" to get command list");
}

void loop() {
  // put your main code here, to run repeatedly:
  checkConsole();     //Обязательно проверять консоль и реагировать на нее
  checkESPInput();   //Не забудем и про ввод с приложения
}
