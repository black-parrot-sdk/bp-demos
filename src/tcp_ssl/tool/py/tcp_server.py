#!/usr/bin/env python3

import socket

# next create a socket object
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)

# Specify the server address and port here:
host_addr = ''
port      = 12345

sock.bind((host_addr, port))
sock.listen(1)

while True:

    c, addr = sock.accept()
    print ('Got connection from', addr )

    while True:
        data = c.recv(1024)
        if data != b'':
            print(repr(data))
    c.close()
