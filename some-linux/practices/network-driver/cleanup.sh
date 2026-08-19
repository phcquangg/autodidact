#!/bin/bash

sudo ip netns del ns_client
sudo ip netns del ns_server
sudo ip link del br0
sudo ip link del br1
sudo ip tuntap del mode tap dev tap0
sudo ip tuntap del mode tap dev tap1

# TODO: update needed for verbose and logs
