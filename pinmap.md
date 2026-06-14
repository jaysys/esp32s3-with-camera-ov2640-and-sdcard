# ESP32-S3 Camera & SD Card Pin Mapping (Final - 2026-06-13)

This document summarizes the verified hardware pin configuration and Arduino IDE settings for the **Freenove ESP32-S3 Cam (N16R8)** project.

## 1. Camera Pins (OV2640)
The following pins are configured and verified for the OV2640 camera interface:

| Function | GPIO Pin | Note |
| :--- | :--- | :--- |
| **SIOD (SDA)** | 4 | |
| **SIOC (SCL)** | 5 | |
| **VSYNC** | 6 | |
| **HREF** | 7 | |
| **PCLK** | 13 | |
| **XCLK** | 15 | |
| **Y2** | 11 | |
| **Y3** | 9 | |
| **Y4** | 8 | |
| **Y5** | 10 | |
| **Y6** | 12 | |
| **Y7** | 18 | |
| **Y8** | 17 | |
| **Y9** | 16 | |
| **PWDN** | -1 | Software control disabled |
| **RESET** | -1 | Software control disabled |

## 2. SD Card Pins (SDMMC 1-bit)
The integrated SD card slot uses the SDMMC protocol in 1-bit mode.

| Function | GPIO Pin |
| :--- | :--- |
| **CMD** | 38 |
| **CLK** | 39 |
| **DATA0** | 40 |

## 3. PSRAM Pins (OPI)
The **8MB Octal PSRAM (R8)** occupies the following pins internally. These pins MUST NOT be used for any other purpose when OPI PSRAM is enabled.

| Function | GPIO Pin |
| :--- | :--- |
| **PSRAM CLK/DQS** | 35, 36, 37 |

## 4. Arduino IDE Verified Settings (Crucial for Stability)
To prevent boot loops (`entry` hang) and memory allocation failures, use these exact settings:

| Setting | Value | Note |
| :--- | :--- | :--- |
| **Board** | `ESP32S3 Dev Module` | |
| **USB CDC On Boot** | `Enabled` | Required for Serial output |
| **Flash Mode** | `QIO` | **DO NOT use OPI** for Flash |
| **Flash Frequency** | `80MHz` | |
| **Flash Size** | `16MB (128Mb)` | Matches N16 spec |
| **PSRAM** | `OPI PSRAM` | Required for 8MB RAM (R8) |
| **Partition Scheme** | `16M Flash (3MB APP/9.9MB FATFS)` | |
| **Erase All Flash Before Sketch Upload** | `Enabled` | Recommended for first stable upload |

## 5. Network Configuration
*   **Web Server Port:** 80
*   **Static IP:** Dynamic (assigned by DHCP, check Serial Monitor)
*   **Modes:** Capture, View Gallery, Delete All
