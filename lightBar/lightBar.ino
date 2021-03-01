//определение пинов радиомодуля
#define BTN_PIN 8       // при любом нажатии кнопок пульта
#define BTN_A 10        // пин кнопки А
#define BTN_B 12        // пин кнопки B
#define BTN_C 9         // пин кнопки C
#define BTN_D 11        // пин кнопки D

// Создаем объекты кнопок 
#include "GyverButton.h"
GButton buttClick(BTN_PIN);
GButton bA(BTN_A);
GButton bB(BTN_B);
GButton bC(BTN_C);
GButton bD(BTN_D);

// Настройки светодиодной ленты
#define STRIP_PIN 6     // пин ленты
#define NUMLEDS 119      // кол-во светодиодов
#define COLOR_DEBTH 3   // цветовая глубина: 1, 2, 3 (в байтах)

// Подключение библиотеки для работы с лентой
#include <microLED.h>

// Создание объекта ленты
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2818, ORDER_GRB, CLI_AVER> strip;

// массив цветов для первого режима
uint32_t colorsLight[] = {
  0xF6D1A4, // сливочно-белый
  0xFF0000, // красный
  0x0000FF, // синий
  0x00FF00, // зелёный
  0x00FFFF, // голубой
  0xA000FF, // фиолетовый
  0xFF9B1C, // желтый
  0x19FACD, // мятный
  0xFC0FC0, // розовый
};

#include <FastLEDsupport.h>  // Библиотека для работы режима пламени
mGradient<5> myGrad;         // Объект массива градиента пламени (красный/желтый)
mGradient<4> myGrad2;       // Объект массива градиента пламени (фиолетовый/зеленый)

#define HUE_GAP 21      // заброс по hue
#define FIRE_STEP 10    // шаг огня
#define HUE_START 0     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define MIN_BRIGHT 70   // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 190     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

// Подключение библиотеки для работы с памятью
#include <EEPROM.h>     

// переменные
byte modeSelected = EEPROM.read(0); // Режим работы
byte brightness = EEPROM.read(1);   // Яркость
byte colorSelected = EEPROM.read(2); // Выбранный цвет для режима 1
byte tempSelected = EEPROM.read(3); // Выбранная температура свечения
byte rainbowSpeed = EEPROM.read(4); // Скорость радуги
byte flameSpeed = EEPROM.read(5); // Скорость пламени
byte flashSpeed = EEPROM.read(6); // Скорость изменения цвета

long lastSave;           // Время последнего сохранения
bool isChanged = false;   // Было ли изменение
int tempK;              // Температура в Кельвинах
int countFlame;     // Пламя
int counter2 = 0;

#define FOR_i(from, to) for(int i = (from); i < (to); i++)
#define FOR_j(from, to) for(int j = (from); j < (to); j++)
void setup() {
  Serial.begin(9600);   // Подключаем сериал порт
  // указываем подтяжку кнопок LOW_PULL - подтяжка на корпус, HIGH_PULL - к питанию
  buttClick.setType(LOW_PULL);
  bA.setType(LOW_PULL);
  bB.setType(LOW_PULL);
  bC.setType(LOW_PULL);
  bD.setType(LOW_PULL);
  // устанавливаем защиту от дребезка кнопок
  buttClick.setDebounce(20);
  bA.setDebounce(20);
  bB.setDebounce(20);
  bC.setDebounce(20);
  bD.setDebounce(20);

  lastSave = millis(); // инициализируем время последнего сохранения

// необходимый градиент цветов пламя (красный/желтый)
  myGrad.colors[0] = mBlack;
  myGrad.colors[1] = 0xFF0000;
  myGrad.colors[2] = mYellow;
  myGrad.colors[3] = 0xFE9A2E;
  myGrad.colors[4] = 0xF3F781;

// необходимый градиент цветов пламя (зеленый/фиолетовый)
  myGrad2.colors[0] = mBlack;
  myGrad2.colors[1] = mMagenta;
  myGrad2.colors[2] = mGreen;
  myGrad2.colors[3] = mWhite;
}

void loop() {
// опрос состояния кнопок при каждом цикле
  buttClick.tick();
  bA.tick();
  bB.tick();
  bC.tick();
  bD.tick();

  if (isChanged) SaveData(); // если была нажата кнопка - сохраняем данные
  
  switch(modeSelected) {    // выбор режима свечения
    case 0: 
      Mode1();  // цвет из массива
      break;
    case 1: 
      Mode2();  // цвет по температуре
      break;
    case 2:
      Mode3();  // радуга
      break;
    case 3:
      Mode4();  // мерцание
      break;
    case 4:
      Mode5();  // пламя (красный/желтый)
      break;
    case 5:
      Mode6();  // пламя (фиолетовый/зеленый)
      break;
    case 6:
      Mode7();
      break;
  }

// если зажата кнопка А на пульте, повышаем яркость до максимума - 255
  if (bA.isPress()) {    
    if ((255 - brightness) > 50) brightness += 50;
    else brightness = 255;
    isChanged = true;
    Serial.print("A Brightness: "); Serial.println(brightness);
  }
// если зажата кнопка B на пульте, меняем режим свечения (1-6)
  if (bB.isPress()) {
    modeSelected++;
    if (modeSelected > 6) modeSelected = 0;
    isChanged = true;
    Serial.print("B Mode: "); Serial.println(modeSelected);
  }
// если зажата кнопка C на пульте, понижаем яркость до минимума - 0
  if (bC.isPress()) {
    if ((brightness - 50) > 0) brightness -= 50;
    else brightness = 0;
    isChanged = true;
    Serial.print("C Brightness: "); Serial.println(brightness);
  }
// если зажата кнопка D на пульте, в зависимости от выбранного режима меняем настройки
  if (bD.isPress()) {
    switch(modeSelected) {
      case 0: 
        ColorSet();    // изменяем цвет из массива
        break;
      case 1: 
        TempSet();     // изменяем температуру свечения
        break;
      case 2:
        RainbowSpeedSet();  // изменяем скорость движения радуги
        break;
      case 3:
        FlashSpeedSet();    // изменяем скорость изменения цветов
        break;
      case 4:
        FlameSpeedSet();   // изменяем скорость пламя
        break;
      case 5:
        FlameSpeedSet();   // изменяем скорость пламя
        break;
    }
    isChanged = true;
  }
// если была нажата любая кнопка - меняем фалг сохранения данных
//  if (buttClick.isClick()) {
//    Serial.print("Changes throught : "); Serial.println((millis() - lastSave)/1000);
//    isChanged = true;
//  }


 
  
}

void Mode1() {  // цвет из массива
  strip.setBrightness(brightness);
  strip.clear();
  strip.fill(colorsLight[colorSelected]);  // заливаем цветом
  strip.show();         // выводим изменения
  delay(1);
}

void Mode2() { // цвет по тепловой шкале
  strip.setBrightness(brightness);
  strip.clear();
  tempK = map(tempSelected, 0, 255, 2050, 3750);
  for (int i = 0; i < NUMLEDS; i++) {
    strip.set(i, mKelvin(tempK));
  }
  strip.show();
  delay(10);
}

void Mode3() { // бегущая радуга
  strip.setBrightness(brightness);
  strip.clear();
  static byte counter = 0;
  byte speedR = map(rainbowSpeed, 0, 255, 0, 15);
  for (int i = 0; i < NUMLEDS; i++) {
    strip.set(i, mWheel8(counter + i * 255 / NUMLEDS));   // counter смещает цвет
  }
  counter += speedR;   // counter имеет тип byte и при достижении 255 сбросится в 0
  strip.show();
  delay(100);
}

void Mode4() {  // мерцание
  strip.setBrightness(brightness);
  strip.clear();
  static byte counter = 0;
  strip.fill(mWheel8(counter));
  counter += 1;
  strip.show();
  int delayTime = map(flashSpeed, 0, 255, 5, 100);
  delay(delayTime);
}

void Mode5() {  // пламя
  strip.setBrightness(brightness);
  strip.clear();
  for (int i = 0; i < NUMLEDS; i++) {
    strip.leds[i] = myGrad.get(inoise8(i * 20, countFlame), 255);
  }
  byte speedF = map(flameSpeed, 0, 255, 5, 30);
    countFlame += speedF;
    strip.show();
    delay(40);
  
}
void Mode6() {  // пламя
  strip.setBrightness(brightness);
  strip.clear();
  for (int i = 0; i < NUMLEDS; i++) {
    strip.leds[i] = myGrad2.get(inoise8(i * 25, countFlame), 255);
  }
  byte speedF = map(flameSpeed, 0, 255, 5, 30);
    countFlame += speedF;
    strip.show();
    delay(40);
}

void Mode7() {
  static uint32_t prevTime;
  if (millis() - prevTime > 50) {
    prevTime = millis();
    int thisPos = 0, lastPos = 0;
    FOR_i(0, NUMLEDS) {
      int val = (inoise8(i * FIRE_STEP, counter2)); 
      strip.leds[i] = mHSV(
           HUE_START + map(val, 0, 255, 0, HUE_GAP),                    // H
           constrain(map(val, 0, 255, MAX_SAT, MIN_SAT), 0, 255),       // S
           //constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255)  // V
           brightness
         );
    }
    counter2 += 20;
    strip.show();
  }
}

void ColorSet(){
  colorSelected++;
  if (colorSelected > 8) colorSelected = 0;
  Serial.print("D ColorSelected: "); Serial.println(colorSelected);
}

void TempSet(){
  if ((255 - tempSelected) > 10) tempSelected += 10;
    else tempSelected = 0;
  Serial.print("D TempSelected: "); Serial.print("tempSelected="); Serial.print(tempSelected);
  Serial.print(" | tempK=");Serial.println(tempK);
}

void RainbowSpeedSet() {
  if ((255 - rainbowSpeed) > 50) rainbowSpeed += 50;
  else rainbowSpeed = 0;
  Serial.print("D RainbowSpeed: "); Serial.println(rainbowSpeed);
}

void FlashSpeedSet() {
  if ((255 - flashSpeed) > 25) flashSpeed += 25;
  else flashSpeed = 0;
  Serial.print("D FlashSpeed: "); Serial.println(flashSpeed);
}

void FlameSpeedSet() {
  if ((255 - flameSpeed) > 50) flameSpeed += 50;
  else flameSpeed = 0;
  Serial.print("D FlameSpeed: "); Serial.println(flameSpeed);
}
void SaveData() {
  if ((millis() - lastSave) > 10000) {    // если с последнего сохранения прошло больше 10 секунд
    EEPROM.write(0, modeSelected);
    delay(1);
    EEPROM.write(1, brightness);
    delay(1);
    EEPROM.write(2, colorSelected);
    delay(1);
    EEPROM.write(3, tempSelected);
    delay(1);
    EEPROM.write(4, rainbowSpeed);
    delay(1);
    EEPROM.write(5, flameSpeed);
    delay(1);
    EEPROM.write(6, flashSpeed);
    lastSave = millis();
    Serial.println("Saved!");
    isChanged = false;
  }
}
