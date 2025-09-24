#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


// OLED - SH1107 128x128 I2C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET    -1 // No reset pin
#define I2C_ADDRESS   0x3C // Most common I2C address for SH1107
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Digital I/O used
#define SD_CS         3
#define SPI_MOSI      9
#define SPI_MISO      8
#define SPI_SCK       7
#define I2S_DOUT      44
#define I2S_BCLK      1
#define I2S_LRC       2
#define POT_PIN       4
#define BUTTON_PIN    43

Audio audio;

//variables
String UIMode = "play"; //play or select 
String playMode = "shuffle"; //shuffle, straight
String selectMode; //album, playlist

bool paused = false;
bool songEnd = false;

File currentAlbum;
String shuffleDir = "/";

int volume = 12;
int selectionIndex;
int potValue;

String currentSongPath;
String currentAlbumDir;

String displayArtist;
String displayTitle;
String displayAlbum;
long songLength;
unsigned long songStartTime = 0;  // when the song started
unsigned long timepassed = 0;     // elapsed time in ms

#define MAX_ALBUMS 128
#define NAME_LEN   128
char albumPaths[MAX_ALBUMS][NAME_LEN];
int albumCount = 0;


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


void my_audio_info(Audio::msg_t m) {
    String type = String(m.s);
    String message = String(m.msg);

    if (type == "id3data") {
        int colonIndex = message.indexOf(':');
        if (colonIndex > 0) {
            String field = message.substring(0, colonIndex);
            String value = message.substring(colonIndex + 2);
            
            if (field == "Artist") {
                displayArtist = value;
            } else if (field == "Title") {
                displayTitle = value;
            } else if (field == "Album") {
                displayAlbum = value;
            } else if (field == "Length (ms)"){
                songLength = value.toInt();
            }
        }
    } else if(type=="eof"){
      songEnd = true;
    }
}


void setup() {
    Audio::audio_info_callback = my_audio_info;
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    Serial.begin(115200);
    SD.begin(SD_CS);
    display.begin();
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(volume);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.println("Initialising...");
    display.display();

    shuffle(shuffleDir);
    cacheAlbums();
    Serial.print("Free ram heap: ");
    Serial.println(ESP.getFreeHeap());
}

void loop(){
    vTaskDelay(1);
    audio.loop();

    if(digitalRead(BUTTON_PIN) == LOW) {
      if(UIMode=="play"){
        paused = !paused;   // toggle pause/play
        audio.pauseResume();
        playUI();

        unsigned long pressStart = millis();
        bool doublePressed = false;
        while(digitalRead(BUTTON_PIN) == LOW){
          if((millis()-pressStart)>1000){
            int index;
            while(digitalRead(BUTTON_PIN) == LOW){
              updatePot();
              index = map(potValue, 0, 4095, 0, 2);
              display.clearDisplay();
              display.setCursor(0,0);
              if(index==0){
                display.print(">");
                display.println("Shuffle");
                display.println(" Albums");
                display.println(" Playlists");
              } else if(index==1){
                display.println(" Shuffle");
                display.print(">");
                display.println("Albums");
                display.println(" Playlists");
              } else if(index==2){
                display.println(" Shuffle");
                display.println(" Albums");
                display.print(">");
                display.println("Playlists");
              }
              display.display();
            }
            if(index==0){
              UIMode = "play";
              playMode = "shuffle";
              shuffleDir = "/";
              shuffle(shuffleDir);
            } else if(index==1){
              UIMode = "select";
              selectMode = "album";
            } else if(index==2){
              UIMode = "select";
              selectMode = "playlist";
            }
            break;
          }
        }

        while(millis() - pressStart < 500){
            if(digitalRead(BUTTON_PIN) == LOW){
                doublePressed = true;
                // wait for release to avoid retriggering
                while(digitalRead(BUTTON_PIN) == LOW);
                break;
            }
        }

        if(doublePressed){
            songEnd = true;   // double press action
            display.setCursor(0, 0);
            display.println("Skipped!");
            display.display();
            return;
        }
      }  
    }

    if(UIMode=="play"){
      if(songEnd){
        if(playMode=="shuffle"){
          shuffle(shuffleDir);
        } else if(playMode=="straight"){
          nextSong();
        }
        songEnd = false;
      }
      updatePot();
      volume = map(potValue, 0, 4095, 0, 21); // 0-21 matches your audio.setVolume();
      audio.setVolume(volume);
      playUI();
    } else if(UIMode=="select"){
      updatePot();
      selectionIndex = map(potValue, 0, 4095, 0, albumCount-1);
      if(digitalRead(BUTTON_PIN) == LOW){
        playAlbum(String(albumPaths[selectionIndex]));
        playMode = "straight";
        UIMode = "play";
        delay(200);
      }
      selectUI();
    }

    if(!paused && !songEnd){
      timepassed = millis() - songStartTime;
    }
}

int countLines(File file) {
  int lines = 0;
  while (file.available()) {
    if (file.read() == '\n') lines++;
  }
  file.seek(0); // rewind to start
  return lines;
}

String pickRandomLine(File file, int totalLines) {
  int target = random(totalLines); // 0 .. totalLines-1
  int currentLine = 0;
  String line = "";
  
  while (file.available()) {
    char c = file.read();
    if (c == '\n') {
      if (currentLine == target) {
        return line;
      }
      line = "";
      currentLine++;
    } else {
      line += c;
    }
  }
  return line; // fallback for last line
}

void shuffle(String directory){
  playMode = "shuffle";
  File indexFile = SD.open(directory+"/index.txt");
  int totalSongs = countLines(indexFile);
  String currentSongPath = pickRandomLine(indexFile, totalSongs);
  indexFile.close();
  audio.connecttoFS(SD, currentSongPath.c_str());
  if(paused){
    paused=false;
  }
  songStartTime = millis();
  timepassed = 0;
}

void playAlbum(String albumDir){
  currentAlbumDir = albumDir;
  currentAlbum = SD.open("/" + albumDir);
  File currentFile = currentAlbum.openNextFile();
  currentSongPath = albumDir + "/" + currentFile.name();
  audio.connecttoFS(SD, currentSongPath.c_str());
  if(paused){
    paused=false;
  }
  songStartTime = millis();
  timepassed = 0;
}

void nextSong(){
  File currentFile = currentAlbum.openNextFile();
  audio.connecttoFS(SD, (String(currentAlbumDir) + "/" + currentFile.name()).c_str());
  if(paused){
    paused=false;
  }
  songStartTime = millis();
  timepassed = 0;
}

void updatePot() {
  int potReading = analogRead(POT_PIN); // 0 - 4095 on ESP32
  
  if (potReading != potValue) {  // only update if changed
      potValue = potReading;
  }
}

void playUI(){
  display.clearDisplay();
  display.setTextSize(1);

  // Center title
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(displayTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 30);
  display.println(displayTitle);
  display.setTextSize(1);
  display.setCursor(20, 56);
  display.println(displayArtist);

  display.drawBitmap(20, 72, bitmap_rewind, 16, 16, SH110X_WHITE);

  if(paused){
    display.drawBitmap(56, 72, bitmap_play, 16, 16, SH110X_WHITE);
  }
  else{
    display.drawBitmap(56, 72, bitmap_pause, 16, 16, SH110X_WHITE);
  }

  display.drawBitmap(92, 72, bitmap_skip, 16, 16, SH110X_WHITE);

  //progress bar
  display.drawRect(20, 92, 88, 15, SH110X_WHITE);
  int progressWidth = map(timepassed, 0, songLength, 0, 88); // 88 is bar width
  display.fillRect(20, 92, progressWidth, 15, SH110X_WHITE);

  //volume bar
  int filledHeight = map(volume, 0, 21, 0, 100);
  display.drawRect(120, 20, 5, 100, SH110X_WHITE);
  display.fillRect(120, 20 + (100 - filledHeight), 5, filledHeight, SH110X_WHITE);


  display.display();
}

void cacheAlbums() {
	albumCount = 0; // reset

	File file = SD.open("/albums.txt");

	while (file.available() && albumCount < MAX_ALBUMS) {
		String line = file.readStringUntil('\n');
		line.trim(); // remove \r or spaces

		if (line.length() > 0) {
			// Copy into albumPaths buffer
			strncpy(albumPaths[albumCount], line.c_str(), NAME_LEN - 1);
			albumPaths[albumCount][NAME_LEN - 1] = '\0'; // ensure null termination
			albumCount++;
		}
	}

	file.close();
}


void selectUI() {
	display.clearDisplay();
	display.setTextSize(1);

	const int visibleRows = 16;     // how many rows can fit on screen
	const int centerRow   = visibleRows / 2;  

	// Figure out which album should be at the top
	int startIndex = selectionIndex - centerRow;
	if (startIndex < 0) startIndex = 0;
	if (startIndex > albumCount - visibleRows) {
		startIndex = albumCount - visibleRows;
	}
	if (startIndex < 0) startIndex = 0; // in case albumCount < visibleRows

	// Draw the visible list
	for (int i = 0; i < visibleRows && (startIndex + i) < albumCount; i++) {
		display.setCursor(0, i * 8);  // 8 px per row for text size 1
		if ((startIndex + i) == selectionIndex) {
			display.print(">");
		} else {
			display.print(" ");
		}
		String path = String(albumPaths[startIndex + i]);
    path.remove(0, strlen("Albums/"));
    // Truncate to 20 chars
    if (path.length() > 20) {
      path = path.substring(0, 20);
    }
    display.println(path);
	}

	display.display();
}

