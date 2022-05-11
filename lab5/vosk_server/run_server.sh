#!/bin/sh

DIR=`dirname $0`
cd $DIR

export LD_LIBRARY_PATH="`pwd`"
"./ld-linux-armhf.so.3" "./server"

