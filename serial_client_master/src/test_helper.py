"""Module to help in testing the main program. Reads the input from the user and sends to the main program. To open the serial between the main program and this program socat can be used:
    socat PTY,link=/app/ttyV0,raw,echo=0 PTY,link=/app/ttyV1,raw,echo=0
After that the src/device_manager.find_connected_device() function should be changed to return /app/ttyV1 immediately without looking for real USB devices"""
import serial
uart = serial.Serial('ttyV0', 9600, timeout=0)
while True:
    us = input()
    uart.write(f"{us}".replace('\\0', '\0').encode('utf-8'))
    incoming = uart.read(100).decode('utf-8', errors='ignore')
    print(incoming.encode())

