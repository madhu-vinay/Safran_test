# About the uploaded code
Scope- transfer the data between the devices and sync up regurarly

This code uses the ESPNOW protocol (wireless) and UART (wired) communication for transfering the data between the devices.

The data will be sent to other device when the present device senses a data.

We can also activate or deativate the communication protocols ESPNOW and UART using by changing the define to 0.
 For ESPNOW-
     line no.27 #define espnow 0 for off and 1 for on.
 For UART-
     line no.30 #define uart 0 for off and 1 for on.

For ESPNOW we have to mention the MAC address of the devices to which the data has to be sent.
  In line no.49 broadcastAddress1[] should contain the MAC address of the device to which the data to be sent.
  
Future Scope-
1. We can add a GATT code to give the details of MAC address of the other device over BLE instead of giving manually in the code.
2. Use MQTT so that the device can sync anywhere around the world.
# Espressif IoT Development Framework

## Setting Up ESP-IDF

See https://idf.espressif.com/ for links to detailed instructions on how to set up the ESP-IDF depending on chip you use.

**Note:** Each SoC series and each ESP-IDF release has its own documentation. Please see Section [Versions](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/versions.html) on how to find documentation and how to checkout specific release of ESP-IDF.

# Quick Reference

See the Getting Started guide links above for a detailed setup guide. This is a quick reference for common commands when working with ESP-IDF projects:

## Setup Build Environment

(See the Getting Started guide listed above for a full list of required steps with more details.)

* Install host build dependencies mentioned in the Getting Started guide.
* Run the install script to set up the build environment. The options include `install.bat` or `install.ps1` for Windows, and `install.sh` or `install.fish` for Unix shells.
* Run the export script on Windows (`export.bat`) or source it on Unix (`source export.sh`) in every shell environment before using ESP-IDF.

## Configuring the Project

* `idf.py set-target <chip_name>` sets the target of the project to `<chip_name>`. Run `idf.py set-target` without any arguments to see a list of supported targets.
* `idf.py menuconfig` opens a text-based configuration menu where you can configure the project.

## Compiling the Project

`idf.py build`

... will compile app, bootloader and generate a partition table based on the config.

## Flashing the Project

When the build finishes, it will print a command line to use esptool.py to flash the chip. However you can also do this automatically by running:

`idf.py -p PORT flash`

Replace PORT with the name of your serial port (like `COM3` on Windows, `/dev/ttyUSB0` on Linux, or `/dev/cu.usbserial-X` on MacOS. If the `-p` option is left out, `idf.py flash` will try to flash the first available serial port.

This will flash the entire project (app, bootloader and partition table) to a new chip. The settings for serial port flashing can be configured with `idf.py menuconfig`.

You don't need to run `idf.py build` before running `idf.py flash`, `idf.py flash` will automatically rebuild anything which needs it.

## Viewing Serial Output

The `idf.py monitor` target uses the [idf_monitor tool](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/idf-monitor.html) to display serial output from Espressif SoCs. idf_monitor also has a range of features to decode crash output and interact with the device. [Check the documentation page for details](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/idf-monitor.html).

Exit the monitor by typing Ctrl-].

To build, flash and monitor output in one pass, you can run:

`idf.py flash monitor`
