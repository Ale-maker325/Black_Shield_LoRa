/**
 * @file main.cpp
 * @author Ale-maker325 (deed30511@gmail.com)
 * 
 * @brief Пример работы с модулем SX127xx для ESP32_S2 mini. Пример основан на примерах библиотек Adafruit SSD1306 и RadioLib,
 * и рассчитан для применения с дисплеем OLED SSD1306.
 * 
 * Как пользоваться:
 * 
 *  - Для начала необходимо определиться с пинами Rx-Tx для сериал-монитора. Для этого нужно указать пины в переменных
 *    RX_pin, TX_pin. Доступны следующие пины: 40, 39 (выведен на плате), 38, 37 (выведен на плате), 36, 34, 21, 17, 15,
 *    1, 2, 3, 4, 6, 8, 10, 13, 14. По-умолчанию пример использует пины 37,39, которые выведены на плате в виде контактов
 *    RX, TX. Монитор Serial в методе setup() инициализируется со скоростью 115200;
 * 
 * -  Необходимо задать пины линии I2C. Для этого нужно указать пины в переменных SDA_display, SCL_display.
 *    Доступны следующие пины: 40, 38, 36, 34, 21, 17, 15, 1, 2, 3, 4, 6, 8, 10, 13, 14. По-умолчанию
 *    пример использует пины 13, 14;
 * 
 *  - Если необходимо узнать адрес устройства I2С, необходимо раскомментировать #define use_i2c_scanner, а #define use_adafruit_library
 *    наоборот, закомментировать. Подключить устройство I2С к назначенным пинам SDA_display, SCL_display и четез сериал-монитор считать
 *    адрес устройства.
 * 
 *  - Для работы передатчика нужно также определиться, будет он работать как передатчик, либо как приёмник. Для этого необходимо
 *    раскомментировать один из дефайнов: #define RECEIVER или #define TRANSMITTER, а второй закомментировать.
 *  
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define use_adafruit_library    //раскомментировать если будет использована библиотека Adafruit для экрана SSD1306 к примеру
//#define use_i2c_scanner       //раскомментировать, если будет загружен I2C-сканер

//#define RECEIVER                //раскомментировать, если модуль будет использоваться как простой приёмник
#define TRANSMITTER             //раскомментировать, если модуль будет использоваться как простой передатчик

#ifdef use_adafruit_library

  int8_t RX_pin = 37;                   //Пин Rx сериал-монитора
  int8_t TX_pin = 39;                   //Пин Tx сериал-монитора
  
  uint8_t SDA_display = 13;             //пин SDA линии I2C
  uint8_t SCL_display = 14;             //пин SCL линии I2C

  const uint8_t LED_PIN = 5;                  //пин для сигнального светодиода

#endif


#ifdef use_adafruit_library

   
  
  TwoWire SSD1306_Wire = TwoWire(0);    //создаём экземпляр класса TwoWire для экрана

  #define SCREEN_WIDTH 128              // Ширина дисплея в пикселах
  #define SCREEN_HEIGHT 64              // Высота дисплея в пикселах
  #define OLED_RESET    -1              // Пин сброса # ( -1 если для сброса используется стандартный пин ардуино)
  #define SCREEN_ADDRESS 0x3C           // Стандартный адрес I2C для дисплея (в моём случае такой адрес дал I2C-сканнер)

  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SSD1306_Wire, OLED_RESET); //Создаём объект дисплея

  
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



  SPIClass sx127x_fspi(FSPI);     //Создаём кземпляр класса SPI для инициализации на контроллере ESP32S2

  //Задаём пины SPI в соответствии с разводкой модуля для ESP32S2

  const int8_t SX127X_CS_PIN = 12;      //Пин CS
  const int8_t SX127X_MOSI_PIN = 11;    //Пин MOSI
  const int8_t SX127X_MISO_PIN = 9;     //Пин MISO
  const int8_t SX127X_SCK_PIN = 7;      //Пин SCK
  
  // Подключение радиотрансивера SX127.. в соответствии с разводкой  модуля для ESP32S2:
  const uint32_t NSS = SX127X_CS_PIN;      // NSS pin:   12
  const uint32_t DIO_0 = 33;               // DIO0 pin:  33
  const uint32_t RST = 16;                 // RESET pin: 16
  const uint32_t DIO_1 = 35;               // DIO1 pin:  35

  SX1278 radio1 = new Module(NSS, DIO_0, RST, DIO_1, sx127x_fspi); //Инициализируем экземпляр радио



  String str;               //Строка для формирования вывода полученных данных на экран и в сериал порт

  String RSSI = F("RSSI("); //Строка для печати RSSI
  String dBm = F(")dBm");   //Строка для печати RSSI

  String SNR = F("SNR(");   //Строка для печати SNR
  String dB = F(")dB");     //Строка для печати SNR

  String FR_ERR = F("F_Err(");  //Строка для печати SNR
  String HZ = F(")Hz");         //Строка для печати SNR

  String DT_RATE = F("RATE(");  //Строка для печати скорости передачи данных
  String BS = F(")B/s");        //Строка для печати скорости передачи данных

  //#ifdef TRANSMITTER
    String TRANSMIT = F("TRANSMIT: ");  //Строка сообщения для передачи
  //#endif

  // #ifdef RECEIVER
    String RECEIVE = F("RECEIVE: ");  //Строка сообщения для приёма
  // #endif


  int state = RADIOLIB_ERR_NONE;; // Переменная, хранящая код состояния передачи/приёма



  /**
   * @brief Функция инициализации линии I2C и самого дисплея 
   * 
   */
  void displayInit()
  {

    SSD1306_Wire.begin(SDA_display, SCL_display, 400000);     //Инициализируем линию I2C

    //Инициализируем дисплей
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {// SSD1306_SWITCHCAPVCC = напряжение дисплея от 3.3V
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }

    // Показываем содержимое буфера дисплея, созданное по-умолчанию
    // библиотека по-умолчанию использует эмблему Adafruit.
    //display.display();
    //delay(1000); // Pause for 2 seconds

    //Очищаем буффер дисплея
    display.clearDisplay();

    display.setTextSize(1);                 // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);    // Draw white text
    display.cp437(true);                    // Use full 256 char 'Code Page 437' font
  }




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







  /**
  * @brief Функция отправляет данные, выводит на экран информацию об отправке,
  * выводит информацию об отправке в сериал-порт
  * 
  * @param transmit_str - строка для передачи
  */
  void transmit_and_print_data(String &transmit_str)
  {
    //Посылаем очередной пакет
    Serial.print(F("[SX1278] Send packet ... "));

    // можно передавать C-string или Arduino string  длиной до 255 символов
    // Также можно передавать массив байт длиной до 255 байт
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
      int state = radio.startTransmit(byteArr, 8);
    */
    state = radio1.startTransmit(transmit_str);

    //Если передача успешна, выводим сообщение в сериал-монитор
    if (state == RADIOLIB_ERR_NONE) {
      //Выводим сообщение об успешной передаче
      Serial.println(F("transmission finished succes!"));

      display.setCursor(0, 0);
      String str1 = TRANSMIT + transmit_str;
      display.print(str1);
                  
      //Выводим в сериал данные отправленного пакета
      Serial.print(F("[SX1278] Data:\t\t"));
      Serial.println(transmit_str);

      //Печатаем RSSI (Received Signal Strength Indicator)
      float rssi_data = radio1.getRSSI();
      String RSSI_DATA = (String)rssi_data;
          
      Serial.print(F("[SX1278] \t\t\t"));
      Serial.print(RSSI);
      Serial.print(RSSI_DATA);
      Serial.println(dBm);
          
      display.setCursor(0, 16);
      display.print(RSSI);
      display.print(RSSI_DATA);
      display.print(dBm);
              

      // печатаем SNR (Signal-to-Noise Ratio)
      float snr_data = radio1.getSNR();
      String SNR_DATA = (String)snr_data;

      Serial.print(F("[SX1278] \t\t\t"));
      Serial.print(SNR);
      Serial.print(SNR_DATA);
      Serial.println(dB);

      display.setCursor(0, 27);
      display.print(SNR);
      display.print(SNR_DATA);
      display.print(dB);


      // печатаем скорость передачи данных последнего пакета (бит в секунду)
      // float data_rate = radio1.getDataRate();
      float data_rate = radio1.getDataRate();
      String DATA_RATE = (String) data_rate;

      Serial.print(F("[SX1278] \t\t\t"));
      Serial.print(DT_RATE);
      Serial.print(DATA_RATE);
      Serial.println(BS);

      display.setCursor(0, 38);
      display.print(DT_RATE);
      display.print(DATA_RATE);
      display.print(BS);

      display.display();
      display.clearDisplay();

      digitalWrite(LED_PIN, LOW);     //Включаем светодиод, сигнализация об передаче/приёма пакета
          
    } else {
      //Если были проблемы при передаче, сообщаем об этом
      Serial.print(F("transmission failed, code = "));
      Serial.println(state);
      display.clearDisplay();
      display.setCursor(0, 10);
      display.print(F("ERROR: "));
      display.print(state);
      display.display();

    }

  }
  




  /**
  * @brief Функция получает данные, выводит на экран информацию о полученном,
  * выводит информацию о получении в сериал-порт
  * 
  */
  void receive_and_print_data()
  {
    //можно прочитать полученные данные как строку
    state = radio1.readData(str);

    //Если пакет данных был получен успешно, распечатываем данные
    //в сериал - монитор и на экран
    if (state == RADIOLIB_ERR_NONE) {

      display.setCursor(0, 0); 
      display.print(RECEIVE);
      display.print(str);
                  
      Serial.println(F("[SX1278] Received packet!"));

      // print data of the packet
      Serial.print(F("[SX1278] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      float rssi_data = radio1.getRSSI();
      String RSSI_DATA = (String)rssi_data;
          
      Serial.print(F("[SX1278] \t\t\t"));
      Serial.print(RSSI);
      Serial.print(RSSI_DATA);
      Serial.println(dBm);
          
      display.setCursor(0, 16);
      display.print(RSSI);
      display.print(RSSI_DATA);
      display.print(dBm);
              

      // print SNR (Signal-to-Noise Ratio)
      float snr_data = radio1.getSNR();
      String SNR_DATA = (String)snr_data;

      Serial.print(F("[SX1278] \t\t\t"));
      Serial.print(SNR);
      Serial.print(SNR_DATA);
      Serial.println(dB);

      display.setCursor(0, 27);
      display.print(SNR);
      display.print(SNR_DATA);
      display.print(dB);

      // print frequency error
      float freq_error = radio1.getFrequencyError();
      String FREQ_ERROR = (String) freq_error;

      Serial.print(F("[SX1278] \t\t\t"));
      Serial.print(FR_ERR);
      Serial.print(FREQ_ERROR);
      Serial.println(HZ);

      display.setCursor(0, 38);
      display.print(FR_ERR);
      display.print(FREQ_ERROR);
      display.print(HZ);

      display.display();
      display.clearDisplay();

      digitalWrite(LED_PIN, LOW);     //Включаем светодиод, сигнализация об передаче/приёма пакета

      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        Serial.println(F("[SX1278] CRC ERROR!"));
        display.clearDisplay();
        display.setCursor(0, 10);
        display.print(F("CRC ERROR!"));
        display.display();

      } else {
        // some other error occurred
        Serial.print(F("[SX1278] Failed, code "));
        Serial.println(state);
        display.clearDisplay();
        display.setCursor(0, 10);
        display.print(F("ERROR: "));
        display.print(state);
        display.display();
      }
  }
  

#endif




#ifdef use_i2c_scanner

  TwoWire SSD1306_Wire = TwoWire(0);

  /**
   * @brief Функция, сканирующая линию I2C и выводящая результат сканирования
   * в сериал-порт 
   */
  void scanner()
  {
    byte error, address;
    int nDevices;
    Serial.println("Scanning...");
    nDevices = 0;
    for(address = 1; address < 127; address++ ) {
      SSD1306_Wire.beginTransmission(address);
      error = SSD1306_Wire.endTransmission();
      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address<16) {
          Serial.print("0");
        }
        Serial.println(address,HEX);
        nDevices++;
      }
      else if (error==4) {
        Serial.print("Unknow error at address 0x");
        if (address<16) {
          Serial.print("0");
        }
        Serial.println(address,HEX);
      }    
    }
    if (nDevices == 0) {
      Serial.println("No I2C devices found\n");
    }
    else {
      Serial.println("done\n");
    }
    delay(5000);
  }


  /**
   * Если выбран  i2c_scanner, то функция displayInit() будет несколько видоизменена
   * поскольку теперь её назначение - инициализация I2C
   */
  void displayInit(){
    SSD1306_Wire.begin(SDA_display, SCL_display, 400000);
    Serial.println("\nI2C Scanner");
    Serial.println(" ");

    scanner();
  }

#endif











































void setup() {
  //Инициализируем сериал-монитор со скоростью 115200 на выделенных пинах
  //Serial1.begin(115200, SERIAL_8N1, RX_pin, TX_pin);

  Serial.begin(115200);
  delay(1000);

  //инициализируем дисплей
  Serial.println("Display init....");
  displayInit();

  
  //Инициализируем SPI
  Serial.println("Begin SPI....");
  sx127x_fspi.begin(SX127X_SCK_PIN, SX127X_MISO_PIN, SX127X_MOSI_PIN, SX127X_CS_PIN);

  pinMode(sx127x_fspi.pinSS(), OUTPUT);
  pinMode(LED_PIN, OUTPUT);                     //Пин для светодиода 5 используем для сигнализации состояния приёма-передачи
                                                //Для ESP32S3 можно использовать встроенный светодиод при необходимости.
    
  //Задаём параметры конфигурации радиотрансивера 1
  config_radio1.frequency = 433;
  config_radio1.bandwidth = 125;
  config_radio1.spreadingFactor = 10;
  config_radio1.codingRate = 6;
  config_radio1.syncWord = 0x14;
  config_radio1.outputPower = 10;
  config_radio1.currentLimit = 80;
  config_radio1.preambleLength = 15;
  config_radio1.gain = 1;
    
  //Инициализируем радиотрансивер со значениями по-умолчанию
  Serial.println(" ");
  Serial.print(F("[SX127x] Initializing ... "));
    
  int state = radio1.begin();

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("SUCCES!"));
  } else {
    Serial.print(F("ERROR!  "));
    Serial.println(state);
    
    display.setCursor(0, 10);
    display.print(F("ERROR: "));
    display.print(state);
    display.display();

    while (true);
  }
    
    
  //Устанавливаем наши значения, определённые ранее в структуре config_radio1
  radio_setSettings(radio1);


  #ifdef RECEIVER   //Если определена работа модуля как приёмника

    //Устанавливаем функцию, которая будет вызываться при получении пакета данных
    radio1.setPacketReceivedAction(setFlag);

    //Начинаем слушать есть ли пакеты
    Serial.print(F("[SX1278] Starting to listen ... "));
    state = radio1.startReceive();
  
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
      digitalWrite(LED_PIN, LOW);     //Включаем светодиод, сигнализация об передаче/приёма пакета
    } else {
      Serial.print(F("failed, code: "));
      Serial.println(state);
      while (true);
    }

    //получаем данные
    receive_and_print_data();

        
  #endif


  #ifdef TRANSMITTER   //Если определена работа модуля как передатчика

    //Устанавливаем функцию, которая будет вызываться при отправке пакета данных
    radio1.setPacketSentAction(setFlag);

    //Начинаем передачу пакетов
    Serial.println(F("[SX1278] Sending first packet ... "));

    str = F("START!");
    transmit_and_print_data(str);

    delay(2000);

  #endif
  

  Serial.println(" ");

  digitalWrite(LED_PIN, HIGH); //Выключаем светодиод, сигнализация об окончании передачи/приёма пакета
  
  Serial.println(" ");
    
}













void loop() {

  digitalWrite(LED_PIN, HIGH); //Выключаем светодиод, сигнализация об окончании передачи/приёма пакета
  delay(500);
  

  #ifdef RECEIVER   //Если определен модуль как приёмник
    //проверяем, была ли предыдущая передача успешной
    Serial.println("..................................................");
    if(operationDone) {
      //Сбрасываем сработавший флаг прерывания
      operationDone = false;
      receive_and_print_data();
    }
  #endif


  #ifdef TRANSMITTER   //Если определен модуль как передатчик
    //проверяем, была ли предыдущая передача успешной
    Serial.println("..................................................");
    if(operationDone) {
      //Сбрасываем сработавший флаг прерывания
      operationDone = false;
      //готовим строку для отправки
      str = "#" + String(count++);
      transmit_and_print_data(str);
      delay(500);
    }
  #endif


}



