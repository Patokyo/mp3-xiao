#include "Arduino.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

// Digital I/O used
#define SD_CS          43
#define SPI_MOSI      9
#define SPI_MISO      8
#define SPI_SCK       7
#define I2S_DOUT      2
#define I2S_BCLK      1
#define I2S_LRC       3

Audio audio;

void my_audio_info(Audio::msg_t m) {
    Serial.printf("%s: %s\n", m.s, m.msg);
}

void setup() {
    Audio::audio_info_callback = my_audio_info;
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    Serial.begin(115200);
    SD.begin(SD_CS);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(12);
    audio.connecttoFS(SD, "07 Cherub.mp3");
}

void loop(){
    vTaskDelay(1);
    audio.loop();
}

