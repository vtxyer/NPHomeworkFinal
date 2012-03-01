#!/bin/sh
echo "ls" > huge
line=" | cat"
for (( i=1; i<=2000; i=i+1 )) 
do
    line=$line" | cat"
done
echo $line >> huge
