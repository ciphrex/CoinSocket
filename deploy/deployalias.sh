#!/bin/bash

if (( $# != 1 )); then
    echo "Illegal number of parameters"
    exit
fi

echo '' >> ~/.bash_profile
echo "alias start$1='sudo start coinsocketd-$1'" >> ~/.bash_profile
echo "alias stop$1='sudo stop coinsocketd-$1'" >> ~/.bash_profile
echo "alias log$1='sudo -u coinsocket tail -f -n 100 /home/coinsocket/.$1/coinsocket.log'" >> ~/.bash_profile
echo "alias vi$1='sudo -u coinsocket vi /home/coinsocket/.$1/coinsocket.conf'" >> ~/.bash_profile
echo "alias vilog$1='sudo -u coinsocket vi /home/coinsocket/.$1/coinsocket.log'" >> ~/.bash_profile
