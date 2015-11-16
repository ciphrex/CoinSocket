#!/bin/bash

if (( $# != 1 )); then
    echo "Illegal number of parameters"
    exit
fi

echo "# Job for starting coinsocketd for $1 automatically" > /etc/init/coinsocketd-$1.conf
echo '' >> /etc/init/coinsocketd-$1.conf

echo 'author "Ciphrex"' >> /etc/init/coinsocketd-$1.conf
echo 'description "coinsocketd-'$1' job"' >> /etc/init/coinsocketd-$1.conf
echo '' >> /etc/init/coinsocketd-$1.conf

echo 'stop on runlevel [!2345]' >> /etc/init/coinsocketd-$1.conf
echo '#respawn' >> /etc/init/coinsocketd-$1.conf
echo '' >> /etc/init/coinsocketd-$1.conf

echo 'script' >> /etc/init/coinsocketd-$1.conf
echo "  sudo -u coinsocket coinsocketd --datadir $1" >> /etc/init/coinsocketd-$1.conf
echo "end script" >> /etc/init/coinsocketd-$1.conf
