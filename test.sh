#!/bin/sh

python tests/test.py
python tests/test.py --daemon
sudo python tests/test.py
sudo python tests/test.py --daemon
