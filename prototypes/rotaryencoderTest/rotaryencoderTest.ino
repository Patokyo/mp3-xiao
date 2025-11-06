#include <RotaryEncoder.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Pin definitions
const int pinCLK = 1;
const int pinDT  = 2;
const int pinSW  = 3;

// Rotary encoder in half-step mode
RotaryEncoder encoder(pinCLK, pinDT, RotaryEncoder::LatchMode::TWO03);
long lastPosition = 0;

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET    -1   // Reset not used on SH1107 breakout
Adafruit_SH1107 display(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire);

// Volume state
int volume = 5;         // start in middle
bool muted = false;
const int MIN_VOL = 0;
const int MAX_VOL = 10;

void setup() {
    Serial.begin(115200);
    pinMode(pinSW, INPUT_PULLUP);

    // Init OLED
    if (!display.begin(0x3C, true)) {
        Serial.println("SH1107 not found");
        for (;;);
    }
    display.clearDisplay();
    display.display();

    Serial.println("Rotary encoder volume bar started.");
}

void drawVolumeBar(int vol, bool muted) {
    display.clearDisplay();

    // Title
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Volume");

    if (muted) {
        // Show MUTE
        display.setTextSize(2);
        display.setCursor(0, 20);
        display.println("MUTED");
    } else {
        // Draw bar outline
        int barX = 10;
        int barY = 50;
        int barW = 108;
        int barH = 20;

        display.drawRect(barX, barY, barW, barH, SH110X_WHITE);

        // Filled portion
        int fillW = map(vol, MIN_VOL, MAX_VOL, 0, barW - 2);
        display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SH110X_WHITE);

        // % text
        display.setTextSize(2);
        display.setCursor(35, 90);
        display.print(vol);
        display.print("%");
    }

    display.display();
}

void loop() {
    encoder.tick();

    // Adjust volume with encoder
    long pos = encoder.getPosition();
    if (pos != lastPosition) {
        long diff = pos - lastPosition;
        lastPosition = pos;

        if (!muted) {
            volume += diff;
            if (volume < MIN_VOL) volume = MIN_VOL;
            if (volume > MAX_VOL) volume = MAX_VOL;
        }

        Serial.print("Volume: ");
        Serial.println(volume);

        drawVolumeBar(volume, muted);
    }

    // Button press → mute toggle
    if (digitalRead(pinSW) == LOW) {
        muted = !muted;
        Serial.println(muted ? "Muted" : "Unmuted");
        drawVolumeBar(volume, muted);
        delay(200); // debounce
    }
}
