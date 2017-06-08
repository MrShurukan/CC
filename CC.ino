#include <memorysaver.h>
#include <UTFT.h>
#include "RussianFontsRequiredFunctions.h"

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

UTFT tft(CTE32HR, 38, 39, 40, 41);  //Создаем объект tft с такими выводами (дефолт) и такой моделью

void setup() {
  // put your setup code here, to run once:
  tft.InitLCD(LANDSCAPE);
  tft.clrScr();
  tft.setColor(VGA_BLACK);
  tft.fillRect(0,0, tft.getDisplayXSize()-1, tft.getDisplayYSize()-1);
  tft.setFont(BigRusFont);
  tft.setColor(VGA_WHITE);
  tft.setBackColor(VGA_TRANSPARENT);
  printRus(tft,"АБВГДЕЁЖЗИЙКЛМН", 0, 0,0);
 
  printRus(tft,"ОПРСТУФХШЩЬЫЪЭ", 0, 20,0); 
  printRus(tft,"ЮЯабвгдеёжзийкл", 0, 40,0);
  printRus(tft,"мнопрстуфхцчшщь", 0, 60,0);
  printRus(tft,"ыъэюя", 0, 80,0);
  tft.setFont(SmallRusFont);
  printRus(tft,"АБВГДЕЁЖЗИЙКЛМН", 0, 100,0);
 
  printRus(tft,"ОПРСТУФХШЩЬЫЪЭ", 0, 120,0); 
  printRus(tft,"ЮЯабвгдеёжзийкл", 0, 140,0);
  printRus(tft,"мнопрстуфхцчшщь", 0, 160,0);
  printRus(tft,"ыъэюя123456", 0, 180,0);
}

void loop() {
  // put your main code here, to run repeatedly:

}
