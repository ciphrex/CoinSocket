#!/bin/bash

if (( $# != 2 )); then
    echo "Illegal number of parameters"
    exit
fi

echo '' >> ~/.bash_profile
echo "alias start$2='sudo start coinsocketd-$2'" >> ~/.bash_profile
echo "alias stop$2='sudo stop coinsocketd-$2'" >> ~/.bash_profile
echo "alias log$2='sudo -u coinsocket tail -f -n 100 /home/$1/.$2/coinsocket.log'" >> ~/.bash_profile
echo "alias vi$2='sudo -u coinsocket vi /home/$1/.$2/coinsocket.conf'" >> ~/.bash_profile
echo "alias vilog$2='sudo -u coinsocket vi /home/$1/.$2/coinsocket.log'" >> ~/.bash_profile
