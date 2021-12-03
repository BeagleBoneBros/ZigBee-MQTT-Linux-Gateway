#!/bin/bash
#############################################################
# @file setup.sh
#
# @brief Completes series of steps to setup the source code
# to allow the Linux Zigbee Gateway and Node.js Zigbee
# Gateway server to run. 
#
# Group: WCS LPC
# $Target Devices: Linux: AM335x $
#
#############################################################
# $License: BSD3 2018 $
#
#   Copyright (c) 2018, Texas Instruments Incorporated
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#   *  Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#   *  Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#
#   *  Neither the name of Texas Instruments Incorporated nor the names of
#      its contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#############################################################
# $Release Name: ZStack Linux Gateway SDK ENG$
# $Release Date: May 07, 2018 $
#############################################################

echo "******************* BEGIN SETUP! ********************"
echo "This setup script will:"
echo "  - make all necessary scripts executable"
echo "  - ensure necessary files have UNIX line endings"
echo "  - remove prebuilt directories"
echo "  - install Node Modules for Node.js Zigbee Gateway"
echo "  - build Linux Zigbee Gateway"
echo "  - extract Linux Zigbee Gateway binaries"
echo "*****************************************************"

# Enter Node Directory with easy start scripts
if cd Projects/node/; then
    echo "Entered /Projets/node Directory"
else
    echo "ERROR: Unable to Find /Projects/node Directory"
    echo "Current directory is: "
    pwd
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# make all of these scripts executable
echo -e "\nMake the gateway scripts executable\n"
if chmod +x end.sh start_gateway.sh start_local.sh start_ibm.sh; then
    echo "SUCCESS - gateway scripts executable"
else
    echo "ERROR: Unable to make gateway scripts executable"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Return to original directory
cd -

# Remove the previous built directory if it exists
echo -e "\nRemove ERROR__out or out Directory if present\n"
if rm -r ERROR__out/ || rm -r out/; then
    echo "Removed Previous built package."
else    
    echo "Don't worry, there are no Previous built package to delete. Carry on."
fi

# convert all files from DOS to UNIX line endings. Omit converting files in the Node Zigbee Gateway due to it containing .js and .png files
echo -e "\nConvert all files from dos2unix \n"
if find . -type d -name "nodejs_zb_gateway" -prune -o -type f -print0 | xargs -0 dos2unix; then
    echo "SUCCESS: Converted Scripts to Unix Line endings"
else
    echo "ERROR: Failed running dos2unix. Make sure dos2unix is installed"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Enter Node.js Zigbee Gateway
if cd Projects/node/nodejs_zb_gateway; then
    echo "Entered /Projets/node/nodejs_zb_gateway Directory"
else
    echo "ERROR: Unable to Find /Projets/node/nodejs_zb_gateway Directory"
    echo "Current directory is: "
    pwd
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Install Node Modules
echo -e "\nInstalling Node Modules \n"
if npm install; then
    echo "SUCCESS: Installed all Node Modules Successfully"
else
    echo "ERROR: Failed installing Node Modules"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

#Install Python Packages (LCD)
echo -e "\nInstalling pip \n"
if apt-get install pip -y; then
    echo "SUCCESS: Installed pip Successfully"
else
    echo "ERROR: Failed installing pip"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

#Install PIL Python Package (LCD)
echo -e "\nInstalling PIL \n"
if pip install PIL; then
    echo "SUCCESS: Installed PIL Successfully"
else
    echo "ERROR: Failed installing PIL "
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

#Install Adafruit_GPIO Python Package (LCD)
echo -e "\nInstalling Adafruit_GPIO \n"
if pip install Adafruit_GPIO; then
    echo "SUCCESS: Installed Adafruit_GPIO Successfully"
else
    echo "ERROR: Failed installing Adafruit_GPIO"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Return to original directory
cd -

# Make the build all script executable
echo -e "\nMake build_all executable \n"
if chmod +x build_all; then
    echo "SUCCESS: build_all script is now executable"
else
    echo "ERROR: Unable to make build_all executable"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Make all contents in the scripts folder executable
echo -e "\nMake scripts/ executable \n"
if chmod +x scripts/*; then
    echo "SUCCESS: scripts/ folder is now executable"
else
    echo "ERROR: Unable to make scripts/ folder executable"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Build the Linux Zigbee Gateway C project
echo -e "\nRun ./build_all \n"
if ./build_all; then
    echo "SUCCESS: build_all finished."
else
    echo "ERROR: build_all failed."
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Enter the built project folder
if cd out/Precompiled/ || cd ERROR__out/Precompiled/; then
    echo "Entered Precompiled directory."
else
    echo "ERROR: ERROR__out/Precompiled or out/Precompiled/ directory could not be found."
    echo "Current directory is: "
    pwd
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi


# Unzip the binaries 
if tar -xvf ../Precompiled.tar || tar -xvf z-stack_linux_gateway_*.tar; then
    echo "SUCCESS: Extracted binaries"
else
    echo "ERROR: Could not extract the binaries or binary tar file could not be found"
    echo "Current directory is: "
    pwd
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Navigate to the servers folder
if cd servers/; then
    echo "Entered servers/ directory"
else
    echo "ERROR: Could not find the servers/ directory"
    echo "Current directory is: "
    pwd
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# Make all the server applications and tools executable
echo -e "\nMake apps exe, copy protobuf files \n"
if chmod +x zigbeeHAgw track_servers start_application; then
    echo "SUCCESS: Made servers and applications executable"
else
    echo "ERROR: Failed to make servers and applications executable"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

if chmod +x ../tools/*; then
    echo "SUCCESS: Made /tools/ folder executable"
else
    echo "ERROR: Failed to make /tools/ folder executable"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

# copy the necessary protobuf files to the user lib
echo -e "\nCopy Protobuf Files to /usr/lib directory\n"
if cp ../protobuf/libprotobuf-c.so.1.0.0 /usr/lib/; then
    echo "SUCCESS: Copied Protobuf Files"
else
    echo "Protobuf files not found"
    echo -e "\n****** SETUP FAILED! *****\n"
    exit 1
fi

echo -e "\n********** SETUP SUCCESSFUL! **********\n"
echo -e "\n --------- Getting Started Tips ----------- \n"
echo -e "Navigate to Node Scripts ==> cd Projects/node \n"
echo -e "Start Linux Zigbee Gateway and Node.js Zigbee Local Gateway ==> sudo ./start_local.sh \n"
echo -e "Start Linux Zigbee Gateway and Node.js IBM Gateway ==> sudo ./start_ibm.sh \n"
echo -e "Start Linux Zigbee Gateway Only ==> sudo ./start_gateway.sh \n"
echo -e "Kill All Gateways ==> sudo ./end.sh \n"
echo -e "--------------------------------------------- \n"

