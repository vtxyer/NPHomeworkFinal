
#!/bin/bash

rm -f /home/netlog/vtcount
rm -f /home/netlog/net
rm -f /home/netlog/net2
rm -f /home/netlog/dump
rm -f /home/netlog/transmit_status
rm -f /home/netlog/tx_ring_log 
rm -f /home/netlog/receive
rm -f /home/netlog/seq
rm -f /home/netlog/errormsg
rm -f /home/netlog/err
rm -f /home/netlog/x
rm -f /home/netlog/len
rm -f /home/netlog/abc
rm -f /home/netlog/raw_end
rm -f /home/netlog/seq_http
rm -f /home/netlog/window_http
rm -f /home/netlog/window_ftp
echo 0 > /home/netlog/seq_http
echo 0 > /home/netlog/seq_ftp
echo 0 > /home/netlog/rate_http
echo 0 > /home/netlog/rate_ftp
