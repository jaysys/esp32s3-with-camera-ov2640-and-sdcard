
// Adafruit NeoPixel - pinmap 48번

/*
 * Project: ESP32-S3-N16R8 Onboard RGB LED Testing
 * Created by: KES TECH (YouTube Channel)
 * Description: Basic RGB color cycling for onboard WS2812B LED (GPIO 48)
 */

#include <Adafruit_NeoPixel.h>

// Onboard RGB LED Configuration
#define PIN        48   // ESP32-S3 onboard LED Pin
#define NUMPIXELS   1   // Number of LEDs

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.println("--- Welcome to KES TECH ---");
  Serial.println("Starting RGB LED Test...");
  
  pixels.begin(); 
  pixels.setBrightness(50); // Brightness 0-255 (Keep it low to protect eyes)
}

void loop() {
  // RED Color
  Serial.println("Color: RED");
  pixels.setPixelColor(0, pixels.Color(255, 0, 0)); 
  pixels.show();
  delay(1000);

  // GREEN Color
  Serial.println("Color: GREEN");
  pixels.setPixelColor(0, pixels.Color(0, 255, 0)); 
  pixels.show();
  delay(1000);

  // BLUE Color
  Serial.println("Color: BLUE");
  pixels.setPixelColor(0, pixels.Color(0, 0, 255)); 
  pixels.show();
  delay(1000);

  // Channel Shoutout in Serial Monitor
  Serial.println("Subscribe to KES TECH for more projects!");
}
