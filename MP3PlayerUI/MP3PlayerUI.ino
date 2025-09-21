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

//for DAC decoder
#include "Audio.h"
// Digital I/O used
#define I2S_DOUT      4
#define I2S_BCLK      7
#define I2S_LRC       40
Audio audio;

//variables
bool playing = true;
bool paused = false;
int volume = 50;
int count;
String chosenFile;
String currentSongPath;
String currentArtist;
String currentTitle;
String currentDirectory;

//bitmaps
// 'play', 16x16px
const unsigned char bitmap_play [] PROGMEM = {
	0xc0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0xff, 0x00, 0xff, 0xc0, 0xff, 0xf0, 0xff, 0xfc, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfc, 0xff, 0xf0, 0xff, 0xe0, 0xff, 0x80, 0xfe, 0x00, 0xf8, 0x00, 0xe0, 0x00
};

// 'pause', 16x16px
const unsigned char bitmap_pause [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 
	0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x00, 0x00, 0x00, 0x00
};

// 'rewind', 16x16px
const unsigned char bitmap_rewind [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x40, 0x0c, 0x40, 0x3c, 0x40, 0xfc, 0x43, 0xfc, 0x4f, 0xfc, 0x7f, 0xfc, 
	0x7f, 0xfc, 0x4f, 0xfc, 0x43, 0xfc, 0x40, 0xfc, 0x40, 0x3c, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00
};

// 'skip', 16x16px
const unsigned char bitmap_skip [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x78, 0x04, 0x7e, 0x04, 0x7f, 0x84, 0x7f, 0xe4, 0x7f, 0xfc, 
	0x7f, 0xfc, 0x7f, 0xe4, 0x7f, 0x84, 0x7e, 0x04, 0x78, 0x04, 0x60, 0x04, 0x00, 0x00, 0x00, 0x00
};

String my_audio_info(Audio::msg_t m) {
    Serial.printf("%s: %s\n", m.s, m.msg);
    if (strcmp(m.s, "artist") == 0) {
      currentArtist = m.msg;
    }
    else if (strcmp(m.s, "title") == 0) {
      currentTitle = m.msg;
    }
}

void setup() {
  Serial.begin(115200);
  display.begin();
  SD.begin(SD_CS);

  pinMode(BUTTON_SELECT, INPUT_PULLUP);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.println("Initialising...");
  display.display();
  
  currentDirectory = "/";
  shufflePick();

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(12); // 0...21

  // play file from root of SD card
  Audio::audio_info_callback = my_audio_info;
  audio.connecttoFS(SD, "test.wav");
}

void loop() {
  audio.loop();
  
  //rotary encoder
  encoder.tick();
  long pos = encoder.getPosition();
  long diff = 0;
  if (pos != lastPosition) {
    diff = pos-lastPosition;
    lastPosition = pos;
  }

  //button
  if (digitalRead(BUTTON_SELECT) == LOW) {

  }

  if(playing){
    volume += diff*5;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    playingUI();
  }
  else{

  }
}

void playingUI(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(currentDirectory);
  display.setCursor(20, 30);
  display.println(currentSongPath.substring(3, currentSongPath.length() - 4));
  display.setCursor(20, 45);
  display.println(currentArtist);

 display.setCursor(0, 64);
  display.drawBitmap(20, 56, bitmap_rewind, 16, 16, SH110X_WHITE);

  if(paused){
    display.drawBitmap(56, 56, bitmap_play, 16, 16, SH110X_WHITE);
  }
  else{
    display.drawBitmap(56, 56, bitmap_pause, 16, 16, SH110X_WHITE);
  }

  display.drawBitmap(92, 56, bitmap_skip, 16, 16, SH110X_WHITE);
  display.drawRect(20, 92, 88, 15, SH110X_WHITE); //progress bar

  //volume bar
  display.drawRect(120, 20, 5, 100, SH110X_WHITE);
  display.fillRect(120, 20 + (100 - volume), 5, volume, SH110X_WHITE);


  display.display();
}

void shufflePick() {
	count = 0;
	chosenFile = "";
	shuffleDirectory(SD.open("/"), "");
	currentSongPath = chosenFile;
	Serial.println("Picked: " + currentSongPath);
}
 
void shuffleDirectory(File directory, String parentPath) {
	File song;
	while ((song = directory.openNextFile())) {
		String fullPath = parentPath + "/" + song.name();  // build full path
		if (song.isDirectory()) {
			shuffleDirectory(song, fullPath);              // recurse deeper
		} else {
			count++;
			if (random(count) == 0) {
				chosenFile = fullPath;                    // save full path
			}
		}
		song.close();
	}
}
