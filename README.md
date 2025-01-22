# smart-alarm-clock-esp32
IoT project of a smart alarm clock with dawn simulation.
<p>The alarm clock is controlled by a Telegram bot.

# Project idea
Ease the process of waking up in the morning, test my skills and learn new ones.

# Technologies used
Programming language:
* C++ (Arduino platform).

Libraries:
* Adafruit GFX and Adafruit SSD1306 - for working with an OLED display.
* Audio - to play audio files.
* SD, SPI, FS - to work with an SD card and file system.
* WiFi, WiFiClientSecure - for connecting to a Wi-Fi network and secure connection.
* UniversalTelegramBot - to create a Telegram bot.
* NTPClient - to get the current time.

Arduino system modules:
* Wire - to work with I2C devices.
* analogWrite, millis - for working with PWM control and time processing.

Hardware technologies and tools:
* 3D modeling - for designing the device case in Fusion 360.
* 3D printing - manufacturing of the case and auxiliary parts on Creality printers.
* Slicering - preparing 3D models for printing in the Cura program.
* Soldering - is used to connect electronic components.
* Development of schemes - designing the connection scheme for the device elements.
* Programming microcontrollers - writing firmware for the ESP platform.

# IoT scheme
<p><img src = "https://github.com/1Rebern/smart-alarm-clock-esp32/blob/main/Preview/iot_scheme.png?raw=true">

Components:
* ESP32-38 pin.
* UPS 5v.
* Speaker 8Ω 0.25w.
* MAX98357A.
* IRF540N.
* LCD 0,91.
* LED strip.
* Touch button.
* Switch.
* MH-SD card module.

# Project view
Disassembled view:<p><img src = "https://github.com/1Rebern/smart-alarm-clock-esp32/blob/bb8efe545ff15daeeeb21c6e58e9ab9a34d6c4a4/Preview/disassembled_view.jpg">

Final assembly view:<p><img src = "https://github.com/1Rebern/smart-alarm-clock-esp32/blob/bb8efe545ff15daeeeb21c6e58e9ab9a34d6c4a4/Preview/final_assembly.jpg">
# Example of work
Відправивши боту команду /start ми отримаємо список команд:
* Ласкаво просимо! 😊<p>Ось доступні команди:<p>/start - показати цей список команд<p>/dir - Список файлів на SD-карті<p>/upload - Завантаження файлу<p>/delete <ім'я_файлу> - Видалення файлу<p>/play <ім'я_файлу> - Відтворення файлу<p>/volume <0-21> - Змінити гучність<p>/sound <ім'я_файлу> - Вибір звуку будильника<p>/setalarm - Додати будильник<p>/viewalarms - Переглянути будильники<p>/deletealarm - Видалити будильник<p>/play <ім'я_файлу> - Відтворити звук

Eample of setting an alarm:<p><img src = "https://github.com/1Rebern/smart-alarm-clock-esp32/blob/bb8efe545ff15daeeeb21c6e58e9ab9a34d6c4a4/Preview/example.png">
