#!/bin/bash

if [ -e ./wio_link_git_clone ]
then
    echo "Already cloned"
    cd ./wio_link_git_clone/
else
    echo "Now clone the repo..."
    git clone https://github.com/Seeed-Studio/Wio_Link.git wio_link_git_clone
    cd ./wio_link_git_clone/
    git submodule init
fi

#git checkout dev
git pull
git submodule update
supervisorctl stop esp8266

if [ ! -d  ../esp8266_iot_node ]
then
    mkdir -p ../esp8266_iot_node
    cd ../esp8266_iot_node
    cp -rf ../wio_link_git_clone/* ./
    rm -rf ./grove_drivers/grove_example
else
    cd ../esp8266_iot_node
    mv database.db database.db.bak
    mv config.py config.py.bak
    cp -rf ../wio_link_git_clone/* ./
    mv database.db.bak database.db
    mv config.py.bak config.py
    rm -rf ./grove_drivers/grove_example
fi

python ./scan_drivers.py
supervisorctl start esp8266