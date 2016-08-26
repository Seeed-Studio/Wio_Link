#Wio Link



![](https://ksr-ugc.imgix.net/assets/004/976/751/7d273f1694c9c37b446ae820ea49c92a_original.jpg?v=1448431948&w=680&fit=max&auto=format&q=92&s=d0c984a9f958d807035d12c045619660)

## Introduction
Wio Link is designed to simplify your IoT development. It is an ESP8266 based Wi-Fi development board for you to create IoT applications with open-source, plug and play electronics, mobile APPs and RESTful APIs.The key feature of this project is that users never need to program one line of code before he get all the Grove modules into Web of Things. 

Features:

* Totally open sourced for both software and hardware
* No need microcontroller programming
* Config with GUI
* Web of Things for hundreds of Grove modules (covering most IOT application domains)
* WiFi enabled with ESP8266 which is not expensive to have
* Security enhenced (AES for session between node & server, https for API calls)
* RESTful APIs for application 
* Lightweighted and open sourced broker server based on Tornado
* Deploy private server with Docker


[Video: 3 Steps. 5 Minutes. Build IoT Device.](https://www.youtube.com/watch?v=P_SO_a6X-y0#action=share)


## About the Server

Currently we have two sandbox servers - one for international users while another for users inside China. Due to the (G)(F)(W), users in China mainland can not access the international server smoothly. There're options in the App to select the server. 

The sandbox servers are free to use, but we can't guarantee the quality of the service persists good if the amount of connections becomes large. As the project is open sourced, we recommend users deploy their own server for private application. The server can be a local server or a VPS, even can be a lean computer like Raspberry Pi. See next section "Documentation" for the deployment guide. []()



## Documentation

* [API Documentation](https://github.com/Seeed-Studio/Wio_Link/wiki/API%20Documentation)
* [Server Deployment Guide](https://github.com/Seeed-Studio/Wio_Link/wiki/Server%20Deployment%20Guide)
* [Advanced User Guide](https://github.com/Seeed-Studio/Wio_Link/wiki/Advanced%20User%20Guide)


## How to write module driver for Wio Link? 

A compatible module driver can be scanned by the server and be integrated into the online compilation service. Users can develop driver for their own modules. The low level of this project is based on the esp8266 Arduino project. Arduino SDK functions can be called in the drivers.

The only difference between writing a Wio Link compatible driver and writing an Arduino compatible driver is that, you need to follow some rules to write the header file. It is for the scanning script to process it correctly. Please move to this [guide](https://github.com/Seeed-Studio/Wio_Link/wiki/How-to-write-module-driver-for-Wio-Link%3F) to see the rules.

Users can pull request to [this github repo](https://github.com/Seeed-Studio/Grove_Drivers_for_Wio) to integrate the developed driver into the online compiling system, and at the same time into the mobile App. SEEED's staff will test the merged driver. If it's written correctly, we will accept the pull request and update the web services to let users use this module. For advanced users, we recommend to deploy a self server, to add 3rd party drivers immediately.

## License

This project is developed by Jack Shao(<xuguang.shao@seeed.cc>) for seeed studio. 

This project refers to:

ESP8266_Arduino, mbed ssl, Espressif SDK, Tornado, PyYaml, PyJWT, pycrypto

Thank them very much.


The code written in this project is licensed under the [GNU GPL v3 License](http://www.gnu.org/licenses/gpl-3.0.en.html). 

[![Analytics](https://ga-beacon.appspot.com/UA-46589105-3/Wio_Link)](https://github.com/igrigorik/ga-beacon)