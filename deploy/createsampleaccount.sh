#!/bin/bash

if (( $# != 1 )); then
    echo "Illegal number of parameters"
    exit
fi

coindb newkeychain $1 keychain1
coindb newkeychain $1 keychain2
coindb newkeychain $1 keychain3
coindb newaccount $1 account123 2 keychain1 keychain2 keychain3
