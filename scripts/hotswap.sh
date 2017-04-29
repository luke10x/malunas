#!/bin/bash

inotifywait -r -m ./src/ | while read _ event file;
do 
    if echo $event | grep -q 'CLOSE_WRITE' ;
    then
        o="$event ./obj/${file%.*}.o";
        rm -f $o; make ./malunas ;
    fi;
done

