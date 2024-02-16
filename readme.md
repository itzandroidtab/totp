## Hardware TOTP token
Open source hardware TOTP token. Currently targets the LPC1756, LPC1758 and LPC1759 (with a small change it should also run on the LPC1754). 

### Features
* keys never leave the token after setup
* supports up to 32 profiles
* USB mode to add/delete profiles without having the keys leave the token
* support to change the RTC calibration values in the settings
* support for setting the time

### Images
Photos
<img src='img/totp_test.jpg' width='500'>
<img src='img/full_front0.jpg' width='500'>
<img src='img/case.jpg' width='500'>

PCB
<img src='img/pcb.jpg' width='500'>
<img src='img/lpc1756_totp.png' width='500'>
<img src='img/lpc1756_totp_back.png' width='500'>

Additional pictures can be found [here](./img/)

### TODO
* encrypt profiles
* Add support for more than 32 profiles (needs a rework if more profiles are required. The profiles are copied to ram and there is not enough for more at the moment. If we leave them in flash we can store a lot more)

### Extra
Is intended to be used with [USB dfu bootloader](https://github.com/itzandroidtab/dfu_bootloader). To build without bootloader support delete `set(TARGET_LINKERSCRIPT ...)` from the `CMakeLists.txt` in this project.