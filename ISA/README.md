# DHCP-STATS
Monitors the utilization of DHCP pools.

## Description
`dhcp-stats` monitors the utilization in both numbers and percentages of specified networks,
which represents DHCP pools. The specified pools can overlap and it is possible to specify
device's IP address instead of network IP address.

## Example
`$ dhcp-stats -i eth0 192.168.0.0/24 192.168.0.0/25 192.168.10.0/26`
```
IP Prefix        Max hosts    Allocated addresses         Utilization
192.168.0.1/24      254               25                     9.84%
10.0.0.0/16        65534             1871                    2.85%
```

## Bugs
Limited number of networks
It is possible to use max 15 networks when using standard sized console, which is 80x24.
If there must be specified more then 15 networks, the terminal have to be resized.
Be aware that it has to be done before running dhcp-stats.


Resizing and minimizing terminal
When the terminal is either resized or minimized, the output disapear.
You can still close dhcp-stats using key-shortcut CTRL+C.


Statically configured devices
The dhcp-stats can not recognize IP addresses which were configured manually without DHCP.

## Needed packages
pcapy - aviable in EPEL repository

## Files
`dhcp-stats`
`dhcp-stats.1`
`dhcp_monitor.py`
`README.md`
`Makefile`

---
Maros Kopec, xkopec44@stud.fit.vutbr.cz