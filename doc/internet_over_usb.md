sshpass -p temppwd ssh debian@192.168.7.2

en bbb:
sudo ifconfig usb0 192.168.7.2
sudo route add default gw 192.168.7.1

en PC:
ifconfig enx0cb2b7c8587e 192.168.7.1
iptables --table nat --append POSTROUTING --out-interface wlp2s0 -j MASQUERADE
iptables --append FORWARD --in-interface enx0cb2b7c8587e -j ACCEPT
echo 1 > /proc/sys/net/ipv4/ip_forward

en bbb:
sudo nano /etc/resolv.conf
nameserver 8.8.8.8
sudo systemctl restart systemd-resolved.service
ping www.example.com
