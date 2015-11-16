#!/bin/bash

if (( $# != 2 )); then
    echo "Illegal number of parameters"
    exit
fi

mkdir -p /home/coinsocket/.$1
touch /home/coinsocket/.$1/coinsocket.log

echo "network=bitcoin" > /home/coinsocket/.$1/coinsocket.conf
echo "peerhost=localhost" >> /home/coinsocket/.$1/coinsocket.conf
echo "sync=true" >> /home/coinsocket/.$1/coinsocket.conf
echo '' >> /home/coinsocket/.$1/coinsocket.conf

echo "dbname=$1" >> /home/coinsocket/.$1/coinsocket.conf
echo "dbuser=" >> /home/coinsocket/.$1/coinsocket.conf
echo "dbpasswd=" >> /home/coinsocket/.$1/coinsocket.conf
echo '' >> /home/coinsocket/.$1/coinsocket.conf

echo "wsport=$2" >> /home/coinsocket/.$1/coinsocket.conf
echo "allowedips=.*" >> /home/coinsocket/.$1/coinsocket.conf
echo "tlscertfile=" >> /home/coinsocket/.$1/coinsocket.conf

connectkey=$(openssl rand -base64 32 | head -c 32)
echo "connectkey=$connectkey" >> /home/coinsocket/.$1/coinsocket.conf
echo '' >> /home/coinsocket/.$1/coinsocket.conf

echo "#emailalerts=" >> /home/coinsocket/.$1/coinsocket.conf
echo "#emailalerts=" >> /home/coinsocket/.$1/coinsocket.conf
echo "#smtpuser=" >> /home/coinsocket/.$1/coinsocket.conf
echo "#smtppasswd=" >> /home/coinsocket/.$1/coinsocket.conf
echo "#smtpurl=" >> /home/coinsocket/.$1/coinsocket.conf
echo "#smtpfrom=<alerts@ciphrex.com>Ciphrex Alerts" >> /home/coinsocket/.$1/coinsocket.conf
