#!/usr/bin/python

import select
import socket
import os
import os.path
import time
import sys
import Queue

led = 0

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 50007              # Arbitrary non-privileged port

def check_already_exported(pinnum):
    path = '/sys/class/gpio/gpio' + str(pinnum)
    isdir = os.path.isdir(path)
    return isdir

def initpin(pinnum, mode):
    '''
    pinnum: pin number, eg. 172, 175 etc.
    mode: pin mode, valid values: in or out
    '''
    if not check_already_exported(pinnum):
        with open('/sys/class/gpio/export', 'w') as f:
            f.write(str(pinnum))
        with open('/sys/class/gpio/gpio' + str(pinnum) + '/direction', 'w') as f:
            f.write(str(mode))

def setpin(pinnum, value):
    with open('/sys/class/gpio/gpio' + str(pinnum) + '/value', 'w') as f:
        f.write(str(value))

def closepin(pinnum):
    with open('/sys/class/gpio/unexport', 'w') as f:
        f.write(str(pinnum))

# Create a TCP/IP socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setblocking(0)

# Bind the socket to the port
server.bind((HOST, PORT))

# Listen for incoming connections
server.listen(5)

# Sockets from which we expect to read
inputs = [ server ]

# Sockets to which we expect to write
outputs = [ ]

# Outgoing message queues (socket:Queue)
message_queues = {}

while inputs:

    # Wait for at least one of the sockets to be ready for processing
    readable, writable, exceptional = select.select(inputs, outputs, [])

    # Handle inputs
    for s in readable:

        if s is server:
            # A "readable" server socket is ready to accept a connection
            connection, client_address = s.accept()
            connection.setblocking(0)
            inputs.append(connection)
            initpin(led, 'out')

            # Give the connection a queue for data we want to send
            message_queues[connection] = Queue.Queue()
        else:
            data = s.recv(1024)
            if data:
                # A readable client socket has data
                # print data
                # message_queues[s].put(data)
                if data == "1":
                        setpin(led, 1)
                        message_queues[s].put("Light On ")
                elif data == "0":
                        setpin(led, 0)
                        message_queues[s].put("Light Off ")
                else:
                        message_queues[s].put("Type 1 or 0 ")
                # Add output channel for response
                if s not in outputs:
                    outputs.append(s)
            else:
                # Interpret empty result as closed connection
                # Stop listening for input on the connection
                if s in outputs:
                    outputs.remove(s)
                inputs.remove(s)
                s.close()
                closepin(led)

                # Remove message queue
                del message_queues[s]

    # Handle outputs
    for s in writable:
        try:
            next_msg = message_queues[s].get_nowait()
        except Queue.Empty:
            # No messages waiting so stop checking for writability.
            outputs.remove(s)
        else:
            s.send(next_msg)
