## An mp3 player using a Seeed Studio Xiao Esp32-S3 microcontroller

# Components:
Seeed Studio Xiao Esp32-S3  
DAC Decoder with 3.5mm jack - PCM5102A  
Micro-SD card reader module  
OLED display - 1.5 inch, 128x128, SH1107, 4 pin  
Rotary encoder with push button  

# --UI--
## Modes:
Playing(folder, mode) - plays songs in the folder using the mode given, e.g. shuffle, straight  
Playlist/Album select - option between playlist/album, then displays all the playlists/albums on the sd card  

### Playing: mode 0
OLED display - current folder, currently playing song, progress bar, rewind, skip & play controls.  
Rotary encoder - Rotate for volume, press to pause, double press to skip, press and rotate back to rewind, press and rotate forward to enter playlist/album select  

### Playlist/Album select: mode 1
OLED display - displays folders/files in current directory, e.g. Playlists/Albums to start, then shows all Playlist files or Album folders  
Rotary encoder - Rotate to browse folders/files, press to select  


