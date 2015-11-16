#!/bin/bash

if (( $# != 3 )); then
    echo "Illegal number of parameters"
    exit
fi

mysql -u $1 -p$2 --execute="CREATE DATABASE $3"
coindb create $3
