#!/usr/bin/python

import socket
import time

HOST = '192.168.86.202'   # The remote host
PORT = 50007              # The same port as used by the server

sock=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

for i in range( 5 ):
    sock.sendall("middle")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("right")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("left")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("middle")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("left")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("right")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("middle")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("left")
    print(sock.recv(1024))
    time.sleep( 0.8 )

sock.sendall("stop")
print(sock.recv(1024))
sock.close()
