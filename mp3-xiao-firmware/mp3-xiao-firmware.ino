#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//OLED SDD1306 SETUP
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//SD CARD MODULE SETUP (SPI)
#define SD_CS 44 
#define SPI_MOSI 9 
#define SPI_MISO 8 
#define SPI_SCK 7 

//DAC SETUP (I2S)
#define I2S_DOUT 3
#define I2S_BCLK 4
#define I2S_LRC 2

//INPUT SETUP
#define DUO_PIN 1 
#define SINGLE_PIN 43 

//STRUCTS
struct SongInfo {
  String artist;
  String title;
  String album;
  String path;
  int index = 0;
  long length;
  unsigned long startTime;
  bool ended = false;
  bool skipped = false;
};

struct state {
  String mode = "play"; //play, select or charge
  String currentFolder = "/";
  bool shuffle = true; //false for straight
  String selectMode;
  int selectionIndex = 0;
  int indexLines;
  bool paused = false;
  int volume = 12;
  unsigned long timepassed = 0;
};

//VARIABLES
Audio audio;
state state;
SongInfo currentSong;
bool upPressed;
bool downPressed;
int debounce = 200;

#define MAX_ALBUMS 128
#define NAME_LEN 128
char albumPaths[MAX_ALBUMS][NAME_LEN];
int albumCount = 0;

#define MAX_PLAYLISTS 128
char playlistPaths[MAX_ALBUMS][NAME_LEN];
int playlistCount = 0;


//bitmaps
// 'play', 16x16px
const unsigned char bitmap_play[] PROGMEM = {
  0xc0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0xff, 0x00, 0xff, 0xc0, 0xff, 0xf0, 0xff, 0xfc, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfc, 0xff, 0xf0, 0xff, 0xe0, 0xff, 0x80, 0xfe, 0x00, 0xf8, 0x00, 0xe0, 0x00
};

// 'pause', 16x16px
const unsigned char bitmap_pause[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38,
  0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x1c, 0x38, 0x00, 0x00, 0x00, 0x00
};

// 'rewind', 16x16px
const unsigned char bitmap_rewind[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x40, 0x0c, 0x40, 0x3c, 0x40, 0xfc, 0x43, 0xfc, 0x4f, 0xfc, 0x7f, 0xfc,
  0x7f, 0xfc, 0x4f, 0xfc, 0x43, 0xfc, 0x40, 0xfc, 0x40, 0x3c, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00
};

// 'skip', 16x16px
const unsigned char bitmap_skip[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x78, 0x04, 0x7e, 0x04, 0x7f, 0x84, 0x7f, 0xe4, 0x7f, 0xfc,
  0x7f, 0xfc, 0x7f, 0xe4, 0x7f, 0x84, 0x7e, 0x04, 0x78, 0x04, 0x60, 0x04, 0x00, 0x00, 0x00, 0x00
};

//shuffle
const unsigned char bitmap_shuffle[] PROGMEM = {
	0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x7c, 0x3e, 0x02, 0x44, 0x02, 0x48, 0x01, 0x00, 0x01, 0x00, 
	0x00, 0x80, 0x00, 0x80, 0x02, 0x48, 0x02, 0x44, 0x7c, 0x3e, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00
};

// 'straight, 16x16px
const unsigned char bitmap_straight[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x30, 0x06, 0x18, 0x03, 0x0c, 0x7f, 0xfe, 
	0x7f, 0xfe, 0x03, 0x0c, 0x06, 0x18, 0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void audio_info(Audio::msg_t m) {
  String type = String(m.s);
  String message = String(m.msg);

  if (type == "id3data") {
    int colonIndex = message.indexOf(':');
    if (colonIndex > 0) {
      String field = message.substring(0, colonIndex);
      String value = message.substring(colonIndex + 2);

      if (field == "Artist") {
        currentSong.artist = value;
      } else if (field == "Title") {
        currentSong.title = value;
      } else if (field == "Album") {
        currentSong.album = value;
      } else if (field == "Length (ms)") {
        currentSong.length = value.toInt();
      }
    }
  } else if (type == "eof") {
    currentSong.ended = true;
  }
}

void setup() {
  Serial.begin(115200);

  Audio::audio_info_callback = audio_info;

  //SD CARD SETUP (SPI)
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(10000000);
  SD.begin(SD_CS);

  //OLED SETUP (I2C)
  Wire.begin(5,6);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  //DAC SETUP (I2S)
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(state.volume);

  //INPUT SETUP
  pinMode(SINGLE_PIN, INPUT_PULLUP);

  display.println("Initialising...");
  display.display();

  cacheAlbums();
  cachePlaylists();
  play();
}

void loop() {
  vTaskDelay(1);
  audio.loop();
  duoPin();

  if(digitalRead(SINGLE_PIN) == LOW && state.mode == "play"){
    state.paused = !state.paused;
    audio.pauseResume();
    playUI();

    unsigned long pressStart = millis();
    bool doublePressed = false;
    while (digitalRead(SINGLE_PIN) == LOW) {
        if ((millis() - pressStart) > 1000) {
          int index = 0;
          while (digitalRead(SINGLE_PIN) == LOW) {
            duoPin();
            if(upPressed){
              index -= 1;
              delay(200);
            } else if(downPressed){
              index += 1;
              delay(200);
            }
            if(index<0) index = 2;
            else if(index>2) index = 0;
            display.clearDisplay();
            display.setCursor(0, 0);
            if (index == 0) {
              display.print(">");
              display.println("Shuffle");
              display.println(" Albums");
              display.println(" Playlists");
            } else if (index == 1) {
              display.println(" Shuffle");
              display.print(">");
              display.println("Albums");
              display.println(" Playlists");
            } else if (index == 2) {
              display.println(" Shuffle");
              display.println(" Albums");
              display.print(">");
              display.println("Playlists");
            }
            display.display();
          }
          if (index == 0) {
            state.mode = "play";
            state.shuffle = true;
            state.currentFolder = "/";
            play();
          } else if (index == 1) {
            state.mode = "select";
            state.selectMode = "album";
            state.indexLines = albumCount;
          } else if (index == 2) {
            state.mode = "select";
            state.selectMode = "playlist";
            state.indexLines = playlistCount;
          }
          state.selectionIndex = 0;
          break;
        }
      }

    while(millis() - pressStart < 500){
      if(digitalRead(SINGLE_PIN) == LOW){
        doublePressed = true;

        while(digitalRead(SINGLE_PIN) == LOW);
        break;
      }
    }

    if(doublePressed){
      currentSong.skipped = true;
    }
  }

  if(state.mode == "play"){
    if(currentSong.ended || currentSong.skipped){
      playUI();
      play();
      currentSong.ended = false;
      currentSong.skipped = false;
    }
    if (upPressed){
      state.volume = constrain(state.volume + 1, 0, 21);
      delay(50);
    } else if (downPressed){
      state.volume = constrain(state.volume - 1, 0, 21);
      delay(50);
    }
    audio.setVolume(state.volume);

    playUI();
  } else if(state.mode == "select"){
    if(!upPressed && !downPressed){
      debounce = 200;
    } else if(upPressed){
      state.selectionIndex = constrain(state.selectionIndex - 1, 0, state.indexLines-1);
      delay(debounce);
      debounce = 30;
    } else if(downPressed){
      state.selectionIndex = constrain(state.selectionIndex + 1, 0, state.indexLines-1);
      delay(debounce);
      debounce = 30;
    }

    if(digitalRead(SINGLE_PIN) == LOW){
      File indexFile;
      if(state.selectMode == "album"){
        indexFile = SD.open("/albums.txt");
        state.shuffle = false;
        currentSong.index = 0;
      } else{
        indexFile = SD.open("/playlists.txt");
        state.shuffle = true;
      }
      state.currentFolder = findLine(indexFile, state.selectionIndex);
      indexFile.close();
      play();
    }
    
    selectUI();
  }

  if (!state.paused && !currentSong.ended) {
    state.timepassed = millis() - currentSong.startTime;
  }
}

String selectMode(){
  int index = 0;
  while (digitalRead(SINGLE_PIN) == LOW) {
    duoPin();
    if(upPressed){
      index = constrain(state.selectionIndex - 1, 0, 2);
      delay(200);
    } else if(downPressed){
      index = constrain(state.selectionIndex + 1, 0, 2);
      delay(200);
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    if (index == 0) {
      display.print(">");
      display.println("Shuffle");
      display.println(" Albums");
      display.println(" Playlists");
    } else if (index == 1) {
      display.println(" Shuffle");
      display.print(">");
      display.println("Albums");
      display.println(" Playlists");
    } else if (index == 2) {
      display.println(" Shuffle");
      display.println(" Albums");
      display.print(">");
      display.println("Playlists");
    }
    display.display();
  }

  if (index == 0) {
    state.mode = "play";
    state.shuffle = true;
    state.currentFolder = "/";
    play();
  } else if (index == 1) {
    return "album";
  } else if (index == 2) {
    return "playlist";
  }
  return "shuffle";
}

int countLines(File file) {
  int lines = 0;
  while (file.available()) {
    if (file.read() == '\n') lines++;
  }
  file.seek(0);  // rewind to start
  return lines;
}

void duoPin(){
  if(upPressed || downPressed){
    if(analogRead(DUO_PIN) > 3000){
      upPressed = false;
      downPressed = false;
    }
  }
  if (analogRead(DUO_PIN) < 1000){
    upPressed = true;
  } else if(analogRead(DUO_PIN) < 3000){
    downPressed = true;
  }
}

String findLine(File &file, int target) {
  file.seek(0);                      // make sure we start at beginning
  for (int i = 0; i <= target; i++) {
    String line = file.readStringUntil('\n');
    line.trim();                      // remove \r or whitespace
    if (i == target) return line;
  }
  return "";  // fallback
}

void play(){
  File indexFile = SD.open(state.currentFolder + "/index.txt");
  state.indexLines = countLines(indexFile);

  if(state.shuffle){
    int target = random(state.indexLines);
    currentSong.path = findLine(indexFile, target);
  } else{
    currentSong.path = findLine(indexFile, currentSong.index);
    currentSong.index++;
  }

  indexFile.close();

  state.mode = "play";
  audio.connecttoFS(SD, currentSong.path.c_str());
  if(state.paused) state.paused = false;
  currentSong.startTime = millis()+1500;
  state.timepassed = 0;
}

void playUI() {
  display.clearDisplay();
  display.setTextSize(1);

  if(state.shuffle){
    display.drawBitmap(0, 0, bitmap_shuffle, 16, 16, SSD1306_WHITE);
    display.setCursor(24, 2);
    String displayPath = state.currentFolder;
    if(displayPath.length()>1){
      displayPath.remove(0, 11);
      if(displayPath.length()>17){
        displayPath = displayPath.substring(0, 18);
        display.setCursor(20, 2);
      }
    } else{
      displayPath = "shuffle all";
      display.setCursor(26, 2);
    }
    display.print(displayPath);
  } else{
    display.drawBitmap(0, 0, bitmap_straight, 16, 16, SSD1306_WHITE);
    display.setCursor(24, 2);
    String albumName = currentSong.album;

    if (albumName.length() > 17) {
      albumName = albumName.substring(0, 18);
      display.setCursor(20, 2);
    }

    display.print(albumName);
  }

  // Center title
  int16_t x1, y1;
  uint16_t w, h;
  String displayTitle = currentSong.title;
  String displayArtist = currentSong.artist;
  if(displayTitle.length()>19){
    displayTitle = displayTitle.substring(0, 19);
  }
  display.getTextBounds(displayTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 20);
  display.println(displayTitle);
  display.setTextSize(1);
  display.setCursor(20, 32);
  if(displayArtist.length()>14){
    displayArtist = displayArtist.substring(0, 15);
  }
  display.println(displayArtist);

  //progress bar
  display.drawRect(20, 52, 88, 8, SSD1306_WHITE);
  state.timepassed = constrain(state.timepassed, 0, currentSong.length);
  int progressWidth = map(state.timepassed, 0, currentSong.length, 0, 88);
  display.fillRect(20, 52, progressWidth, 8, SSD1306_WHITE);

  display.fillRect(56, 48, 16, 16, SSD1306_BLACK);

  if(currentSong.skipped){
    display.drawBitmap(56, 48, bitmap_skip, 16, 16, SSD1306_WHITE);
  } else if(state.paused) {
    display.drawBitmap(56, 48, bitmap_play, 16, 16, SSD1306_WHITE);
  } else {
    display.drawBitmap(56, 48, bitmap_pause, 16, 16, SSD1306_WHITE);
  }

  

  //volume bar
  int filledHeight = map(state.volume, 0, 21, 0, 50);
  //display.drawRect(120, 20, 2, 60, SSD1306_WHITE);
  display.fillRect(124, 10 + (50 - filledHeight), 2, filledHeight, SSD1306_WHITE);


  display.display();
}

void cacheAlbums() {
  albumCount = 0;  // reset

  File file = SD.open("/albums.txt");

  while (file.available() && albumCount < MAX_ALBUMS) {
    String line = file.readStringUntil('\n');
    line.trim();  // remove \r or spaces

    if (line.length() > 0) {
      // Copy into albumPaths buffer
      strncpy(albumPaths[albumCount], line.c_str(), NAME_LEN - 1);
      albumPaths[albumCount][NAME_LEN - 1] = '\0';  // ensure null termination
      albumCount++;
    }
  }

  file.close();
}

void cachePlaylists() {
  playlistCount = 0;

  File file = SD.open("/playlists.txt");

  while(file.available() && playlistCount < MAX_PLAYLISTS){
    String line = file.readStringUntil('\n');
    line.trim();

    if(line.length()>0){
      strncpy(playlistPaths[playlistCount], line.c_str(), NAME_LEN - 1);
      playlistPaths[playlistCount][NAME_LEN - 1] = '\0';
      playlistCount++;
    }
  }

  file.close();
}

void selectUI() {
  int count;
  char (*paths)[NAME_LEN];  // pointer to array of NAME_LEN chars
  int remove;

  if (state.selectMode == "album") {
      count = albumCount;
      paths = albumPaths;
      remove = 8;
  } else if (state.selectMode == "playlist") {
      count = playlistCount;
      paths = playlistPaths;
      remove = 11;
  }

  display.clearDisplay();
  display.setTextSize(1);

  const int visibleRows = 8;  // how many rows can fit on screen
  const int centerRow = (visibleRows-1) / 2;

  // Figure out which album should be at the top
  int startIndex = state.selectionIndex - centerRow;
  if (startIndex < 0) startIndex = 0;
  if (startIndex > count - visibleRows) startIndex = max(0, count - visibleRows);

  // Draw the visible list
  for (int i = 0; i < visibleRows && (startIndex + i) < count; i++) {
    display.setCursor(0, i * 8);  // 8 px per row for text size 1
    if ((startIndex + i) == state.selectionIndex) {
      display.print(">");
    } else {
      display.print(" ");
    }
    String path = String(paths[startIndex + i]);
    path.remove(0, remove);
    // Truncate to 20 chars
    if (path.length() > 20) {
      path = path.substring(0, 20);
    }
    display.println(path);
  }

  display.display();
}