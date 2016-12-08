#!/bin/bash

if (( $# != 2 )); then
    echo "Illegal number of parameters"
    exit
fi

mkdir -p ~/.$1
touch ~/.$1/coinsocket.conf

echo "network=bitcoin" > ~/.$1/coinsocket.conf
echo "peerhost=localhost" >> ~/.$1/coinsocket.conf
echo "sync=true" >> ~/.$1/coinsocket.conf
echo '' >> ~/.$1/coinsocket.conf

echo "dbname=$1" >> ~/.$1/coinsocket.conf
echo "dbuser=" >> ~/.$1/coinsocket.conf
echo "dbpasswd=" >> ~/.$1/coinsocket.conf
echo '' >> ~/.$1/coinsocket.conf

echo "wsport=$2" >> ~/.$1/coinsocket.conf
echo "allowedips=.*" >> ~/.$1/coinsocket.conf
echo "tlscertfile=" >> ~/.$1/coinsocket.conf

connectkey=$(openssl rand -base64 32 | head -c 32)
echo "connectkey=$connectkey" >> ~/.$1/coinsocket.conf
echo '' >> ~/.$1/coinsocket.conf

echo "#emailalerts=" >> ~/.$1/coinsocket.conf
echo "#smtpuser=" >> ~/.$1/coinsocket.conf
echo "#smtppasswd=" >> ~/.$1/coinsocket.conf
echo "#smtpurl=" >> ~/.$1/coinsocket.conf
echo "#smtpfrom=" >> ~/.$1/coinsocket.conf
