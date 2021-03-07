#!/usr/bin/python

import socket
import time

HOST = '192.168.86.202'   # The remote host
PORT = 50007              # The same port as used by the server

sock=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

for i in range( 5 ):
    sock.sendall("1")
    print(sock.recv(1024))
    time.sleep( 1 )
    sock.sendall("0")
    print(sock.recv(1024))
    time.sleep( 1 )

sock.close()
