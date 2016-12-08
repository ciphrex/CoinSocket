#!/bin/bash

if (( $# != 2 )); then
    echo "Illegal number of parameters"
    exit
fi

echo "# Job for starting coinsocketd for $2 automatically" > /etc/init/coinsocketd-$2.conf
echo '' >> /etc/init/coinsocketd-$2.conf

echo 'author "Ciphrex"' >> /etc/init/coinsocketd-$2.conf
echo 'description "coinsocketd-'$2' job"' >> /etc/init/coinsocketd-$2.conf
echo '' >> /etc/init/coinsocketd-$2.conf

echo 'stop on runlevel [!2345]' >> /etc/init/coinsocketd-$2.conf
echo '#respawn' >> /etc/init/coinsocketd-$2.conf
echo '' >> /etc/init/coinsocketd-$2.conf

echo 'script' >> /etc/init/coinsocketd-$2.conf
echo "  sudo -u $1 coinsocketd --datadir $2" >> /etc/init/coinsocketd-$2.conf
echo "end script" >> /etc/init/coinsocketd-$2.conf
