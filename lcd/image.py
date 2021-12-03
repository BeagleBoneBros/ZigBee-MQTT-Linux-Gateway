# Copyright (c) 2014 Adafruit Industries
# Author: Tony DiCola
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
from PIL import Image, ImageDraw, ImageFont

import ST7735 as TFT
import Adafruit_GPIO as GPIO
import Adafruit_GPIO.SPI as SPI
import time
import json
import os
import subprocess
import re


WIDTH = 128
HEIGHT = 160
SPEED_HZ = 4000000
NUM_SCREENS = 2

def display_devices():
    # create image to draw to
    image = Image.new("RGB", ( 160, 128 ), "black")
    draw = ImageDraw.Draw(image)
    # open JSON file lising devices to be displayed
    with open("../source/Projects/node/nodejs_zb_gateway/devices.json") as read_file:
        data = json.load(read_file)
    draw.text((10, 0), "Devices", fill=(255, 255, 0) )
    # offset for the next line
    offset = 1
    #Extract list of devices from JSON payload
    for i in data:
        draw.text((10, offset * 10), str(i["data"]["guid"]))
        #increment offset
        offset += 1
    loadAndDisp(image)

def display_status():
    # create image to draw to
    image = Image.new("RGB", ( 160, 128 ), "black")
    draw = ImageDraw.Draw(image)
    # open JSON file to be displayed
    with open("../source/Projects/node/nodejs_zb_gateway/status.json") as read_file:
        data = json.load(read_file)

    #Extract from JSON payload
    draw.text((10, 0), "Status", fill=(255, 255, 0) )
    draw.text((10, 1 * 10), "Broker Up: " + str(data["MQTTConnected"]))
    draw.text((10, 2 * 10), "User: " + str(data["username"]))
    draw.text((10, 3 * 10), "Pass: " + str(data["password"]))
    draw.text((10, 4 * 10), "Client ID: " + str(data["client_Id"]))
    draw.text((10, 5 * 10), "Broker IP: " + str(data["broker_ip"]))
    loadAndDisp(image)

def display_interfaces():
    image = Image.new("RGB", ( 160, 128 ), "black")
    draw = ImageDraw.Draw(image)
    get_if_list = """#!/bin/bash\n
    ifconfig -s | grep -E \"^[a-zA-Z]+[0-9]\" | cut -d \" \" -f 1\n"""

    file2write = open("display_if.sh", 'w')
    file2write.write(get_if_list)
    file2write.close( )

    # use subprocess to run bash script to get interface list
    interfaces = subprocess.Popen( ["sh","./display_if.sh"],
     stdout = subprocess.PIPE ).communicate()[0]

    interfaces = interfaces.decode('utf-8')
    # remove trailing white spaces
    interfaces = interfaces.rstrip()

    # split the interface into an array of strings,
    interfaces = re.split( '\n', interfaces )

    # offset for the next line
    offset = 1
    draw.text((10, 0), "Network and IP", fill=(255, 255, 0) )
    for interface in interfaces:
        # get the interface information for each interface
        out = subprocess.Popen( [ 'ifconfig', interface ],
          stdout = subprocess.PIPE ).communicate()[0]

        # get the ip of the interface using regex
        ip = re.search( '[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}',
          out.decode('utf-8') )
        # print each interface
        if ip is not None:
            draw.text((10, offset * 10), interface + ": " + ip.group())
        else:
            draw.text((10, offset * 10), interface + ": ")

        #increment offset
        offset += 1
    loadAndDisp(image)
    os.remove("display_if.sh")

# load, resize and display the image
def loadAndDisp(image):
    # load image object
    _image = image
    # Resize the image and rotate it so matches the display.
    _image = _image.rotate(90, expand=1).resize((WIDTH, HEIGHT))
    # Draw the image on the display hardware.
    disp.display(_image)

    # Load default font.
font = ImageFont.load_default()



    

# Raspberry Pi configuration.
#DC = 24
#RST = 25
#SPI_PORT = 0
#SPI_DEVICE = 0

# BeagleBone Black configuration.
DC = 'P9_15'
RST = 'P9_12'
SPI_PORT = 1
SPI_DEVICE = 0

# Create TFT LCD display class.
disp = TFT.ST7735(
    DC,
    rst=RST,
    spi=SPI.SpiDev(
        SPI_PORT,
        SPI_DEVICE,
        max_speed_hz=SPEED_HZ))

# Initialize display and GPIO
disp.begin()
GPIO = GPIO.get_platform_gpio()
GPIO.setup("P9_14", 1)
# Display Splash screen for 5 seconds
image = Image.open('splash.jpg')
loadAndDisp(image)
time.sleep(1)

currentScreen = 2
old_button_state = 0
while True:
    new_button_state = GPIO.input("P9_14")
    if new_button_state == 1 and old_button_state == 0 :
        if currentScreen >= NUM_SCREENS :
            currentScreen = 0
        else :
            currentScreen += 1
        # write the corrent screen data to the display
        print("Current Screen is:",currentScreen)
        if currentScreen is 0 :
            display_devices()
        # Network and IP Screen
        elif currentScreen is 1 :    
            display_interfaces()
        elif currentScreen is 2 :
            display_status()  
        time.sleep(0.1)
    old_button_state = new_button_state





