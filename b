#!/bin/bash

cd third-party
cd websockify

cd webserver
make >/dev/null
sudo make install >/dev/null
cd ..
cd websocket
make >/dev/null
sudo make install >/dev/null
cd ..
cd wsproxy
make >/dev/null
sudo make install >/dev/null
cd ..

cd ..
cd ..

make >/dev/null

