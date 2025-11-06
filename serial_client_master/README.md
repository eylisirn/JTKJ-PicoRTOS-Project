# serial_client

This is a small command line utility for the course Computer Systems in University of Oulu. It is used to transfer text strings in Morse code from the workstation to an embedded device running the final project program "Secret messages".

## How does it function

The program looks for the device configured in the `config.json` files and connected to the PC. It then starts serial communication with it. The messages typed in the program input are encoded with morse and sent to the device. The messages from the device are decoded and displayed in the console. 

## Quick start

To get up and running as fast as possible, here are quick instructions for using this program:

1. Enter the root directory of this project
2. Run `src/main.py`
3. If `colorama` and/or `pyserial` modules are missing install them using pip
4. Connect your device
5. To stop the program use `.exit` command or `Ctrl+C` twice

## Communitcation protocol

The program supports morse to lower case english alphabet encoding/decoding.

The characters encoded with morse should be separated by one space. The words are separated by two and the end of the message is two spaces and line feed (\n). Once the program detects the two spaces + line feed,  it proceeds to decode the message and print it to the console. If there are invalid characters they are replaced with `?` in the decoded string.

Everything that is placed between `__` and `__` characters is considered a debug message and excluded from the decoding.

## Boards

It is possible to use this program with multiple target boards (but not simultaneously!) running the final project program, or a program with a similar communication protocol.

The vendor and device id's for these boards are specified in `config.json`-file located in the root directory of this project. New boards can be added by following the existing format of the file. Currently, there are three supported boards, the Texas Instruments SensorTag, the Raspberry Pi Pico WH and the University of Oulu TKJHAT (when using the usb-debug library) included in the configuration.

**Note** Be careful when adding new board configurations, as the program will refuse to run if the JSON formatting is not correct, or there are attributes missing in the configuration file.

##Development

To test the program during development serial channel can be opened for example with the "socat" tool. More detailed description in the `src/test_helper.py`.
