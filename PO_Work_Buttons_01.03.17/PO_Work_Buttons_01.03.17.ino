

#include <Bounce2.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
//#include <String.h>
#include <ModbusRtu.h>

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
#define MAIN_BCGRND 0xEF9D
#define RED 0xF800
#define WHITE 0xFFFF
#define BLACK 0x0000

#define UP_BUTTON 8
#define DOWN_BUTTON 7
#define SPEED_UP_BUTTON 6
#define SPEED_DOWN_BUTTON 5

Bounce UP = Bounce();
Bounce DOWN = Bounce();
Bounce SPEED_UP = Bounce();
Bounce SPEED_DOWN = Bounce();


int speed_first = 0;
int speed_second = 0;
int fill_value = 0;
int displacement_first = 0;
int displacement_second = 0;
int strenght_first = 0;
int strenght_second = 0;
int buf = 0;

long displacement_long = 0;
long speed_long = 0;
long speed_long_buf = 0;


float strenght_float = 0;
float displacement_float = 0;
float speed_float = 0;

unsigned long timer = 0;

union Pun {float f; uint32_t u;};

uint16_t au16data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


Modbus slave(2,0,2);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(SPEED_UP_BUTTON, INPUT_PULLUP);
  pinMode(SPEED_DOWN_BUTTON, INPUT_PULLUP);
    
  UP.attach(UP_BUTTON);                               //кнопка "вверх"
  UP.interval(5);
  DOWN.attach(DOWN_BUTTON);                           //кнопка "вниз"
  DOWN.interval(5);
  SPEED_UP.attach(SPEED_UP_BUTTON);                   //кнопка "увеличить скорость"
  SPEED_UP.interval(5);
  SPEED_DOWN.attach(SPEED_DOWN_BUTTON);               //кнопка "уменьшить скорость"
  SPEED_DOWN.interval(5);
  
  slave.begin( 19200 );
  
  tft.begin();
  tft.setRotation(2);                                 // поворот экрана
  tft.fillScreen(MAIN_BCGRND);                        // фон экрана
  
  tft.setCursor(40, 15);                              // отрисовка "Усилие, Н" и поля для его отображения
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.println(utf8rus("Усилие,Н"));
  tft.drawRect(10, 45, 150, 40, 0x0000); 
  tft.fillRect(11, 46, 148, 38, 0xFFFF); 
  tft.setCursor(50, 60);
  tft.println("000000");

  tft.setCursor(5, 115);                              // отрисовка "Перемещение, мм" и поля для его отображения
  tft.println(utf8rus("Перемещение,мм"));
  tft.drawRect(10, 145, 150, 40, 0x0000); 
  tft.fillRect(11, 146, 148, 38, 0xFFFF); 
  tft.setCursor(50, 160);
  tft.println("000.0000");

  tft.setCursor(5, 215);                              // отрисовка "Скорость, мм/мин." и поля для его отображения
  tft.println(utf8rus("Скорость,мм/мин"));
  tft.drawRect(10, 245, 150, 40, 0x0000); 
  tft.fillRect(11, 246, 148, 38, 0xFFFF); 
  tft.setCursor(70, 255);
  tft.println(speed_float);

  tft.drawTriangle(200,40,230,85,170,85,0x0000);      // отрисовка индикатора "движение вверх"
  tft.fillTriangle(200,41,228,84,172,84,0xFFFF); 
  tft.drawRect(188,85,24,60,0x0000); 
  tft.fillRect(189,85,22,59,0xFFFF); 
  
  tft.drawTriangle(200,280,230,235,170,235,0x0000);   // отрисовка индикатора "движение вниз"
  tft.fillTriangle(200,278,228,236,172,236,0xFFFF); 
  tft.drawRect(188,176,24,60,0x0000); 
  tft.fillRect(189,177,22,59,0xFFFF); 

}

void loop() {
  //----------принимаем данные с ПЛК -------//
  slave.poll( au16data, 16 );
  
 // ---------обрабатываем полученные данные--------//
  if (strenght_changed()){                                      // если изменилось значение силы
     tft.fillRect(30, 60, 120, 15, 0xFFFF);                       
     tft.setCursor(40, 60);
     tft.println(strenght_float,number(strenght_float));       // отрисовываем
  }
  if (displacement_changed()){                                  // если изменилось перемещение
      tft.fillRect(40, 160, 115, 15, 0xFFFF); 
      tft.setCursor(40, 160);
      tft.println(displacement_float,4);                        // отрисовываем
  }
  if (speed_changed_from_PLC()){                              // если с контроллера пришло новое значение скорости
    speed_update();                                           // обновляем
  }
  
  if(changed_moving_from_PLC()){                              // если есть движение с контроллера
    if ( (buf & 0x01)==1 ) {fill_upper_tri(RED);}             // показываем
    else {fill_upper_tri(WHITE);}
    
    if ( ( (buf >> 1) & 0x01 )==1 ){ fill_down_tri(RED);}
    else {fill_down_tri(WHITE);}
  }
//---------------------- обновляем и обрабатываем состояние кнопок --------//
  boolean UP_changed = UP.update();
  boolean DOWN_changed = DOWN.update();
  boolean SPEED_UP_changed = SPEED_UP.update();
  boolean SPEED_DOWN_changed = SPEED_DOWN.update();
  
  if (UP_changed){                                              //если изменено состояние кнопки "вверх"
      if(UP.read()==HIGH){                                         //если она нажата
          fill_upper_tri(RED);
          bitSet(au16data[0],0);
      }
  else{                                                            //если отпущена
          fill_upper_tri(WHITE);
          bitClear(au16data[0],0);
      }
  }
  if (DOWN_changed){                                            //если изменено состояние кнопки "вниз"
    if(DOWN.read()==HIGH){                                          //если она нажата
        fill_down_tri(RED);
        bitSet(au16data[0],1);
    }
    else{                                                           //если она отпущена
        fill_down_tri(WHITE);
        bitClear(au16data[0],1);
    }
  }
  if (SPEED_UP_changed){                                        //если изменено состояние кнопки "увеличить скорость"
    if (SPEED_UP.read() == HIGH){                                   //если она нажата
      speed_long = speed_long + 100000;                                   //увеличиваем скорость на 10
      if (speed_long <= 2500000){
        speed_update();
      }
      else{
        speed_long = 2500000; 
        speed_update();
      }   
    }
   }
  if (SPEED_DOWN_changed){                                      //если изменено состояние кнопки "уменшить скорость"
    if (SPEED_DOWN.read() == HIGH){                                  //если она нажата
       speed_long = speed_long - 100000;
       if (speed_long >= 0){
        speed_update();
       }
       else{
        speed_long = 0; 
        speed_update();
       }
     }
  }
 
}

int fill_upper_tri(int color){
  tft.fillTriangle(200,41,228,84,172,84,color);
  tft.fillRect(189,85,22,59, color);
}

int fill_down_tri(int color){
  tft.fillTriangle(200,278,228,236,172,236,color); 
  tft.fillRect(189,177,22,59,color); 
}

          
          
void speed_update(){                                            // обновление показаний скорости 

    au16data[2] = (speed_long >> 16) & 0xFFFFU;                //отправляем в [1]и[2] текущее значение
    au16data[1] = speed_long & 0xFFFFU;  
   
    speed_float = float(speed_long)/10000;                         
    fill_value = map(constrain(speed_float,0,250), 0,250 , 0, 148);   //расчет для красного ползунка
    tft.fillRect(30, 255, 115, 15, 0xFFFF);                            //очистка предыдущих показаний скорости
    tft.setCursor(30, 255);                                           //отрисовка новых
    tft.println(speed_float,4);
    tft.fillRect(11, 271, fill_value, 13, ILI9341_RED);               //отрисовка ползунка
    tft.fillRect(11+fill_value, 271, 148-fill_value, 13, 0xFFFF);
}

boolean strenght_changed(){                                     // проверка на изменение показаний силы
  if (strenght_first!=au16data[3]||strenght_second!=au16data[4]){   // проверка на изменение каждой ячейки
    strenght_second = au16data[4];
    strenght_first = au16data[3];
    strenght_float = decodeFloat(&au16data[3]);                          // раскодировка во float из 2 16 битных ячеек
    return true;
  }
  else{                                                              // если не изменилось возвр false
    return false;  
  }
}

boolean displacement_changed(){                                 // проверка на изменение показаний перемещения
   if (displacement_first != au16data[5] || displacement_second != au16data[6]){    // проверка на изменение каждой ячейки
       displacement_first = au16data[5];                                            
       displacement_second = au16data[6];
       displacement_long = decodeLong( &au16data[5]);                                // раскодировка в long из 2 16 битных ячеек
       displacement_float = float(displacement_long)/10000;                               // преобразование в формат 00.0000
       return true;
    }
  else{
       return false;
  }
 }
 boolean speed_changed_from_PLC(){                             // проверка на изменение скорости от контроллера
    if(speed_first != au16data[7] || speed_second != au16data[8]){
      speed_long = decodeLong(&au16data[7]);
      speed_first = au16data[7];
      speed_second = au16data[8];
      return true;
    }
    else{ return false; }
}

 boolean changed_moving_from_PLC(){
   if ( buf != au16data[9]){
    buf = au16data[9];
    return true;
   }
   else {return false;}
 }
 
 float decodeFloat(const uint16_t *regs){                         //раскодировка во float  из 2 16 битных ячеек
    union Pun pun;
    pun.u = ((uint32_t)regs[1] << 16) | regs[0];
    return pun.f;
}

 long decodeLong(const uint16_t *regs){                           // раскодировка в long из 2 16 битных ячеек
   long value_long = ((uint32_t)regs[1] << 16) | regs[0];
    return value_long;
 }
 
int number(float value){                                          // вычисление количества знаков после точки для отображения силы
  value = abs(value);
  int i =1;
   while (value>=10){
    value = value / 10;
    i= i+1;
   }
   return (6-i);
  }
  
 
 
String utf8rus(String source)                                     // ф-я для отрисовки русского алфавита
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break;Serial.println(n); }
          if (n >= 0x90 && n <= 0xBF) {n = n + 0x2F;Serial.println(n);} //0x30
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break;Serial.println(n); }
          if (n >= 0x80 && n <= 0x8F) {n = n + 0x6F;Serial.println(n);} //70
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}

