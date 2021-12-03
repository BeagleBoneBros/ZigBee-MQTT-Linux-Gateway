#! /bin/sh

pkill -f node
sleep 5
echo -e "\nRestarting main.js...\n"
node main.js MQTTandLocal