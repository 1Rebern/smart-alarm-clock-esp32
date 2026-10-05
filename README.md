# Smart Alarm Clock with Dawn Simulation

ESP32-based IoT alarm clock designed to make waking up easier through gradual light simulation and configurable audio alarms.

The alarm clock can be controlled remotely using a Telegram bot and combines embedded firmware, audio playback, LED control, Wi-Fi connectivity, SD-card storage and a custom 3D-printed enclosure.

---

## Project Goal

The goal of the project was to create a functional smart alarm clock while developing practical skills in embedded systems, electronics, firmware development and 3D prototyping.

The device was developed as a complete physical product rather than only a software prototype.

---

## System Architecture

```text
                    ┌─────────────────┐
                    │   Telegram Bot  │
                    └────────┬────────┘
                             │
                           Wi-Fi
                             │
                             ▼
                    ┌─────────────────┐
                    │      ESP32      │
                    │                 │
                    │  Alarm Logic    │
                    │  Time Sync      │
                    │  Bot Control    │
                    │  LED Control    │
                    │  Audio Control  │
                    └───────┬─────────┘
                            │
          ┌─────────────────┼──────────────────┐
          │                 │                  │
          ▼                 ▼                  ▼
      ┌───────┐        ┌─────────┐       ┌──────────┐
      │ OLED  │        │ SD Card │       │ LED Strip│
      └───────┘        └────┬────┘       └──────────┘
                            │
                            ▼
                       ┌─────────┐
                       │MAX98357A│
                       └────┬────┘
                            │
                            ▼
                         Speaker
```

---

## IoT / Electrical Scheme

The complete device connection scheme:

<p>
<img src="https://github.com/1Rebern/smart-alarm-clock-esp32/blob/main/Preview/Smart_Alarm_Clock.jpg?raw=true">
</p>

---

## Hardware

The project uses the following components:

- ESP32 38-pin development board
- 5 V UPS module
- 8 Ω 0.25 W speaker
- MAX98357A audio amplifier
- IRF540N transistor
- 0.91-inch display
- LED strip
- Touch button
- Power switch
- MH-SD card module

---

## Software

**Programming language:**

- C++ / Arduino

**Libraries:**

- Adafruit GFX
- Adafruit SSD1306
- Audio
- SD
- SPI
- FS
- WiFi
- WiFiClientSecure
- UniversalTelegramBot
- NTPClient

**Arduino system modules:**

- Wire
- analogWrite
- millis

---

## Firmware Responsibilities

The ESP32 firmware is responsible for:

- connecting the device to Wi-Fi;
- synchronizing time using NTP;
- processing Telegram bot commands;
- storing and managing audio files on an SD card;
- playing audio files;
- configuring the alarm sound;
- creating and deleting alarms;
- controlling display output;
- controlling the LED strip;
- processing user input from the touch button.

---

## 3D Design and Manufacturing

The physical enclosure and auxiliary parts were developed as part of the project.

### Tools and technologies

- Fusion 360 — 3D modeling
- Cura — slicing
- Creality 3D printers — manufacturing
- Soldering — electronic assembly
- Electrical scheme design — component integration

---

## Project View

### Disassembled View

<p>
<img src="https://github.com/1Rebern/smart-alarm-clock-esp32/blob/bb8efe545ff15daeeeb21c6e58e9ab9a34d6c4a4/Preview/disassembled_view.jpg">
</p>

### Final Assembly

<p>
<img src="https://github.com/1Rebern/smart-alarm-clock-esp32/blob/bb8efe545ff15daeeeb21c6e58e9ab9a34d6c4a4/Preview/final_assembly.jpg">
</p>

---

## Telegram Control

The alarm clock is controlled through a Telegram bot.

The bot provides commands for:

- listing files on the SD card;
- uploading audio files;
- deleting audio files;
- playing selected audio;
- changing volume;
- selecting the alarm sound;
- creating alarms;
- viewing alarms;
- deleting alarms.

Example command set:

```text
/start
/dir
/upload
/delete <file_name>
/play <file_name>
/volume <0-21>
/sound <file_name>
/setalarm
/viewalarms
/deletealarm
```

---

## Example of Work

Example of configuring an alarm through the Telegram bot:

<p>
<img src="https://github.com/1Rebern/smart-alarm-clock-esp32/blob/bb8efe545ff15daee21c6e58e9ab9a34d6c4a4/Preview/example.png">
</p>

---

## Engineering Scope

This project combines several areas of engineering in one device:

- embedded firmware;
- IoT communication;
- digital audio;
- external storage;
- time synchronization;
- LED control;
- electronic assembly;
- 3D modeling;
- 3D printing;
- physical device integration.

---
## Project Status

**Status:** Completed physical prototype.
