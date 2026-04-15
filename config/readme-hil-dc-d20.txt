To execute the HIL setup `config-hil-dc-d20`, you need to EVAcharge SE boards.
On the first one, the QCA chipset should be configured as EVSE (called CCM),
on the second one, the role must be EV (called CCP).

It is assumed that on both boards ser2net is configured and listens on
TCP port 5000, forwarding an incoming telnet connection to the KL02 MCU.

Also it is assumed, that your kernel has support for VXLANs and that on both
EVAcharge SEs a bridge between eth1 and such an VXLAN is setup.
This is used to tunnel the eth1 (aka PLC) connection to the host running
this HIL setup.

The following commands are used to configure the VXLANs:

# CCP - configure
ip link add br0 type bridge
ip link set dev eth1 master br0
ip link add vxlan0 type vxlan id 101 remote 192.168.8.1 dstport 4789 dev eth0
ip link set dev vxlan0 master br0
ip link set vxlan0 up
ip link set br0 up

# CCP - deconfigure
ip link set vxlan0 down
ip link delete vxlan0
ip link set br0 down
ip link delete br0

# CCM - configure
ip link add br1 type bridge
ip link set dev eth1 master br1
ip link add vxlan0 type vxlan id 100 remote 192.168.8.1 dstport 4789 dev br0
ip link set dev vxlan0 master br1
ip link set vxlan0 up
ip link set br1 up

# CCM - deconfigure
ip link set vxlan0 down
ip link delete vxlan0
ip link set br1 down
ip link delete br1

# Host - configure
ip link add vxlan0 type vxlan id 100 remote 192.168.8.115 dstport 4789 dev enp3s0
ip link set vxlan0 up
ip link add vxlan1 type vxlan id 101 remote 192.168.8.113 dstport 4789 dev enp3s0
ip link set vxlan1 up

# Host - deconfigure
ip link set vxlan0 down
ip link delete vxlan0
ip link set vxlan1 down
ip link delete vxlan1
