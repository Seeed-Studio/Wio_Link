#!/bin/bash

if [ -e ./pion_one_git_clone ]
then
    echo "Already cloned"
else
    echo "Now clone the repo..."
    git clone https://github.com/Seeed-Studio/Wio_Link.git pion_one_git_clone
    git submodule init
fi

cd ./pion_one_git_clone/
#git checkout dev
git pull
git submodule update
supervisorctl stop esp8266

if [ ! -d  ../esp8266_iot_node ]
then
    mkdir -p ../esp8266_iot_node
    cd ../esp8266_iot_node
    cp -rf ../pion_one_git_clone/* ./
    rm -rf ./grove_drivers/grove_example
else
    cd ../esp8266_iot_node
    mv database.db database.db.bak
    mv config.py config.py.bak
    cp -rf ../pion_one_git_clone/* ./
    mv database.db.bak database.db
    mv config.py.bak config.py
    rm -rf ./grove_drivers/grove_example
fi

python ./scan_drivers.py
supervisorctl start esp8266