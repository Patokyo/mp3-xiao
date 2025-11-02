#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin (or -1 if sharing Arduino reset)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Digital I/O used
#define SD_CS 44 
#define SPI_MOSI 9 
#define SPI_MISO 8 
#define SPI_SCK 7 
#define I2S_DOUT 3
#define I2S_BCLK 4
#define I2S_LRC 2
#define DUO_PIN 1 
#define SINGLE_PIN 43 

Audio audio;

//variables
String UIMode = "play";       //play or select
String playMode = "shuffle";  //shuffle, straight
String selectMode;            //album, playlist

bool paused = false;
bool songEnd = false;

File currentAlbum;
String shuffleDir = "/";
String shuffleNext;

bool upPressed;
bool downPressed;
int volume = 12;
int selectionIndex = 0;

String currentSongPath;
String currentAlbumDir;

String displayArtist;
String displayTitle;
String displayAlbum;
long songLength;
unsigned long songStartTime = 0;  // when the song started
unsigned long timepassed = 0;     // elapsed time in ms

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
      } else if (field == "Length (ms)") {
        songLength = value.toInt();
      }
    }
  } else if (type == "eof") {
    songEnd = true;
  }
}


void setup() {
  Audio::audio_info_callback = my_audio_info;
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(10000000);
  Serial.begin(115200);
  SD.begin(SD_CS);
  Wire.begin(5,6);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume);
  pinMode(SINGLE_PIN, INPUT_PULLUP);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("Initialising...");
  display.display();

  shufflepick(shuffleDir);
  shuffle();
  cacheAlbums();
  cachePlaylists();
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


void loop() {
  vTaskDelay(1);
  audio.loop();

  duoPin();

  if (digitalRead(SINGLE_PIN) == LOW) {
    if (UIMode == "play") {
      paused = !paused;  // toggle pause/play
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
            UIMode = "play";
            playMode = "shuffle";
            shuffleDir = "/";
            shuffle();
          } else if (index == 1) {
            UIMode = "select";
            selectMode = "album";
          } else if (index == 2) {
            UIMode = "select";
            selectMode = "playlist";
          }
          break;
        }
      }

      while (millis() - pressStart < 500) {
        if (digitalRead(SINGLE_PIN) == LOW) {
          doublePressed = true;
          // wait for release to avoid retriggering
          while (digitalRead(SINGLE_PIN) == LOW)
            ;
          break;
        }
      }

      if (doublePressed) {
        songEnd = true;  // double press action
        display.setCursor(0, 120);
        display.println("Skipped!");
        display.display();
        return;
      }
    }
  }



  if (UIMode == "play") {
    if (songEnd) {
      if (playMode == "shuffle") {
        shuffle();
      } else if (playMode == "straight") {
        nextSong();
      }
      songEnd = false;
    }
    
    if(upPressed){
      if(volume != 21) volume+=1;
      audio.setVolume(volume);
      delay(100);
    }  else if(downPressed){
      if(volume!=0) volume-=1;
      audio.setVolume(volume);
      delay(100);
    }
    playUI();
  } else if (UIMode == "select") {
    if(selectMode=="album"){
      if(upPressed){
        selectionIndex -= 1;
        delay(100);
      } else if(downPressed){
        selectionIndex += 1;
        delay(100);
      }
      if(selectionIndex > albumCount-1) selectionIndex = 0;
      else if(selectionIndex < 0) selectionIndex = albumCount-1;
      if (digitalRead(SINGLE_PIN) == LOW) {
        playAlbum(String(albumPaths[selectionIndex]));
        playMode = "straight";
        UIMode = "play";
        delay(200);
      }
      selectUI();
    } else if(selectMode=="playlist"){
      if(upPressed){
        selectionIndex += 1;
        delay(200);
      } else if(downPressed){
        selectionIndex -= 1;
        delay(200);
      }
      if(selectionIndex > playlistCount-1) selectionIndex = 0;
      else if(selectionIndex < 0) selectionIndex = playlistCount-1;
      if (digitalRead(SINGLE_PIN) == LOW) {
        shuffleDir = "/" + String(playlistPaths[selectionIndex]);
        UIMode = "play";
        shufflepick(shuffleDir);
        shuffle();
        delay(200);
      }
      selectUI();
    }
    

  }

  if (!paused && !songEnd) {
    timepassed = millis() - songStartTime;
  }
}

int countLines(File file) {
  int lines = 0;
  while (file.available()) {
    if (file.read() == '\n') lines++;
  }
  file.seek(0);  // rewind to start
  return lines;
}

String pickRandomLine(File file, int totalLines) {
  int target = random(totalLines);  // 0 .. totalLines-1
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
  return line;  // fallback for last line
}

void shufflepick(String directory) {
  playMode = "shuffle";
  File indexFile = SD.open(directory + "/index.txt");
  int totalSongs = countLines(indexFile);
  String currentSongPath = pickRandomLine(indexFile, totalSongs);
  indexFile.close();
  shuffleNext = currentSongPath;
}

void shuffle() {
  audio.connecttoFS(SD, shuffleNext.c_str());
  if (paused) {
    paused = false;
  }
  songStartTime = millis();
  timepassed = 0;
  shufflepick(shuffleDir);
}

void playAlbum(String albumDir) {
  currentAlbumDir = albumDir;
  currentAlbum = SD.open("/" + albumDir);
  File currentFile = currentAlbum.openNextFile();
  currentSongPath = albumDir + "/" + currentFile.name();
  audio.connecttoFS(SD, currentSongPath.c_str());
  if (paused) {
    paused = false;
  }
  songStartTime = millis();
  timepassed = 0;
}

void nextSong() {
  File currentFile = currentAlbum.openNextFile();
  audio.connecttoFS(SD, (String(currentAlbumDir) + "/" + currentFile.name()).c_str());
  if (paused) {
    paused = false;
  }
  songStartTime = millis();
  timepassed = 0;
}

void playUI() {
  display.clearDisplay();
  display.setTextSize(1);

  if(playMode=="shuffle"){
    display.drawBitmap(0, 0, bitmap_shuffle, 16, 16, SSD1306_WHITE);
    display.setCursor(24, 2);
    String displayPath = shuffleDir;
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
    String albumName = currentAlbum.name();

    if (albumName.length() > 17) {
      albumName = albumName.substring(0, 18);
      display.setCursor(20, 2);
    }

    display.print(albumName);
  }

  // Center title
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(displayTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 20);
  if(displayTitle.length()>19){
    displayTitle = displayTitle.substring(0, 19);
  }
  display.println(displayTitle);
  display.setTextSize(1);
  display.setCursor(20, 32);
  if(displayArtist.length()>14){
    displayArtist = displayArtist.substring(0, 15);
  }
  display.println(displayArtist);

  display.drawBitmap(32, 48, bitmap_rewind, 16, 16, SSD1306_WHITE);

  if (paused) {
    display.drawBitmap(56, 48, bitmap_play, 16, 16, SSD1306_WHITE);
  } else {
    display.drawBitmap(56, 48, bitmap_pause, 16, 16, SSD1306_WHITE);
  }

  display.drawBitmap(80, 48, bitmap_skip, 16, 16, SSD1306_WHITE);

  //progress bar
  display.drawRect(20, 100, 44, 4, SSD1306_WHITE);
  int progressWidth = map(timepassed, 0, songLength, 0, 44);  // 88 is bar width
  display.fillRect(20, 100, progressWidth, 15, SSD1306_WHITE);

  //volume bar
  int filledHeight = map(volume, 0, 21, 0, 50);
  //display.drawRect(120, 20, 2, 60, SSD1306_WHITE);
  display.fillRect(120, 10 + (50 - filledHeight), 2, filledHeight, SSD1306_WHITE);


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

  if (selectMode == "album") {
      count = albumCount;
      paths = albumPaths;
      remove = 7;
  } else if (selectMode == "playlist") {
      count = playlistCount;
      paths = playlistPaths;
      remove = 10;
  }

  display.clearDisplay();
  display.setTextSize(1);

  const int visibleRows = 8;  // how many rows can fit on screen
  const int centerRow = visibleRows / 2;

  // Figure out which album should be at the top
  int startIndex = selectionIndex - centerRow;
  if (startIndex < 0) startIndex = 0;
  if (startIndex > count - visibleRows) {
    startIndex = count - visibleRows;
  }
  if (startIndex < 0) startIndex = 0;  // in case albumCount < visibleRows

  // Draw the visible list
  for (int i = 0; i < visibleRows && (startIndex + i) < count; i++) {
    display.setCursor(0, i * 8);  // 8 px per row for text size 1
    if ((startIndex + i) == selectionIndex) {
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