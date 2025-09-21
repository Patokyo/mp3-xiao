//for oled screen
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

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

// SH1107 128x128 I2C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET    -1 // No reset pin
#define I2C_ADDRESS   0x3C // Most common I2C address for SH1107

Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//variables
String selectedAlbumName;
int selectedAlbumIndex = 0;
int albumAmount;

void setup() {
  // Start serial for debugging
  Serial.begin(115200);
  display.begin();
  SD.begin(SD_CS);

  //buttons
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
      long diff = pos - lastPosition;
      lastPosition = pos;
      Serial.println(pos);
      selectedAlbumIndex = pos;
      displayAlbums();
  }

  // Button press
    if (digitalRead(BUTTON_SELECT) == LOW) {
        displaySongs();
        delay(200); // debounce
    }
}

void displayAlbums() {
  albumAmount = 0;
  File root = SD.open("/");
  display.clearDisplay();
  display.setCursor(0,0);

  File album = root.openNextFile();

  int i = 0;
  while (album) {
    if (album.isDirectory()) {
      if(i==selectedAlbumIndex){
        selectedAlbumName = album.name();
        display.print(">");
      }
      else{
        display.print(" ");
      }
      char buf[21]; // 20 chars + null terminator
      strncpy(buf, album.name(), 20);
      buf[20] = '\0'; // make sure it's null-terminated
      display.println(buf);
      albumAmount++;
    }
    album.close();
    album = root.openNextFile();
    i++;
  }
  display.display();
  root.close();
}

void displaySongs(){
  File album = SD.open("/"+selectedAlbumName);

  display.clearDisplay();
  display.setCursor(0, 0);
  File song = album.openNextFile();
  while (song) {
    if (!song.isDirectory()) {
      display.println(song.name()); // print filename
    }
    song.close();
    song = album.openNextFile();
  }
  album.close();
  display.display();
}
