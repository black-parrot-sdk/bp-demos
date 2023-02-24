#!/usr/bin/env python3

import socket
import time
import ssl

# Specify the server address and port here:
HOST = ''
PORT = 12345

SSL_DATA_DIR = '../../ssl_data/'
CA_CERTIFICATE = SSL_DATA_DIR + 'ca_bundle.pem'

context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
context.check_hostname = False
context.verify_mode = ssl.CERT_REQUIRED
# load CA certificate
context.load_verify_locations(CA_CERTIFICATE)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0) as sock:
    sock.connect((HOST, PORT))
    with context.wrap_socket(sock, server_side=False) as ssock:
        print('Client: Now sending data through SSL')
        while(True):
            ssock.send(b'hello from client')
            time.sleep(1)
