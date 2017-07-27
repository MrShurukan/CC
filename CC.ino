#include <Streaming.h>

#include <memorysaver.h>
#include <UTFT.h>
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
extern uint8_t ArialNumFontPlus[];

UTFT tft(CTE32HR, 38, 39, 40, 41);  //Создаем объект tft с такими выводами (дефолт) и такой моделью

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  tft.InitLCD(LANDSCAPE);
  tft.clrScr();
  tft.setColor(VGA_GRAY);
  tft.fillRect(0,0, tft.getDisplayXSize()-1, tft.getDisplayYSize()-1);
  tft.setFont(SmallRusFont);
  tft.setColor(VGA_BLACK);
  tft.setBackColor(VGA_TRANSPARENT);
  printRus(tft, String("Управление котлами (версия ") + V + String(")"), CENTER, 2);
}

void loop() {
  // put your main code here, to run repeatedly:

}
