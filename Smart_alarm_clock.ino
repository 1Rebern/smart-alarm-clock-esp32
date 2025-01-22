#define byte uint8_t

// Ініціалізація бібліотек
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <NTPClient.h>
#include <HTTPUpdate.h>
#include <FS.h>
#include <Arduino.h>
#include <map>

// Налаштування SD-карти
#define SD_CS_PIN         5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18

// Налаштування I2S
#define I2S_DOUT_PIN      25
#define I2S_BCLK_PIN      27
#define I2S_LRC_PIN       26

// Інші піни
#define PIN_DIOD          4
#define PIN_BUTTON        2

// Налаштування Wi-Fi та Telegram
#define BOT_TOKEN         ""
#define ALLOWED_CHAT_ID   ""

#define DEBOUNCE_TIME 500  // Час між натисканнями (мс)
#define DELAY_BETWEEN_CLICKS 3500  // Максимальна різниця часу між натисканнями (мс) для двох натискань

// Налаштування часу
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 3600000);

// Telegram-бот
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;
const unsigned long BOT_MTBS = 10000; // Час перевірки нових повідомлень

// Структура для збереження стану користувача
struct BotState {
  String currentCommand;  // Поточна команда
  int step;               // Поточний етап налаштування
  int selectedMonth;      // Місяць
  int selectedDay;        // День
  int selectedHour;       // Година
  int selectedMinute;     // Хвилина
};

std::map<String, BotState> botStates;  // Словник станів для кожного chat_id

// Дисплей
Adafruit_SSD1306 display(128, 32, &Wire, -1);

// Аудіо
Audio audio;
int volumeLevel = 10;
String currentFilename = "default_alarm";

// Перемінні для роботи з кнопкою
unsigned long lastButtonPress = 0;  // Час останнього натискання кнопки
unsigned long buttonPressStartTime = 0;  // Час початку натискання кнопки
int buttonPressCount = 0;  // Лічильник натискань


// Структура для збереження будильника
struct Alarm {
  int month;  // Місяць
  int day;    // День
  int hour;   // Година
  int minute; // Хвилина
};

std::vector<Alarm> alarms;  // Вектор для збереження будильників
String alarmSound = "default_alarm";

// Час початку (для яскравості)
unsigned long startTime;

// Ініціалізація
void setup() {
  Serial.begin(115200);
  
  // Ініціалізація дисплея
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();

  // Масив мереж
  struct WiFiCredentials {
      const char* ssid;
      const char* password;
  };

  WiFiCredentials wifiNetworks[] = {
    {"name", "pass"},
    {"", ""},
    {"", ""},
    {"", ""}
    //{"SSID", "PASSWORD"}
  };

  const int wifiCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

  // Wi-Fi: пошук і підключення
  WiFi.mode(WIFI_STA); // Режим клієнта
  bool isConnected = false;

  for (int i = 0; i < wifiCount; i++) {
    display.clearDisplay();
    display.setCursor(0, 16);
    display.print("Connecting to: ");
    display.print(wifiNetworks[i].ssid);
    display.display();
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(1000);
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      isConnected = true;
      break;
    }
  }

  if (!isConnected) {
    display.clearDisplay();
    display.setCursor(0, 16);
    display.print("Wi-Fi Error!");
    display.display();
    while (1); // Зупиняємо програму, якщо немає Wi-Fi
  }

  display.clearDisplay();
  display.setCursor(0, 16);
  display.print("Wi-Fi Connected!");
  display.display();
  delay(1000);

  // Ініціалізація Telegram-клієнта
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // SD-карта
  pinMode(SD_CS_PIN, OUTPUT);
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
  if (!SD.begin(SD_CS_PIN)) {
    display.clearDisplay();
    display.setCursor(0, 16);
    display.print("SD-card Error!");
    display.display();
    while (1);
  }
  display.clearDisplay();
  display.setCursor(0, 16);
  display.print("SD-card Connected!");
  display.display();

  printAlarms();// Тестування справності файлу
  delay(1000);

  // Аудіо
  audio.setPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
  audio.setVolume(volumeLevel);

  // NTP-клієнт
  timeClient.begin();
  
  // Ініціалізація часу для яскравості
  startTime = millis();

  // Ініціалізація будильників з sd-карти
  loadAlarmsFromSD();
}

// Встановлення часу відповідно до часового поясу України
int getTimeOffset() {
  // Отримання поточної дати
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&now);
  
  int month = timeinfo->tm_mon + 1; // Місяці від 0 до 11
  int day = timeinfo->tm_mday; // День місяця

  // Перевірка на літній час
  if ((month > 3 && month < 10) || (month == 3 && day >= 25) || (month == 10 && day < 25)) {
    return 10800; // UTC+3 (літній час)
  } else {
    return 7200; // UTC+2 (зимовий час)
  }
}

void loop() {
  // Оновлення часу
  timeClient.update();
  timeClient.setTimeOffset(getTimeOffset()); // Встановлення зміщення часу
  
  // Перевірка кнопки
  checkButton();

  // Telegram-бот
  if (millis() - bot_lasttime > BOT_MTBS) {
    bot_lasttime = millis();  // Оновлюємо таймер Telegram-бота
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages) {
      handleNewMessages(numNewMessages);
    }
  }
  
  // Перевірка будильників і оновлення яскравості
  updateAlarms();
  
  // Оновлення дисплея
  updateDisplay();

  // Основний цикл аудіо
  audio.loop();
}

// Тестування правильності запису файлу
void printAlarms() {
    File file = SD.open("/alarms.txt");
    if (!file) {
        Serial.println("Не вдалося відкрити файл alarms.txt для читання.");
        return;
    }
    Serial.println("Вміст alarms.txt:");
    while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
    }
    file.close();
}

void updateAlarms() {
  time_t now = timeClient.getEpochTime(); // Отримуємо поточний час
  struct tm *timeinfo = localtime(&now);

  for (size_t i = 0; i < alarms.size(); ++i) {
    // Перевіряємо, чи співпадає дата
    if (alarms[i].month == (timeinfo->tm_mon + 1) && alarms[i].day == timeinfo->tm_mday) {
      // Розрахунок часу початку активації LED-стрічки
      struct tm brightnessTimeInfo = *timeinfo;
      brightnessTimeInfo.tm_hour = alarms[i].hour;
      brightnessTimeInfo.tm_min = alarms[i].minute;
      brightnessTimeInfo.tm_sec = 0;
      time_t alarmTime = mktime(&brightnessTimeInfo);
      time_t brightnessStartTime = alarmTime - 40 * 60;

      // Якщо зараз час для активації LED-стрічки
      if (now >= brightnessStartTime && now < alarmTime) {
        unsigned long currentMillis = millis();
        unsigned long brightnessStartMillis = millis() - (now - brightnessStartTime) * 1000;
        updateBrightness(currentMillis, brightnessStartMillis);
      }

      // Перевіряємо, чи співпадає час
      if (alarms[i].hour == timeinfo->tm_hour &&
          alarms[i].minute == timeinfo->tm_min) {
        Serial.println("Будильник спрацював!");
        bot.sendMessage(ALLOWED_CHAT_ID, "Будильник працює!");
        playFile(ALLOWED_CHAT_ID, alarmSound, 0);
        analogWrite(PIN_DIOD, 255); // Увімкнути максимальну яскравість
        
        // Видаляємо будильник після спрацювання
        alarms.erase(alarms.begin() + i);
        saveAlarmsToSD();
        --i;
      }
    }
  }
}

// Поступове збільшення яскравості Led-стрічки за 40хв
void updateBrightness(unsigned long currentMillis, unsigned long brightnessStartMillis) {
  unsigned long elapsedMillis = currentMillis - brightnessStartMillis;
  unsigned long elapsedSeconds = elapsedMillis / 1000;

  int brightness = (elapsedSeconds * 255) / (40 * 60);
  if (brightness > 255) brightness = 255; // Обмежуємо яскравість максимальним значенням
  analogWrite(PIN_DIOD, brightness);
}

// Оновлення дисплея
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(F("Time: "));
  display.print(timeClient.getFormattedTime());

  display.setCursor(0, 16);
  display.print(F("Volume: "));
  display.print(volumeLevel);
  display.display();
}

// Обробка нових повідомлень Telegram
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    if (chat_id != ALLOWED_CHAT_ID) {
      bot.sendMessage(chat_id, "Доступ заборонено ❌", "");
      continue;
    }

    String text = bot.messages[i].text;

    if (bot.messages[i].type == "message") {
      if (botStates[chat_id].currentCommand == "/upload") {
        processUploadFile(chat_id, i);
        botStates[chat_id].currentCommand = ""; // Скидання стану після завантаження
        return;
      }

      if(botStates[chat_id].currentCommand == "/setalarm"){
        setAlarm(chat_id, text);
        return;
      }

      if (text == "/start") {
      bot.sendMessage(chat_id, "Ласкаво просимо! 😊\nОсь доступні команди:\n\n/start - показати цей список команд\n/dir - Список файлів на SD-карті\n/upload - Завантаження файлу\n/delete <ім'я_файлу> - Видалення файлу\n/play <ім'я_файлу> - Відтворення файлу\n/volume <0-21> - Змінити гучність\n/sound <ім'я_файлу> - Вибір звуку будильника\n/setalarm - Додати будильник\n/viewalarms - Переглянути будильники\n/deletealarm - Видалити будильник\n/play <ім'я_файлу> - Відтворити звук", "");
      }
      else if (text == "/dir") {
        listFiles(chat_id);
      }
      else if (text == "/upload") {
        bot.sendMessage(chat_id, "⏳ Очікую надсилання файлу .wav...", "");
        botStates[chat_id].currentCommand = "/upload"; // Встановлення стану для очікування файлу
      }
      else if (text.startsWith("/deletealarm")) {
        deleteAlarm(chat_id, text);
      }
      else if (text.startsWith("/delete")) {
        deleteFile(chat_id, text);
      }
      else if (text.startsWith("/volume")) {
        setVolume(chat_id, text);
      }
      else if (text.startsWith("/sound")) {
        setAlarmSound(chat_id, text);
      }
      else if (text == "/viewalarms") {
        viewAlarms(chat_id);
      }
      else if (text.startsWith("/setalarm")) {
        setAlarm(chat_id, text);
      }
      else if (text.startsWith("/play")){
        playFile(chat_id, text, 1);
      }
      else if (text.startsWith("/ledtest")){
        ledTest(chat_id);
      }
      else if (text.startsWith("/ledselect")){
        ledSelect(chat_id, text);
      }
      else {
        bot.sendMessage(chat_id, "Команда не розпізнана. Спробуйте ще раз.", "");
      }
    }
  }
}

// Функція для перевірки LED-стрічки
void ledTest(String chat_id) {
  bot.sendMessage(chat_id, "💡 Тестування LED-стрічки: Вмикаємо...", "");
  for (int brightness = 0; brightness <= 255; brightness++) { // Плавне підвищення яскравості
    analogWrite(PIN_DIOD, brightness);
    delay(1000); // Затримка для плавного переходу
  }
  bot.sendMessage(chat_id, "✨ LED-стрічка працює належним чином!", "");

  bot.sendMessage(chat_id, "💡 Вимикаємо LED-стрічку...", "");
  analogWrite(PIN_DIOD, 0);
  bot.sendMessage(chat_id, "🚦 LED-стрічка вимкнена.", "");
}

// Функція для встановлення яскравості LED-стрічки
void ledSelect(String chat_id, String text) {
  int brightness = text.substring(11).toInt();
  bot.sendMessage(chat_id, "💡 Яскравість LED-стрічки встановлено на: " + String(brightness), "");
  
  analogWrite(PIN_DIOD, brightness);
}

// Функція для переліку файлів на SD-карті
void listFiles(String chat_id) {
  File root = SD.open("/");
  String fileList = "Файли на SD-карті:\n";
  while (File file = root.openNextFile()) {
    fileList += String(file.name()) + " (" + String(file.size() / 1024) + " KB)\n";
    file.close();
  }
  bot.sendMessage(chat_id, fileList, "");
}

// Функція обробки завантаження файлів
void processUploadFile(String chat_id, int messageIndex) {
  // Перевірка наявності документа
  if (bot.messages[messageIndex].hasDocument) {
    String fileName = bot.messages[messageIndex].file_name;
    String fileCaption = bot.messages[messageIndex].file_caption;
    int fileSize = bot.messages[messageIndex].file_size;

    // Перевірка доступного місця на SD-карті
    if (fileSize < SD.totalBytes() - SD.usedBytes()) {
      bot.sendMessage(chat_id, "⏳ Завантажуємо файл: " + fileName, "");

      HTTPClient http;
      if (http.begin(secured_client, bot.messages[messageIndex].file_path)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
          int fileLen = http.getSize();
          int remaining = fileLen;
          uint8_t buffer[256];
          File file = SD.open("/" + fileName, FILE_WRITE);

          if (!file) {
            bot.sendMessage(chat_id, "❌ Помилка відкриття файлу на SD-карті.", "");
          } else {
            while (http.connected() && (remaining > 0 || remaining == -1)) {
              size_t available = http.getStreamPtr()->available();
              if (available) {
                int bytesRead = http.getStreamPtr()->readBytes(buffer, 
                                  min(available, sizeof(buffer)));
                file.write(buffer, bytesRead);
                if (remaining > 0) {
                  remaining -= bytesRead;
                }
              }
              delay(1);
            }
            file.close();
            if (remaining == 0) {
              bot.sendMessage(chat_id, "✅ Файл успішно завантажено: " + fileName, "");
            } else {
              bot.sendMessage(chat_id, "❌ Помилка під час завантаження файлу.", "");
            }
          }
        } else {
          bot.sendMessage(chat_id, "❌ HTTP помилка: " + String(httpCode), "");
        }
        http.end();
      } else {
        bot.sendMessage(chat_id, "❌ Помилка з'єднання для завантаження.", "");
      }
    } else {
      bot.sendMessage(chat_id, "❌ Недостатньо місця на SD-карті для файлу " + fileName, "");
    }
  } else {
    bot.sendMessage(chat_id, "⚠️ Надішліть документ для завантаження.", "");
  }
}

// Функція для видалення файлу
void deleteFile(String chat_id, String text) {
  if (text.substring(8) != ""){
    String fileName = "/" + text.substring(8);
    if (SD.exists(fileName)) {
      SD.remove(fileName.c_str());
      bot.sendMessage(chat_id, "Файл " + fileName + " успішно видалено з SD-карти. 🗑️", "");
    } else {
      bot.sendMessage(chat_id, "Помилка! Файл " + fileName + " не знайдено на SD-карті. 😕", "");
    } 
  } else {
      bot.sendMessage(chat_id, "Помилка! Вкажіть назву файла!", "");
    }
}

// Функція для відтворення файлу
void playFile(String chat_id, String text, bool change) {
  if(change){
    String fileName = "/" + text.substring(6) + ".wav";
      if (SD.exists(fileName)) {
        audio.connecttoFS(SD, fileName.c_str());
        bot.sendMessage(chat_id, "🎶 Відтворюється файл: " + fileName, "");
      } else {
        bot.sendMessage(chat_id, "Помилка! Файл " + fileName + " не знайдено на SD-карті. 😕", "");
      }
  }else{
    String fileName = "/" + text + ".wav";
      if (SD.exists(fileName)) {
        audio.connecttoFS(SD, fileName.c_str());
      } else {
        bot.sendMessage(chat_id, "Помилка! Файл " + fileName + " не знайдено на SD-карті. 😕", "");
      }
  }
}

// Функція для зміни гучності
void setVolume(String chat_id, String text) {
  int volume = text.substring(8).toInt();
  if (volume >= 0 && volume <= 21) {
    volumeLevel = volume;
    audio.setVolume(volumeLevel);
    bot.sendMessage(chat_id, "🔊 Гучність встановлено на " + String(volumeLevel), "");
  } else {
    bot.sendMessage(chat_id, "❌ Некоректне значення гучності. Введіть число від 0 до 21.", "");
  }
}

// Функція для вибору звуку будильника
void setAlarmSound(String chat_id, String text) {
  String soundFile = text.substring(7); // Відрізаємо команду "/sound " (довжина 7 символів)

  // Перевірка, чи існує файл на SD-карті
  if (SD.exists(soundFile)) {
    alarmSound = "/" + soundFile + ".wav";
    bot.sendMessage(chat_id, "🔔 Звук будильника успішно змінено на: " + soundFile, "");
  } else {
    bot.sendMessage(chat_id, "❌ Файл " + soundFile + " не знайдено на SD-карті. Перевірте ім'я файлу та спробуйте ще раз.", "");
  }
}

// Функція для отримання даних про будильник від користувача
void setAlarm(String chat_id, String text) {
  BotState &state = botStates[chat_id];  // Отримуємо стан користувача

  if (state.currentCommand != "/setalarm") {
    bot.sendMessage(chat_id, "Введіть день і місяць для будильника (наприклад, 06/12):");
    state.currentCommand = "/setalarm";
    state.step = 1;
    return;
  }

  if (state.step == 1) {
    // Перевіряємо формат введеного дня і місяця
    if (text.indexOf("/") != -1) {
      state.selectedMonth = text.substring(text.indexOf("/") + 1).toInt();
      state.selectedDay = text.substring(0, text.indexOf("/")).toInt();

      bot.sendMessage(chat_id, "Введіть час будильника (наприклад, 06:40):");
      state.step = 2;
    } else {
      bot.sendMessage(chat_id, "Невірний формат дати. Будь ласка, спробуйте ще раз.");
    }
    return;
  }

  if (state.step == 2) {
    // Перевіряємо формат введеного часу
    if (text.indexOf(":") != -1) {
      state.selectedHour = text.substring(0, text.indexOf(":")).toInt();
      state.selectedMinute = text.substring(text.indexOf(":") + 1).toInt();

      // Зберігаємо будильник
      Alarm newAlarm = {
        state.selectedMonth, // Місяць
        state.selectedDay,   // День
        state.selectedHour,  // Година
        state.selectedMinute // Хвилина
      };

      alarms.push_back(newAlarm);
      saveAlarmsToSD();

      // Повідомляємо користувача
      bot.sendMessage(chat_id, "Будильник встановлено на " +
                        String(newAlarm.day < 10 ? "0" : "") + String(newAlarm.day) + "/" +
                        String(newAlarm.month < 10 ? "0" : "") + String(newAlarm.month) + " " +
                        String(newAlarm.hour < 10 ? "0" : "") + String(newAlarm.hour) + ":" +
                        String(newAlarm.minute < 10 ? "0" : "") + String(newAlarm.minute));

      // Скидаємо стан
      state.currentCommand = "";
      state.step = 0;
    } else {
      bot.sendMessage(chat_id, "Невірний формат часу. Будь ласка, спробуйте ще раз.");
    }
  }
}

// Функція для перегляду всіх будильників
void viewAlarms(String chat_id) {
  if (alarms.empty()) {
    bot.sendMessage(chat_id, "Немає встановлених будильників⏰.", "");
    return;
  }

  String alarmList = "Список будильників⏰:\n";
  for (size_t i = 0; i < alarms.size(); i++) {
    alarmList += String(i + 1) + ". " + 
                 (alarms[i].day < 10 ? "0" : "") + String(alarms[i].day) + "." +
                 (alarms[i].month < 10 ? "0" : "") + String(alarms[i].month) + " " +
                 (alarms[i].hour < 10 ? "0" : "") + String(alarms[i].hour) + ":" +
                 (alarms[i].minute < 10 ? "0" : "") + String(alarms[i].minute) + "\n";
  }

  bot.sendMessage(chat_id, alarmList, "");
}

// Функція для видалення будильника
void deleteAlarm(String chat_id, String text) {
  String indexStr = text.substring(13);  // Видаляємо команду "/deletealarm "
  int index = indexStr.toInt() - 1; // Індекс починається з 0
  
  if (index >= 0 && index < alarms.size()) {
    alarms.erase(alarms.begin() + index);  // Видаляємо будильник
    saveAlarmsToSD();  // Оновлюємо список на SD
    bot.sendMessage(chat_id, "Будильник видалено! ⏰", "");
  } else {
    bot.sendMessage(chat_id, "Помилка! Невірний індекс будильника.", "");
  }
}

// Функція для збереження списку будильників на SD карту
void saveAlarmsToSD() {
  File file = SD.open("/alarms.txt", FILE_WRITE);
  if (file) {
    for (size_t i = 0; i < alarms.size(); ++i) {
      file.printf("%d %d %02d %02d\n", alarms[i].month, alarms[i].day, alarms[i].hour, alarms[i].minute);
    }
    file.close();
  } else {
    Serial.println("Помилка відкриття файлу для запису.");
  }
}

// Функція для завантаження списку будильників з SD карти
void loadAlarmsFromSD() {
  alarms.clear();
  File file = SD.open("/alarms.txt", FILE_READ);
  if (file) {
    char line[64];
    while (file.available()) {
      file.readBytesUntil('\n', line, sizeof(line));
      line[sizeof(line) - 1] = '\0';

      int month, day, hour, minute;
      if (sscanf(line, "%d %d %02d %02d", &month, &day, &hour, &minute) == 4) {
        alarms.push_back({month, day, hour, minute});
      }
    }
    file.close();
  } else {
    Serial.println("Помилка відкриття файлу для читання.");
  }
}

// Функція обробки натискання кнопки
void checkButton() {
  int buttonState = digitalRead(PIN_BUTTON);
  unsigned long currentTime = millis();
  bool isPlaying;

  if (buttonState == HIGH) {
    if (currentTime - lastButtonPress > DEBOUNCE_TIME) {
      lastButtonPress = currentTime;

      if (buttonPressCount == 0) {
        buttonPressStartTime = currentTime;  // Фіксуємо час початку натискання
      }

      buttonPressCount++;
    }
  } else {
    if (buttonPressCount > 0 && currentTime - buttonPressStartTime > DELAY_BETWEEN_CLICKS) {
      if (buttonPressCount == 1) {
        Serial.println("Один натиск - скидання лічильника.");
        buttonPressCount = 0;
      } else if (buttonPressCount == 2) {
        bot.sendMessage(ALLOWED_CHAT_ID, "Будильник вимкнуто", "");
        analogWrite(PIN_DIOD, LOW);  // Вимикаємо LED
        isPlaying = false;  // Зупиняємо цикл відтворення
      }
      // Скидаємо лічильник натискань після обробки
      buttonPressCount = 0;
    }
  }

  // Циклічне відтворення звуку будильника
  if (isPlaying) {
    if (!audio.isRunning()) {  // Якщо звук завершився, запускаємо знову
      playFile(ALLOWED_CHAT_ID, alarmSound, 0);
    }
  }
}
