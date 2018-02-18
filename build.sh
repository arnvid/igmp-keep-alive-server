#!/bin/bash
echo "Removing old binaries."
rm igmp-keep-alive-server.o igmp-keep-alive-server
gcc -Wall -lz -o igmp-keep-alive-server igmp-keep-alive-server.c
