#!/usr/bin/env bash

rsync -avz --rsync-path="sudo rsync" \
    kaan@192.168.1.5:/lib/ ~/rpi-sysroot/lib/
rsync -avz --rsync-path="sudo rsync" \
    kaan@192.168.1.5:/usr/ ~/rpi-sysroot/usr/
rsync -avz --rsync-path="sudo rsync" \
    kaan@192.168.1.5:/opt/ ~/rpi-sysroot/opt/

