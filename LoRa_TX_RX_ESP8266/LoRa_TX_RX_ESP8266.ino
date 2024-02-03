/**
 * @file main.cpp
 * @author Ale-maker325 (deed30511@gmail.com)
 * 
 * @brief Пример работы с модулем SX127xx для ESP8266
 *  
 */

#include <Arduino.h>
#include <RadioLib.h>


//#define PING_PONG     //раскомментировать, если будет загружен пример пинг-понг нижний закомментировать
#define RECEIVER        //раскомментировать, если модуль будет использоваться как простой приёмник
//#define TRANSMITTER     //раскомментировать, если модуль будет использоваться как простой передатчик



#ifdef PING_PONG
  //Раскомментировать на одном из узлов для начала передачи друг-другу
  #define INITIATING_NODE

  //флаг, указывающий на состояние передачи или приема
  bool transmitFlag = false;
#endif




uint8_t LED_PIN = 16;             //Контакт управления светодиодом


//Сохранение состояния передачи между циклами
int transmissionState = RADIOLIB_ERR_NONE;

//Флаг окончания операции чтобы указать, что пакет был отправлен или получен
volatile bool operationDone = false;

// Эта функция вызывается, когда модуль передает или получает полный пакет
// ВАЖНО: эта функция ДОЛЖНА БЫТЬ 'пуста' типа и НЕ должна иметь никаких аргументов!
IRAM_ATTR void setFlag(void) {
// мы отправили или получили пакет, установите флаг
  operationDone = true;
}


//Счётчик для сохранения количества отправленных/полученных пакетов
uint64_t count = 0;




// Подключение радиотрансивера SX127.. к следующим пинам esp8266 (в нашем случае это SX1278 433MHz):
uint32_t NSS = SS;                // NSS pin:   15 - стандартный пин esp8266
uint32_t DIO_0 = D2;              
uint32_t RST = D4;                
uint32_t DIO_1 = D1;               

SX1278 radio1 = new Module(NSS, DIO_0, RST, DIO_1);

/**
 * @brief Структура для настройки параметров радиотрансивера
 * 
 */
struct LORA_CONFIGURATION
{
  float frequency = 434.0;        //Частота работы передатчика (по-умолчанию 434 MHz)
  float bandwidth = 125.0;        //Полоса пропускания (по-умолчанию 125 килогерц)
  uint8_t spreadingFactor = 9;   //Коэффициент расширения (по-умолчанию 9)
  uint8_t codingRate = 7;         //Скорость кодирования (по-умолчанию 7)
  uint8_t syncWord = 0x18;        //Слово синхронизации (по-умолчанию 0х18). ВНИМАНИЕ! Значение 0x34 зарезервировано для сетей LoRaWAN и нежелательно для использования
  int8_t outputPower = 10;        //Установить выходную мощность (по-умолчанию 10 дБм) (допустимый диапазон -3 - 17 дБм) ПРИМЕЧАНИЕ: значение 20 дБм позволяет работать на большой мощности, но передача рабочий цикл НЕ ДОЛЖЕН ПРЕВЫШАТЬ 1
  uint8_t currentLimit = 80;      //Установить предел защиты по току (по-умолчанию до 80 мА) (допустимый диапазон 45 - 240 мА) ПРИМЕЧАНИЕ: установить значение 0 для отключения защиты от перегрузки по току
  int16_t preambleLength = 8;    //Установить длину преамбулы (по-умолчанию в 8 символов) (допустимый диапазон 6 - 65535)
  uint8_t gain = 0;               //Установить регулировку усилителя (по-умолчанию 1) (допустимый диапазон 1 - 6, где 1 - максимальный рост) ПРИМЕЧАНИЕ: установить значение 0, чтобы включить автоматическую регулировку усиления оставьте в 0, если вы не знаете, что вы делаете

};

//Экземпляр структуры для настройки параметров радиотрансивера 1
LORA_CONFIGURATION config_radio1;







/**
 * @brief Функция установки настроек передатчика
 * 
 * @param radio - экземпляр класса передатчика
 * @param config - экземпляр структуры для настройки модуля
 */
void radio_setSettings(SX1278 radio)
{
  Serial.println(F("Set LoRa settings..."));

  // Устанавливаем необходимую нам частоту работы трансивера
  if (radio.setFrequency(config_radio1.frequency) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set frequency = "));
  Serial.println(config_radio1.frequency);


  // установить полосу пропускания до 250 кГц
  if (radio.setBandwidth(config_radio1.bandwidth) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set bandWidth = "));
  Serial.println(config_radio1.bandwidth);

  // коэффициент расширения 
  if (radio.setSpreadingFactor(config_radio1.spreadingFactor) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set spreadingFactor = "));
  Serial.println(config_radio1.spreadingFactor);

  // установить скорость кодирования
  if (radio.setCodingRate(config_radio1.codingRate) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set codingRate = "));
  Serial.println(config_radio1.codingRate);

  // Устанавливаем слово синхронизации
  if (radio.setSyncWord(config_radio1.syncWord) != RADIOLIB_ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
    while (true);
  }
  Serial.print(F("Set syncWord = "));
  Serial.println(config_radio1.syncWord);

  // Устанавливаем выходную мощность трансивера
  if (radio.setOutputPower(config_radio1.outputPower) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set setOutputPower = "));
  Serial.println(config_radio1.outputPower); 

  // установить предел защиты по току (допустимый диапазон 45 - 240 мА)
  // ПРИМЕЧАНИЕ: установить значение 0 для отключения защиты от перегрузки по току
  if (radio.setCurrentLimit(config_radio1.currentLimit) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Selected current limit is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set currentLimit = "));
  Serial.println(config_radio1.currentLimit);

  // установить длину преамбулы (допустимый диапазон 6 - 65535)
  if (radio.setPreambleLength(config_radio1.preambleLength) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set preambleLength = "));
  Serial.println(config_radio1.preambleLength);

  // Установить регулировку усилителя (допустимый диапазон 1 - 6, где 1 - максимальный рост)
  // ПРИМЕЧАНИЕ: установить значение 0, чтобы включить автоматическую регулировку усиления
  //   оставьте в 0, если вы не знаете, что вы делаете
  if (radio.setGain(config_radio1.gain) == RADIOLIB_ERR_INVALID_GAIN) {
    Serial.println(F("Selected gain is invalid for this module!"));
    while (true);
  }
  Serial.print(F("Set Gain = "));
  Serial.println(config_radio1.gain);

  Serial.println(F("All settings successfully changed!"));
}





void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);      //Контакт управления светодиодом
    
  //Задаём параметры конфигурации радиотрансивера 1
  config_radio1.frequency = 433;
  config_radio1.bandwidth = 125;
  config_radio1.spreadingFactor = 9;
  config_radio1.codingRate = 7;
  config_radio1.syncWord = 0x12;
  config_radio1.outputPower = 2;
  config_radio1.currentLimit = 100;
  config_radio1.preambleLength = 8;
  config_radio1.gain = 0;
  
  //Инициализируем радиотрансивер со значениями по-умолчанию
  Serial.println(" ");
  Serial.print(F("[SX127x] Initializing ... "));
  int state = radio1.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("SUCCES!"));
  } else {
    Serial.print(F("ERROR!  "));
    Serial.println(state);
    while (true);
  }
  
  
  //Устанавливаем наши значения, определённые ранее в структуре config_radio1
  radio_setSettings(radio1);


  #ifdef PING_PONG

    // задать функцию, которая будет вызываться при получении/отправке пакета
    radio1.setDio0Action(setFlag, RISING);
    
    //Если определено, что узел стартует передачу
    #if defined(INITIATING_NODE)
      //посылаем первый пакет на этом узле
      Serial.print(F("[SX1278] Sending first packet ... "));
      transmissionState = radio1.startTransmit("Hello World!");
      transmitFlag = true;
      digitalWrite(LED_PIN, LOW);     //Включаем светодиод, сигнализация об передаче/приёма пакета
      delay(2000);
    #else
      //Иначе, если определено, что узел ждёт пока передачу стартанёт другой узел
      //Начать прослушивание пакетов LoRa на этом узле
      Serial.print(F("[SX1278] Starting to listen ... "));
      state = radio1.startReceive();
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
      } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
      }
    #endif
  #endif

  #ifdef RECEIVER
    //Устанавливаем функцию, которая будет вызываться при получении пакета
    radio1.setPacketReceivedAction(setFlag);

    // start listening for LoRa packets
    Serial.print(F("[SX1278] Starting to listen ... "));

    transmissionState = radio1.startReceive();

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    }else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true);
    }
      
  #endif

  #ifdef TRANSMITTER
    //Устанавливаем функцию, которая будет вызываться при отправке пакета
    radio1.setPacketSentAction(setFlag);
    //Начинаем передачу пакетов
    Serial.println(F("Sending first packet ... "));
    String str = F("START!");
    state = radio1.startTransmit(str);
    //Если передача успешна, выводим сообщение в сериал-монитор
  if (state == RADIOLIB_ERR_NONE) {
    //Выводим сообщение об успешной передаче
    Serial.println(F(".................................................."));
    Serial.println(F("transmission finished succes!"));
                
    //Выводим в сериал данные отправленного пакета
    Serial.print(F("Data:\t\t"));
    Serial.println(str);

    //Печатаем RSSI (Received Signal Strength Indicator)
    float rssi_data = radio1.getRSSI();
    Serial.print(F("RSSI: \t\t"));
    Serial.println(rssi_data);
          
    digitalWrite(LED_PIN, LOW);     //Включаем светодиод, сигнализация об передаче/приёма пакета
    delay(1000);

  } else {
    //Если были проблемы при передаче, сообщаем об этом
    Serial.print(F("transmission failed, code = "));
    Serial.println(state);
  }
  #endif

  digitalWrite(LED_PIN, HIGH);      //Выключаем светодиод, сигнализация об окончании передачи/приёма пакета
  
  Serial.println(" ");
}




void loop() {

  
  //Если определён пример пинг-понг
  #ifdef PING_PONG
    //Если предыдущая операция была окончена
    if(operationDone) {
      //Сбрасываем флаг окончания операции
      operationDone = false;

      //Если предыдущая операция была передачей, ждём ответа от другого модуля
      if(transmitFlag) {
        if (transmissionState == RADIOLIB_ERR_NONE) {
          //Если предыдущая передача была успешной, выводим сообщение
          Serial.println(F("transmission finished!"));
          digitalWrite(LED_PIN, LOW);      //Включаем светодиод, сигнализация о передаче пакета

        } else {
          Serial.print(F("failed, code "));
          Serial.println(transmissionState);

        }

        //Переходим в режим ожидания ответа от другого модуля
        radio1.startReceive();
        //Не разрешаем передачу. Мы в режиме приёма
        transmitFlag = false;
        delay(400);
        digitalWrite(LED_PIN, HIGH);      //Выключаем светодиод

      } else {
        //Предыдущая операция была приёмом, выводим полученные данные на печать
        //в монитор порта и посылаем ответ
        
        String str;
        int state = radio1.readData(str);

        if (state == RADIOLIB_ERR_NONE) {
          // packet was successfully received
          Serial.println(F("[SX1278] Received packet!"));

          // print data of the packet
          Serial.print(F("[SX1278] Data:\t\t"));
          Serial.println(str);

          // print RSSI (Received Signal Strength Indicator)
          Serial.print(F("[SX1278] RSSI:\t\t"));
          Serial.print(radio1.getRSSI());
          Serial.println(F(" dBm"));

          // print SNR (Signal-to-Noise Ratio)
          Serial.print(F("[SX1278] SNR:\t\t"));
          Serial.print(radio1.getSNR());
          Serial.println(F(" dB"));

          digitalWrite(LED_PIN, LOW);      //Включаем светодиод, сигнализация о приёме пакета

        }

        // Ждём секунду чтобы отправить передачу в ответ на полученное собщение
        delay(1000);
        digitalWrite(LED_PIN, HIGH);      //Выключаем светодиод

        //Переходим в режим передачи на другой узел
        Serial.print(F("[SX1278] Sending another packet ... "));
        transmissionState = radio1.startTransmit("Hello World!");
        transmitFlag = true;
      }
    }
  #endif


  #ifdef RECEIVER
    //проверяем, была ли предыдущая передача успешной
    if(operationDone) {
      //Сбарсываем флаг
      operationDone = false;

      //Получаем данные
      String str;
      int state = radio1.readData(str);

      //Если данные успешно получены
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("[SX1278] Received packet!"));

        // print data of the packet
        Serial.print(F("[SX1278] Data:\t\t"));
        Serial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[SX1278] RSSI:\t\t"));
        Serial.print(radio1.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[SX1278] SNR:\t\t"));
        Serial.print(radio1.getSNR());
        Serial.println(F(" dB"));

        // print frequency error
        Serial.print(F("[SX1278] Frequency error:\t"));
        Serial.print(radio1.getFrequencyError());
        Serial.println(F(" Hz"));

        Serial.println(F(".................................................."));

        digitalWrite(LED_PIN, HIGH);    //сигнализируем светодиодом

      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        Serial.println(F("[SX1278] CRC error!"));

      } else {
        // some other error occurred
        Serial.print(F("[SX1278] Failed, code "));
        Serial.println(state);

      }

      //Делаем задержку 1с
      delay(2000);
      digitalWrite(LED_PIN, LOW);       //сигнализируем светодиодом
    }
  #endif

  #ifdef TRANSMITTER
    //проверяем, была ли предыдущая передача успешной
    Serial.println(F(".................................................."));
    if(operationDone) {
      
      //Сбрасываем сработавший флаг прерывания
      operationDone = false;

      //готовим строку для отправки
      String str = "#" + String(count++);
      int state = radio1.startTransmit(str);

      //Если передача успешна, выводим сообщение в сериал-монитор
      if (state == RADIOLIB_ERR_NONE) {
        //Выводим сообщение об успешной передаче
        Serial.println(F("transmission finished succes!"));
                
        //Выводим в сериал данные отправленного пакета
        Serial.print(F("Data:\t\t"));
        Serial.println(str);

        //Печатаем RSSI (Received Signal Strength Indicator)
        float rssi_data = radio1.getRSSI();
        Serial.print(F("RSSI: \t\t"));
        Serial.println(rssi_data);
          
        digitalWrite(LED_PIN, HIGH);    //сигнализируем светодиодом

      } else {
        //Если были проблемы при передаче, сообщаем об этом
        Serial.print(F("transmission failed, code = "));
        Serial.println(state);
      }
      
      //Делаем задержку 1с
      delay(2000);
      digitalWrite(LED_PIN, LOW);       //сигнализируем светодиодом
      
    }

  #endif

  
}
