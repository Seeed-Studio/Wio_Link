FROM python:2.7-buster

#install required packages
RUN apt-get update && \
    apt-get install -qqy wget openssl python2.7-dev python-pip vim git

#get the toolchain
WORKDIR /opt
RUN dpkg --print-architecture
RUN /bin/bash -c "if dpkg --print-architecture | grep -q -e x86 -e amd ; then \
        wget -O xtensa.tar.gz https://github.com/esp8266/Arduino/releases/download/2.3.0/linux64-xtensa-lx106-elf-gb404fb9.tgz; else \
        wget -O xtensa.tar.gz https://github.com/esp8266/Arduino/releases/download/2.3.0/linuxarm-xtensa-lx106-elf-g46f160f-2.tar.gz; \
        ln -sf /lib/arm-linux-gnueabi/ld-2.24.so /lib/ld-linux-armhf.so.3; fi"
RUN tar -zxvf xtensa.tar.gz
ENV PATH="${PATH}:opt/xtensa-lx106-elf/bin"

COPY requirements.txt .
RUN pip install -r requirements.txt

#add the files into image
RUN mkdir -p /root/wio
WORKDIR /root/wio
COPY . /root/wio
#this is for marina.io builder
RUN git submodule init || true
RUN git submodule update || true
RUN python ./scan_drivers.py
RUN mv ./update.sh ../update.sh
RUN chmod a+x ../update.sh

#expose ports
EXPOSE 8000 8001 8080 8081

CMD python server.py

