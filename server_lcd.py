#!/usr/bin/python

import select
import socket
import time
import sys
import Queue
import smbus

bus = smbus.SMBus(1)

# this device has two I2C addresses
DISPLAY_RGB_ADDR = 0x62
DISPLAY_TEXT_ADDR = 0x3e

# I2C LCD
# set backlight to (R,G,B) (values from 0..255 for each)
def setRGB(r,g,b):
    bus.write_byte_data(DISPLAY_RGB_ADDR,0,0)
    bus.write_byte_data(DISPLAY_RGB_ADDR,1,0)
    bus.write_byte_data(DISPLAY_RGB_ADDR,0x08,0xaa)
    bus.write_byte_data(DISPLAY_RGB_ADDR,4,r)
    bus.write_byte_data(DISPLAY_RGB_ADDR,3,g)
    bus.write_byte_data(DISPLAY_RGB_ADDR,2,b)

# send command to display (no need for external use)    
def textCommand(cmd):
    bus.write_byte_data(DISPLAY_TEXT_ADDR,0x80,cmd)

# set display text \n for second line(or auto wrap)     
def setText(text):
    textCommand(0x01) # clear display
    time.sleep(.05)
    textCommand(0x08 | 0x04) # display on, no cursor
    textCommand(0x28) # 2 lines
    time.sleep(.05)
    count = 0
    row = 0
    for c in text:
        if c == '\n' or count >= 16:
            count = 0
            row += 1
            if row >= 2:
                break
            textCommand(0xc0)
            if c == '\n':
                continue
        count += 1
        bus.write_byte_data(DISPLAY_TEXT_ADDR,0x40,ord(c))

textCommand(0x01) # clear display
time.sleep(.05)
textCommand(0x08 | 0x04) # display on, no cursor
textCommand(0x28) # 2 lines
time.sleep(.05)

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 50007              # Arbitrary non-privileged port

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

            # Give the connection a queue for data we want to send
            message_queues[connection] = Queue.Queue()
        else:
            data = s.recv(1024)
            if data:
                # A readable client socket has data
                # print data
                # message_queues[s].put(data)
		if data == "red":
			setRGB( 255, 0, 0 )
                        setText("\n\n")
			message_queues[s].put("Red ")
		elif data == "green":
			setRGB( 0, 255, 0 )
                        setText("\n\n")
			message_queues[s].put("Green ")
		elif data == "blue":
			setRGB( 0, 0, 255 )
                        setText("\n\n")
			message_queues[s].put("Blue ")
		elif data == "yellow":
			setRGB( 255, 255, 0 )
                        setText("\n\n")
			message_queues[s].put("Yellow ")
		elif data == "cyan":
			setRGB( 0, 255, 255 )
                        setText("\n\n")
			message_queues[s].put("Cyan ")
		elif data == "purple":
			setRGB( 255, 0, 255 )
                        setText("\n\n")
			message_queues[s].put("Purple ")
		elif data == "black":
			setRGB( 0, 0, 0 )
                        setText("\n\n")
			message_queues[s].put("Black ")
		elif data == "white":
			setRGB( 255, 255, 255 )
                        setText("\n\n")
			message_queues[s].put("White ")
		else:
                        setRGB( 128, 255, 0 )
                        setText( data )
			message_queues[s].put("OK ")
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
