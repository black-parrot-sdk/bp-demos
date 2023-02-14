#!/usr/bin/env python3

import socket
import time

# Specify the server address and port here:
HOST = '192.168.0.141'
PORT = 12345



with socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0) as sock:
    sock.connect((HOST, PORT))
    print('Client: Now sending data through TCP')
    while(True):
        data = sock.send(b'hello from client')
        time.sleep(1)
