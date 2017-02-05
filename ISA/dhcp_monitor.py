u'''
DHCP monitoring - functions

Author: Maros Kopec, xkopec44@stud.fit.vutbr.cz
Created: 20. Oktober 2016
File: dhcp_monitor
'''

from __future__ import absolute_import
from os import _exit
import sys
import socket
from struct import unpack
import time
from threading import Timer


def put_error(messsage, exit_code):
    u'''Prints message to STDERR and exits'''
    print >> sys.__stderr__, messsage
    _exit(exit_code)
# def put_error


def isnum(var, msg):
    u'''Exit program with msg if not a number.'''
    try:
        int(var)
    except ValueError:
        put_error(msg, 1)
# def isnum()


def inrange(var, min_value, max_value, msg):
    u'''Exit program with msg if number is not in specified range.'''
    if var < min_value or var > max_value:
        put_error(msg, 1)
# def inrange()


def get_bin_prefix(prefix):
    u'''Returns last octet of prefix in form of 8bit number
    @prefix needs to be int'''
    bin_prefix = ""
    last_octet = prefix - int(prefix/8)*8
    for i in range(0, last_octet):
        bin_prefix = bin_prefix + "1"
    for i in range(0, 8-len(bin_prefix)):
        bin_prefix = bin_prefix + "0"
    return int(bin_prefix, 2)
#def get_bin_prefix


def get_network(address, prefix):
    u'''Returns netwrok address from IP address and prefix
    @address needs to be list
    @prefix needs to be int'''
    ipclass = int(prefix/8)

    if ipclass == 0:
        address[0] = str(int(bin(int(address[0]))[2:], 2) & get_bin_prefix(prefix))
        address[1] = '0'
        address[2] = '0'
        address[3] = '0'
    elif ipclass == 1:
        address[1] = str(int(bin(int(address[1]))[2:], 2) & get_bin_prefix(prefix))
        address[2] = '0'
        address[3] = '0'
    elif ipclass == 2:
        address[2] = str(int(bin(int(address[2]))[2:], 2) & get_bin_prefix(prefix))
        address[3] = '0'
    elif ipclass == 3:
        address[3] = str(int(bin(int(address[3]))[2:], 2) & get_bin_prefix(prefix))
    # else: ipclass == 4 (host network, prefix /32)
        # do nothing
    return address
#def get_network


def parse_args():
    u'''Parse arguments'''
    if len(sys.argv) < 4:
        print u"usage: ./dhcp-stats -i <interface> <ip-prefix> [ <ip-prefix> [ ... ] ]"
        print u"For more info see man pages."
        _exit(0)

    addresses = []
    for arg in sys.argv[3:]:
        # option -i for interface
        if sys.argv[1] != "-i":
            print u"Not specified interface, or on wrong position."
            print u"See man pages for more info or run dhcp-stats without arguments."
            _exit(1)
        # get prefix
        prefix = arg[arg.find("/")+1:]      # get prefix
        isnum(prefix, u"Error in prefix at " + arg)
        inrange(int(prefix), 1, 32, u"Prefix in IP address:" + arg +u" not in range <1,32>")
        # get address
        arg = arg[:arg.find("/")]           # remove prefix from ip address
        address_part = arg.split(".")       # split on dot
        if len(address_part) != 4:
            put_error(u"Wrong IP address format at " + arg + u"/" + prefix, 1)
        for part in address_part:
            isnum(part, u"Error in IP address at " + arg)
            inrange(int(part), 0, 255, u"Number in IP address:" + arg +u" not in range <0,255>")
        address = {}
        address['network'] = get_network(address_part, int(prefix))   # add ip address parts to list
        address['prefix'] = prefix                                    # add prefix
        address['count'] = 0
        addresses.append(address)
    return addresses
# def parse_args


def parse_network(addresses):
    u'''Parse network. Calculate network address, hosts, etc'''
    for address in addresses:
        if int(address['prefix']) == 31:
            hosts = 2
        elif int(address['prefix']) == 32:
            hosts = 1
        else:
            hosts = 2**(32 - int(address['prefix'])) - 2
        address['hosts'] = str(hosts)
    # return addresses
# def parse_network


def glue_address(address):
    u'''Makes one string from list
    @address list containg parts of IP address'''
    glued_address = u""
    for part in address:
        glued_address = glued_address + part + "."
    glued_address = glued_address[:-1]          # remove last unwanted dot
    return glued_address
# def glue_address


def check_pool(addresses, address, add):
    u'''Check if IP is occupied in DHCP pool'''
    for network in addresses:
        ip_addr = address.split(".")
        temp = get_network(ip_addr, int(network["prefix"]))
        for i in range(0, 4):
            if network["network"][i] != temp[i]:
                break
            if i == 3:
                if add:
                    network["count"] = network["count"] + 1
                else:
                    network["count"] = network["count"] - 1
# def check_pool


def get_utilization(count, hosts):
    u'''Returns utilization of IP in DHCP pool in % as string.'''
    return "{0:.2f}".format(count / float(hosts) * 100) + "%"
# def get_utilization


def get_status(addresses):
    u'''Get the text for status window.'''
    status = u""
    for address in addresses:
        status = status + glue_address(address['network']) + u"/" + address['prefix']
        spaces = 24 - len(glue_address(address['network']) + u"/" + address['prefix'])
        status = status + (" " * spaces)

        status = status + address['hosts']
        spaces = 20 - len(address['hosts'])
        status = status + (" " * spaces)

        status = status + str(address['count'])
        spaces = 20 - len(str(address['count']))
        status = status + (" " * spaces)

        utilization = get_utilization(address['count'], address['hosts'])
        if len(utilization) == 6: # xx.xx%
            spaces = 1
        elif len(utilization) == 5: # x.xx%
            spaces = 2
        else: # no need to add more spaces when xxx.xx%
            spaces = 0
        status = status + (" " * spaces)
        status = status + utilization + "\n"
    return status
# def get_status


# Author: Silver Moon (binarytides@gmail.com)
# Available at http://skuld.bmsc.washington.edu/raster3d/html/raster3d.html
# (Accessed 29 October 2016)
def eth_addr(addr):
    u'''Convert a string of 6 characters of ethernet address into a dash separated hex string'''
    hex_addr = "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x" % (ord(addr[0]), ord(addr[1]),
                                                  ord(addr[2]), ord(addr[3]),
                                                  ord(addr[4]), ord(addr[5]))
    return hex_addr
# def eth_addr


def print_dhcp_data(dhcp_data):
    u'''Convert a string of 6 characters of ethernet address into a dash separated hex string'''
    print "Message type: %d" % (dhcp_data["op"])
    print "Hardwere type: %d" % (dhcp_data["htype"])
    print "Hardware address length: %d" % (dhcp_data["hlen"])
    print "Hops: %d" % (dhcp_data["hops"])
    print "Transaction ID: 0x%x" % (dhcp_data["xid"])
    print "Seconds elapsed: %d" % (dhcp_data["secs"])
    print "Bootp flags: 0x%x" % (dhcp_data["flags"])
    print "Client IP address: %s" % (dhcp_data["ciaddr"])
    print "Your (client) IP address: %s" % (dhcp_data["yiaddr"])
    print "Next server IP address: %s" % (dhcp_data["siaddr"])
    print "Relay agent IP address: %s" % (dhcp_data["giaddr"])
    print "Client MAC address: %s" % (eth_addr(dhcp_data["chaddr"]))
    print "Server host name: %s" % (dhcp_data["sname"])
    print "Boot file name: %s" % (dhcp_data["file"])
# def print_dhcp_data


def get_byte(array, count=1, format_string='!B'):
    u'''Gets byte from array of bytes and return content. Also removes that byte from array.'''
    array[:] = array[1:]                    # remove first byte - length
    temp = "".join(array[:count])           # take another byte
    array[:] = array[count:]                # remove first byte
    temp = unpack(format_string, temp)      # convert byte
    temp = temp[0]                          # extract number from tuple
    return temp
# def get_byte


def plan_check(leased_ip, key):
    u'''Set Timer to plan checking on leased IP address'''
    timer = Timer(leased_ip[key]["lease"], check_lease, [leased_ip, key])
    timer.start()
    return timer
# def plan_check


def check_lease(leased_ip, key):
    u'''Check lease time of IP address and removes it when expired'''
    if int(leased_ip[key]["timestamp"]) + int(leased_ip[key]["lease"]) == int(time.time()):
        del leased_ip[key]
# def check_lease


def parse_dhcp_options(dhcp_opt_raw):
    u'''Parse options from DHCP frame'''
     # to store dhcp options informations
    options = {}

    acceptable = [50, 51, 53, 54, 58, 59]           # acceptable options codes
    while bool(dhcp_opt_raw): # True
        temp = dhcp_opt_raw[:1]                     # take one byte
        dhcp_opt_raw = dhcp_opt_raw[1:]             # remove first byte
        temp = unpack('!B', temp[0])                # convert byte to int
        temp = temp[0]                              # extract number from tuple

        if temp == 255:     # end of options
            break
        elif temp == 0:     # padding
            continue

        if not temp in acceptable:
            temp = dhcp_opt_raw[:1]                 # take another byte where is length
            dhcp_opt_raw = dhcp_opt_raw[1:]         # remove first byte
            temp = unpack('!B', temp[0])            # convert byte to int
            temp = temp[0]                          # extract number from tuple
            dhcp_opt_raw = dhcp_opt_raw[temp:]      # remove not needed bytes
            continue

        if temp == 50:
            temp = get_byte(dhcp_opt_raw, 4, '!4s')
            temp = socket.inet_ntoa(temp)
            options["req_ip"] = temp
            continue
        if temp == 51:
            temp = get_byte(dhcp_opt_raw, 4, '!I')
            options["lease_time"] = temp
            continue
        if temp == 53:
            temp = get_byte(dhcp_opt_raw)
            options["type"] = temp
            # print "Type:", temp
            continue
        if temp == 54:
            temp = get_byte(dhcp_opt_raw, 4, '!4s')
            temp = socket.inet_ntoa(temp)
            options["server_id"] = temp
            # print "Server Identifier: ", temp
            continue
        # if temp == 58:
        #     temp = get_byte(dhcp_opt_raw, 4, '!I')
        #     options["renewal"] = temp
        #     # print "Renewal (T1) Time Value: ", temp
        #     continue
        # if temp == 59:
        #     temp = get_byte(dhcp_opt_raw, 4, '!I')
        #     # print "Rebinding (T2) Time Value: ", temp
        #     options["rebind"] = temp
        #     continue

    return options
# def parse_dhcp_options


def handle_req(options, stored_leases, dhcp_data, addresses):
    u'''Handle request'''
    if options["type"] == 5: # ACK
        # request to keep IP address
        if dhcp_data["yiaddr"] in stored_leases.keys():
            stored_leases[dhcp_data["yiaddr"]]["timer"].cancel()
            stored_leases[dhcp_data["yiaddr"]]["timer"] = \
                                                      plan_check(stored_leases, dhcp_data["yiaddr"])
            stored_leases[dhcp_data["yiaddr"]]["timestamp"] = time.time()
            stored_leases[dhcp_data["yiaddr"]]["lease"] = options["lease_time"]
        # standard IP lease
        else:
            stored_leases[dhcp_data["yiaddr"]] = {}
            stored_leases[dhcp_data["yiaddr"]]["timestamp"] = time.time()
            stored_leases[dhcp_data["yiaddr"]]["lease"] = options["lease_time"]
            timer = plan_check(stored_leases, dhcp_data["yiaddr"])
            stored_leases[dhcp_data["yiaddr"]]["timer"] = timer
            # count addresses
            check_pool(addresses, dhcp_data["yiaddr"], True)
    elif options["type"] == 7: # RELEASE
        if dhcp_data["ciaddr"] in stored_leases.keys():
            del stored_leases[dhcp_data["ciaddr"]]
            check_pool(addresses, dhcp_data["ciaddr"], False)
# def handle_req


def parse_dhcp(frame, stored_leases, addresses):
    u'''Parse DHCP frame'''
    # DHCP frame without options - 236 bytes
    dhcph = frame[:236]
    # DHCP options are the rest
    dhcp_opt_raw = frame[236:]
    # Unpack options
    dhcp_raw = unpack('!BBBBIHH4s4s4s4s16s64s128s', dhcph)

    # store dhcp data into dictionary
    dhcp_data = {}
    dhcp_data["op"] = dhcp_raw[0]
    dhcp_data["htype"] = dhcp_raw[1]
    dhcp_data["hlen"] = dhcp_raw[2]
    dhcp_data["hops"] = dhcp_raw[3]
    dhcp_data["xid"] = dhcp_raw[4]
    dhcp_data["secs"] = dhcp_raw[5]
    dhcp_data["flags"] = dhcp_raw[6]
    dhcp_data["ciaddr"] = socket.inet_ntoa(dhcp_raw[7])
    dhcp_data["yiaddr"] = socket.inet_ntoa(dhcp_raw[8])
    dhcp_data["siaddr"] = socket.inet_ntoa(dhcp_raw[9])
    dhcp_data["giaddr"] = socket.inet_ntoa(dhcp_raw[10])
    dhcp_data["chaddr"] = dhcp_raw[11]
    dhcp_data["sname"] = dhcp_raw[12]
    dhcp_data["file"] = dhcp_raw[13]

    # print_dhcp_data(dhcp_data)

    # remove magic cookie from options
    dhcp_opt_raw = dhcp_opt_raw[4:]
    # transform to list, so it can be modified in function
    dhcp_opt_raw = list(dhcp_opt_raw)
    # parse DHCP options and handle request
    handle_req(parse_dhcp_options(dhcp_opt_raw), stored_leases, dhcp_data, addresses)
# parse_dhcp


# Author: Silver Moon (binarytides@gmail.com)
# Available at http://skuld.bmsc.washington.edu/raster3d/html/raster3d.html
# (Accessed 29 October 2016)
#
# editted by Maros Kopec
def parse_packet(packet, stored_leases, addresses):
    u'''Parse packet cought with pcap'''
    # parse ethernet header
    eth_length = 14

    # take first 20 characters for the ip header
    ip_header = packet[eth_length:20+eth_length]
    # now unpack them
    iph = unpack('!BBHHHBBH4s4s', ip_header)

    # get IP header length
    version_ihl = iph[0]
    ihl = version_ihl & 0xF
    iph_length = ihl * 4

    # UDP header length
    udph_length = 8

    # get all headers length
    h_size = eth_length + iph_length + udph_length

    # get only needed data from the packet
    dhcp = packet[h_size:]

    # parse dhcp frame
    parse_dhcp(dhcp, stored_leases, addresses)
# def parse_packet



