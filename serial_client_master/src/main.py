"""2-way communication client for the final project of Computer Systems-course in University of Oulu using usb and Morse code."""
import os
import sys
import time
import serial
import threading
import queue
import colorama
import re

import morse
import device_manager

colorama.init() #allows the use of ANSI escape codes in cmd on Windows (\033[K, \033[F)

user_input = queue.Queue()
input_buffer_size = None

def get_user_input():
    """Get the input from the console and add it to the user_input queue. Special commands are .clear and .exit"""
    while not restart_event.is_set():
        print('>', end='')
        inp = input()
        if inp == '.exit':
            exit_event.set() #signal to main to exit
            restart_event.set() #signal to threads to stop
        elif inp == '.clear':
            print('\033[2J\033[H', end='') #clear the terminal and move cursor to upper left corner
            continue
        user_input.put(inp)
        print('\033[F\033[K', end='') #clear line to hide the user input after pressing enter

def send_data(usb):
    """Get data from the user_input queue. Encode the data to morse and send by one character over usb."""
    while not restart_event.is_set():
        try:
            time.sleep(0.3)# Wait 300ms
            if not user_input.empty():
                morse_string = morse.encode(user_input.get())
                for c in morse_string:
                    # Send spaces also. No time. 
                    #if c == ' ': #don't send spaces
                    #    time.sleep(1)
                    #    continue
                    usb.write(f"{c}".encode('utf-8'))
                    time.sleep(0.005) # wait 5ms
                usb.write(b"  \n") #Add the end of line
        except serial.SerialException:
            print("Error sending data\n")
            restart_event.set() #if device is disconnected signal threads to stop 

def get_data(usb):
    """Get the data from usb. Print to the console. Accumulate data in the message variable, removing debug parts (marked with _ and \0). If 3 spaces are met decode the message and print it to console."""
    message = ''
    while not restart_event.is_set():
        time.sleep(0.1)
        try:
            incoming = usb.read(input_buffer_size).decode('utf-8', errors='ignore')
            if incoming:
                incoming = incoming.replace('\r\n', '\n')  # normalize line endings
                print(f'\r\033[K» {incoming if incoming.endswith("\n") else incoming + "\n"}>', end='') # print raw text clear current line before printing
                message += incoming
                message = re.sub(r'__.*?__', '', message) # Remove debug message. Debug message is anything between __ ___
                if '  \n' in message: #2 spaces and \n
                    print('\r\033[K', '\U0001F524', morse.decode(message), '\n\n>', end='')  # clear current line before printing decoded text
                    message = ''
        except serial.SerialException:
            restart_event.set() #if device is disconnected signal threads to stop 

allowed_devices = None
restart_event = threading.Event()
exit_event = threading.Event()
def main():
    """Load the config. Enter main loop, where the connected device is found and communication threads are started. If device is disconnected the restart_event is set. Threads are stopped and the next loop iteration begins with looking for connected device."""
    global allowed_devices
    print('Loading config file')
    data = device_manager.parse_config()
    if not data: 
        print ('config.json could not be found or is not a valid json file')
        return
    allowed_devices = device_manager.load_allowed_devices(data)
    if allowed_devices is None:
        print('Invalid config.json file. Devices could not be loaded')
        return
    print('Config loaded with ', len(allowed_devices), ' devices')
    global input_buffer_size
    input_buffer_size = device_manager.load_input_buffer(data)

    while not exit_event.is_set():
        device = device_manager.find_connected_device(allowed_devices)
        usb = serial.Serial(device, 9600, timeout=0)
        print('Connected\n')

        input_thread = threading.Thread(target=get_user_input)
        send_data_thread = threading.Thread(target=send_data, args=(usb,))
        get_data_thread = threading.Thread(target=get_data, args=(usb,))

        input_thread.start()
        send_data_thread.start()
        get_data_thread.start()

        get_data_thread.join()
        input_thread.join(timeout=1)
        send_data_thread.join(timeout=1)
        
        print('\033[2J\033[H', end='') #clear the terminal and move cursor to upper left corner
        usb.close()
        restart_event.clear()

if __name__ == "__main__":
    main()
