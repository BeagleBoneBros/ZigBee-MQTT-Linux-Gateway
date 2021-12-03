#! /bin/sh

pkill -f node main.js
sleep 2
echo -e "\nRestarting main.js...\n"
node main.js