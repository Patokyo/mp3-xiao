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
bool playing = true;
int count;
String chosenFile;
String currentSongPath;
String currentDirectory;

//bitmaps
// 'play', 16x16px
const unsigned char bitmap_play [] PROGMEM = {
	0xc0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0xff, 0x00, 0xff, 0xc0, 0xff, 0xf0, 0xff, 0xfc, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfc, 0xff, 0xf0, 0xff, 0xe0, 0xff, 0x80, 0xfe, 0x00, 0xf8, 0x00, 0xe0, 0x00
};



void setup() {
  Serial.begin(115200);
  display.begin();
  SD.begin(SD_CS);

  pinMode(BUTTON_SELECT, INPUT_PULLUP);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.println("Working");
  display.display();
  
  currentDirectory = "/";
  shufflePick();
}

void loop() {
  if(playing){
    playingUI();
  }
  else{

  }
  //rotary encoder
  encoder.tick();
  long pos = encoder.getPosition();
  if (pos != lastPosition) {
    lastPosition = pos;
  }

  //button
  if (digitalRead(BUTTON_SELECT) == LOW) {

  }

}

void playingUI(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(currentDirectory);
  display.println(currentSongPath);

  display.setTextSize(2);
  display.setCursor(0, 64);
  display.drawBitmap(56, 56, bitmap_play, 16, 16, SH110X_WHITE);
  display.display();
}

void shufflePick(){
  count = 0;
  shuffleDirectory(SD.open(currentDirectory));
  currentSongPath = chosenFile;
  Serial.println(currentSongPath);
}

void shuffleDirectory(File directory){
  File song;
  while((song = directory.openNextFile())){
    if(song.isDirectory()){
      shuffleDirectory(song);
    }
    else{
      count++;
      if(random(count)==0){
        chosenFile = song.name();
      }
    }
    song.close();
  }
}
