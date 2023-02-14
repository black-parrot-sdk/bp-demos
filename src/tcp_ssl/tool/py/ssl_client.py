#!/usr/bin/env python3

import socket
import time
import ssl

# Specify the server address and port here:
HOST = '192.168.0.141'
PORT = 12345


context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
context.check_hostname = False
context.verify_mode = ssl.CERT_REQUIRED
# load CA certificate
context.load_verify_locations('../ssl_cert_gen/ssl_cert_build/ca_bundle.pem')

with socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0) as sock:
    sock.connect((HOST, PORT))
    with context.wrap_socket(sock, server_side=False) as ssock:
        print('Client: Now sending data through SSL')
        while(True):
            ssock.send(b'hello from client 1')
            time.sleep(1)
