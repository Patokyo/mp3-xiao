//for oled screen
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
// SH1107 128x128 I2C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET    -1 // No reset pin
#define I2C_ADDRESS   0x3C // Most common I2C address for SH1107
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//for sd card reader
#include <SPI.h>
#include <SD.h>
#define SD_CS 43  // Chip Select

//for rotary encoder
#include <RotaryEncoder.h>
// Pin definitions
const int pinCLK = 1;
const int pinDT  = 2;
// Rotary encoder in half-step mode
RotaryEncoder encoder(pinCLK, pinDT, RotaryEncoder::LatchMode::TWO03);
long lastPosition = 0;
//buttons
#define BUTTON_SELECT 3

//variables
String selectedAlbumName;
int selectedAlbumIndex = 1;
int albumAmount;
int mode = 0; //(0 -> selecting album, 1 -> playing album, 2 -> playing shuffle)

void setup() {
  Serial.begin(115200);
  display.begin();
  SD.begin(SD_CS);

  pinMode(BUTTON_SELECT, INPUT_PULLUP);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  displayAlbums();
}

void loop() {
  //rotary encoder
  encoder.tick();
  long pos = encoder.getPosition();
  Serial.println(pos);
  if (pos != lastPosition) {
    lastPosition = pos;
    if(mode==0){
      selectedAlbumIndex = pos;
      displayAlbums();
    }
    if(mode==1){
      //
    }
  }

  //button
  if (digitalRead(BUTTON_SELECT) == LOW) {
    if(mode==0){
      displaySongs();
      mode = 1;
      delay(200); // debounce
    }
  }

}
