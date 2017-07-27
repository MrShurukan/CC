#include <Streaming.h>

#include <memorysaver.h>
#include <UTFT.h>
#include <Streaming.h>
#include "RussianFontsRequiredFunctions.h"

String V = "1.0-alpha";

/*  CC (Cauldron Control) - Это система по управлению котлами на Arduino Mega 2560 с использованием LCD экрана для визуализации и помощи пользователю в ориентировании
*   Оригинальная программа была так себе (в силу моего незнания языка), но сейчас я хочу сделать эту работу по максимуму хорошо!
*   
*   Создано Ильей Завьяловым (ilyuzhaz@gmail.com), начало разработки: 08.06.2017
*   
*   Конец разработки: Все еще в разработке :D
*
*
*   P.S Скорее всего это читать буду только я сам, но ничего) Проект скорее всего займет долгое время :D
*/

extern uint8_t SmallRusFont[];
extern uint8_t BigRusFont[];
extern uint8_t SevenSegNumFontMDS[];

UTFT tft(CTE32HR, 38, 39, 40, 41);  //Создаем объект tft с такими выводами (дефолт) и такой моделью

String consoleMsg = "";

void checkConsole() {       //Проверка Serial на различные команды для дебага
  while (Serial.available()) consoleMsg += Serial.read();
  if (consoleMsg != "") {
    
    consoleMsg = "";
  }
}

void setup() {
  /*Дефолтная инициализация*/
  Serial.begin(115200);
  Serial << "Welcome to CauldronControl by IZ-Software! (v" << V << ")\n\n";
  Serial << "Initializing...";
  // put your setup code here, to run once:
  tft.InitLCD(LANDSCAPE);
  Serial << "Done\n";
  Serial << "The screen size is " << tft.getDisplayXSize() << " by " << tft.getDisplayYSize() << " pixels\n";;
  tft.clrScr();
  tft.setColor(VGA_GRAY);
  tft.fillRect(0,0, tft.getDisplayXSize()-1, tft.getDisplayYSize()-1);
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
  Serial << "> Type \"help\" to get command list";
}

void loop() {
  // put your main code here, to run repeatedly:
  checkConsole();   //Обязательно проверять консоль и реагировать на нее
}
