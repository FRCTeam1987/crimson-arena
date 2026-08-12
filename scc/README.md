# R7SCC Setup Guide

## What you'll need

- 3x [Waveshare ESP32-S3-ETH-M](https://www.waveshare.com/esp32-s3-eth.htm?sku=34882)
- 3x microSD cards (32GB or smaller, less than 1MB is used)
- 3x Ethernet cables
- USB-C cable
- Chrome, Edge, or Brave browser

## Instructions

TODO: Add details on wiring the e-stop buttons

1. Format SD card as FAT32 with 512 byte sectors
2. Create scc.cfg on the SD card and set the location:
   ```env
   # location of SCC. valid values are RED, BLUE, and SCORING
   location=RED
   ```
3. Go to [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/)
4. If using Brave browser, turn off Brave Shields
5. Set the baud rate at the top to `921600`
6. Plug the ESP32 into computer
   - If the board doesn't connect, hold **BOOT**, press **RESET**, keep holding **BOOT**, then click **Connect** and release **BOOT** once it connects
7. Go to the flash tab
8. Flash the [firmware.factory.bin](#) with offset `0x0`
9. Click disconnect and unplug the ESP32
10. Connect the ESP32 to the FMS
11. Plug in USB-C power supply
12. Connect computer to FMS. Check the SCC status page at [10.0.100.5:8080](http://10.0.100.5:8080/)