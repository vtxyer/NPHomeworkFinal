#!/usr/bin/python
import os
port=8100
for i in range(5):
    port+=i
    os.system("/home/Master1_up/NP/hw2/One-Process/server "+str(port)+" &")
    print "start server on "+str(port)+" \n"
