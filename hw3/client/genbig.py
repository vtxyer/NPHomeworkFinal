#!/usr/bin/python

a = open("big.txt","wr")
a.write("cat num") 
for k in range(1,13000):
    a.write(" | add")
