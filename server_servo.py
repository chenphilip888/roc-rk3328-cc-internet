#!/usr/bin/python

import select
import socket
import os
import os.path
import time
import sys
import Queue

pwm_period = 0.0

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 50007              # Arbitrary non-privileged port

def pwm_open():
    os.system('sudo echo 0 > /sys/class/pwm/pwmchip0/export')
    os.system('sudo echo "normal" > /sys/class/pwm/pwmchip0/pwm0/polarity')

def pwm_enable():
    os.system('sudo echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable')

def pwm_stop():
    os.system('sudo echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable')

def pwm_close():
    os.system('sudo echo 0 > /sys/class/pwm/pwmchip0/unexport')

def pwm_freq(freq):
    global pwm_period
    pwm_period = 1000000000.0 / freq
    os.system('sudo echo ' + str(int(pwm_period)) + ' > /sys/class/pwm/pwmchip0/pwm0/period')

def pwm_duty(duty):
    dutycycle = duty * int(pwm_period)
    os.system('sudo echo ' + str(int(dutycycle)) + ' > /sys/class/pwm/pwmchip0/pwm0/duty_cycle')

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
            pwm_open()
            pwm_freq(50)
            pwm_duty(0.05)              # min 0.05, max 0.15 180 degrees
            pwm_enable()

            # Give the connection a queue for data we want to send
            message_queues[connection] = Queue.Queue()
        else:
            data = s.recv(1024)
            if data:
                # A readable client socket has data
                # print data
                # message_queues[s].put(data)
                if data == "stop":
                        pwm_stop()
                        message_queues[s].put("Stop ")
                elif data == "middle":
                        pwm_duty(0.1)
                        message_queues[s].put("Middle ")
                elif data == "right":
                        pwm_duty(0.05)
                        message_queues[s].put("Right ")
                elif data == "left":
                        pwm_duty(0.15)
                        message_queues[s].put("Left ")
                else:
                        message_queues[s].put("Error ")
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
                pwm_close()

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
