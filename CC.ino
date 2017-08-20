#include <Streaming.h>

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


#include "RussianFontsRequiredFunctions.h"

String V = "3.1-alpha";

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

String months[12] = {                                                                   //Соответсвия названия месяца к его номеру
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec"
};

bool setFontByName(String name) {    //Функция для возможности устанавливать шрифт через консоль
  if (name == "SmallRusFont") tft.setFont(SmallRusFont);
  else if (name == "BigRusFont") tft.setFont(BigRusFont);
  else if (name == "SevenSegNumFontMDS") tft.setFont(SevenSegNumFontMDS);
  else if (name == "Grotesk16x32") tft.setFont(Grotesk16x32);
  else return false;
  return true;
}

String consoleMsg = "", serialMsg = "";

bool consoleHooked = false;   //Если консоль "подцеплена", то весь ее output перенаправляется в ESP вместо Serial и vice versa

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

    if (!consoleHooked) log("Выполнение команды: " + consoleMsg);
    else log("Выполнение команды (с моб. устройства): " + consoleMsg);
  }

  if (consoleMsg.indexOf(' ') != -1) {    //Если команда - это несколько слов

    String firstWord = consoleMsg.substring(0, consoleMsg.indexOf(' ')); //Первое слово

    if (firstWord != "log" && firstWord != "printLog") {     //Не нужно писать "Выполнение команды", если ты сам выводишь свое сообщение в LOG или хочешь его вывести
      if (!consoleHooked) log("Выполнение команды: " + consoleMsg);
      else log("Выполнение команды (с моб. устройства): " + consoleMsg);
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
      console("\n         selectFont #FontName# - выбрать шрифт FontName (DEBUG)\n", RAW);
      delay(delayTime);
      console("\n            drawText #Text,X,Y# - вывести Text на (X,Y) (DEBUG)\n", RAW);

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

    serialMsg = "";
  }
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

    if (hour() == 0 && minute() == 0 && second() == 0) {      //Если это новый день, то
      myFile.close();
      openLogFile(FILE_WRITE);   //Открываем новый файл для записи лога
      log("Привет, сегодня новый день!", WITH_SERIAL);
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

  Serial << "\nФункция Setup завершена!\n\n";
  console("Введите \"help\" для списка команд");
}

void loop() {
  // put your main code here, to run repeatedly:
  checkConsole();     //Обязательно проверять консоль и реагировать на нее
  checkESPInput();    //Не забудем и про ввод с приложения


  updateTime();       //Обработка событий по времени
}
