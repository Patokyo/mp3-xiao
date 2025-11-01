# mp3 xiao  
An mp3 player using a Seeed Studio Xiao Esp32-S3 microcontroller

# Components:
Seeed Studio Xiao Esp32-S3  
DAC Decoder with 3.5mm jack - PCM5102A  
Micro-SD card reader module  
OLED display - SDD1306 128x64, 4 pin  
3 Push buttons

# UI
## Modes:
Playing(folder, mode) - plays songs in the folder using the mode given, e.g. shuffle, straight  
Playlist/Album select - option between playlist/album, then displays all the playlists/albums on the sd card  

### Playing: true
OLED display - current folder, currently playing song, progress bar, rewind, skip & play controls.  
Rotary encoder - Rotate for volume, press to pause, double press to skip, press and rotate back to rewind, press and rotate forward to enter playlist/album select  

### Playlist/Album select: false
OLED display - displays folders/files in current directory, e.g. Playlists/Albums to start, then shows all Playlist files or Album folders  
Rotary encoder - Rotate to browse folders/files, press to select  

website for making bitmap icons https://javl.github.io/image2cpp/

Values for resistor ladder: 4096 - no press
Max up: 363
Min up: 299
Max down: 1965
Min down: 1907
