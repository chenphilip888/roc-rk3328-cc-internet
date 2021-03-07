#!/usr/bin/python

import socket
import time

HOST = '192.168.86.202'   # The remote host
PORT = 50007              # The same port as used by the server

sock=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

for i in range( 5 ):
    sock.sendall("red")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("green")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("blue")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("yellow")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("cyan")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("purple")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("white")
    print(sock.recv(1024))
    time.sleep( 0.8 )
    sock.sendall("black")
    print(sock.recv(1024))
    time.sleep( 0.8 )

sock.sendall( "Hello world !\nIt works !\n" )
print(sock.recv(1024))
sock.close()
