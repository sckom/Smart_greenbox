
// Управляющая программа автоматизированной системы управления тепличным комплексом

#include <Wire.h>             // Библиотека для предачи данных с датчиков по I2C интерфейсу
#include <Adafruit_Sensor.h>  // Библиотека для датчика ???

/*
#include <ESP32Servo.h>
Servo myservo;
int pos = 0;
*/

// Библиотека для управления светодиодным модулем
#include "TLC59108.h"
// Назначение выхода для программнго сброса модуля светодиода
#define HW_RESET_PIN 2
// Назначение переменной для управления светодиодным модулем и назначение I2C интерфейса
#define I2C_ADDR TLC59108::I2C_ADDR::BASE
TLC59108 leds(I2C_ADDR);

// Библиотека для датчика температуры/влажности воздуха и давления
#include <Adafruit_BME280.h>
// Назначение переменной для управления датчиком
Adafruit_BME280 bme280;

// Библиотека для датчика температуры/влажности почвы
#include <BH1750FVI.h>
// Назначение переменной для управления датчиком
BH1750FVI LightSensor_1;

// Библиотека для управления модулем реле
#include "PCA9536.h"
PCA9536 pca9536;

// Датчик почвы подключен к портам (А4/А5)
#define SOIL_MOISTURE    34
#define SOIL_TEMPERATURE 35

#define I2C_HUB_ADDR        0x70 // настройки I2C для платы MGB-I2C63EN
#define EN_MASK             0x08
#define DEF_CHANNEL         0x00
#define MAX_CHANNEL         0x08

// Датчик влажности почвы емкостной
const float air_value    = 1587.0;
const float water_value  = 800.0;
const float moisture_0   = 0.0;
const float moisture_100 = 100.0;

void setup() {
  // Запуск проводного подключение
  Wire.begin();
  // Настройка скорости обмена данными по последовательному интерфейсу
  Serial.begin(115200);
  
  leds.init(HW_RESET_PIN);
  leds.setLedOutputMode(TLC59108::LED_MODE::PWM_IND);

  // Условие для проверки работоспособности датчика
  bool bme_status = bme280.begin();
  // Проверка работы датчика температуры/влыжности воздуха
  if (!bme_status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");// вывод сообщения об ошибке подключения
  }

  // Запуск датчика освещенности
  LightSensor_1.begin();
  //настройка датчика на режим высокой точности
  LightSensor_1.setMode(Continuously_High_Resolution_Mode);
  
  // Задание GPIO для сервопривода
  //myservo.attach(4);

  // Программный сброс модуля реле
  pca9536.reset();
  // Настройка работы модуля реле
  pca9536.setMode(IO_OUTPUT);
  // Задание начального состояния каждого выхода реле
  pca9536.setState(IO0, IO_LOW);
  pca9536.setState(IO1, IO_LOW);
  pca9536.setState(IO2, IO_LOW);
  pca9536.setState(IO3, IO_LOW);
}

void loop() {
  // Задание rgb параметров для индикатора состояния работы системы
  byte pwm_r = 0x56;
  byte pwm_b = 0xb5;
  byte pwm_g = 0xae;
  // Задание значений для работы светодиодов по ШИМ
  byte pwm_off = 0;
  byte pwm_on = 0x0f;
  
  /*
  pca9536.setState(IO2, IO_LOW);
  pca9536.setState(IO3, IO_LOW);
  */

  float t = bme280.readTemperature(); // Привязываем показания температуры к переменной t
  float h = bme280.readHumidity(); // Привязываем показания влажности к переменной h
  float p = bme280.readPressure() / 100.0F; // Привязываем показания давления к переменной p
  float l = LightSensor_1.getAmbientLight(); // загрузить измеренное осещенности значение в переменную “l" типа float

  // Считывание показаний с аналогового выхода, к которому подключен датчик темпратуры/влажности почвы
  float adc0 = analogRead(SOIL_MOISTURE); // Считывание аналогово значения влажности
  float adc1 = analogRead(SOIL_TEMPERATURE); // Считывание аналогово значения температуры
  float t1 = ((adc1 / 4095.0 * 5.0) - 0.3) * 100.0; // Перевод аналогового значения температуры в цифровое
  float h1 = map(adc0, air_value, water_value, moisture_0, moisture_100); // Перевод аналогового значения влажности в цифровое
  delay(250);

  // Вывод занчений датчика температуры, влажности, давления воздуха
  Serial.println("_______________");
  Serial.println("Air:");
  Serial.println(-------);
  Serial.println("Air temperature = " + String(t, 1) + " *C"); // Вывод показаний датчика температуры воздуха
  Serial.println("Air humidity = " + String(h, 1) + " %");  // Вывод показаний датчика влажности воздуха
  Serial.println("Air pressure = " + String(p, 1) + " hPa");  // Вывод показаний давления
  
  // Вывод занчений датчика освещенности
  Serial.println("_______________");
  Serial.println("Light:");
  Serial.println(-------);
  Serial.println("Освещенность: " + String(l, 1) + " Лк"); // Вывод значения освещенности в люксах в последовательный порт

  // Вывод занчений датчика теппературы, влажности почвы
  Serial.println("_______________");
  Serial.println("Gound:");
  Serial.println(-------);
  Serial.print("Soil temperature = ");
  Serial.println(String(t1,1)+" C"); // Вывод показаний температуры почвы
  Serial.print("Soil moisture = ");
  Serial.println(String(h1,1)+" %"); // Вывод показаний влажности почвы

  /*
  Cветовая индикация rgb для отображения состояние "благоприятности"
  микроклимата для растения в теплице
  */
  leds.setBrightness(2, pwm_g);
  leds.setBrightness(3, pwm_r);
  leds.setBrightness(5, pwm_b);

  // Условие, что влажность почвы будет меньше 40%
  if (h1 < 40) {
    pca9536.setState(IO0, IO_HIGH); // Включение первого выхода реле, фитолампа
    // Выключение RGB светодиода
    leds.setBrightness(2, pwm_off);
    leds.setBrightness(3, pwm_off);
    leds.setBrightness(5, pwm_off);
  }
  else {
    pca9536.setState(IO0, IO_LOW); // Выключение первого выхода реле, фитолампа
  }

  // Условие, что освещённость будет меньше 250 лк (люксы)
  if (l < 250) {
    pca9536.setState(IO1, IO_HIGH); // Включение второго выхода реле, помпа
    // Выключение RGB светодиода
    leds.setBrightness(2, pwm_off);
    leds.setBrightness(3, pwm_off);
    leds.setBrightness(5, pwm_off);
  }
  else {
    pca9536.setState(IO1, IO_LOW); // Выключение второго выхода реле, помпа
  }

  /*
  if (h1 > 40) {
    for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
        myservo.write(posDegrees);
        delay(20);
    }
    Serial.println("Servo complited");
  }
  */

}
