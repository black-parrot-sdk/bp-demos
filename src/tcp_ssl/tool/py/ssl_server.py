#!/usr/bin/env python3

import socket
import ssl
import time

# Specify the server address and port here:
HOST = ''
PORT = 12345

SSL_DATA_DIR = '../../ssl_data/'

SERVER_CERTIFICATE = SSL_DATA_DIR + 'server_cert.pem'
SERVER_PRIVATE_KEY = SSL_DATA_DIR + 'server_private_key.pem'

context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
context.check_hostname = False
# load server certificate and its private key:
context.load_cert_chain(SERVER_CERTIFICATE, SERVER_PRIVATE_KEY)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0) as sock:
    sock.bind((HOST, PORT))
    print ('Server: now listening...')
    sock.listen(1)
    with context.wrap_socket(sock, server_side=True) as ssock:
        conn, addr = ssock.accept()
        print ('Got SSL connection from', addr)
        conn.send(b'hello from server')
        conn.close()
        print ('Server connection closed')
        while(True):
            time.sleep(1)
