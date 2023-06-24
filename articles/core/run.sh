#! /bin/bash -ux
# Example output of this script is here.
# https://gist.github.com/akawashiro/128ab5c9f32de95e1bfc1b2f774b8ec7#file-00-12-01-102-847748_-run-sh-log
# 
# You can find deadbeefdeadbeef tried to overwrite the program segment and caused Segmentation fault.
# https://gist.github.com/akawashiro/128ab5c9f32de95e1bfc1b2f774b8ec7#file-00-12-01-102-847748_-run-sh-log-L9012

gcc -o ./make_core ./make_core.c

sudo bash -c 'echo core.%t > /proc/sys/kernel/core_pattern'
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

ulimit -c unlimited
./make_core

echo 2 | sudo tee /proc/sys/kernel/randomize_va_space

CORE_FILENAME=$(find . | grep "^./core.*" | sort -R | head -n 1)
readelf -l ${CORE_FILENAME} | grep 555555557000 -A 5

# Dump around Segmentation falut
xxd -s 0x3000 -l 0x100000 ${CORE_FILENAME}

readelf --note ${CORE_FILENAME}

g++ -o ./parse_core ./parse_core.cc
./parse_core ${CORE_FILENAME}
