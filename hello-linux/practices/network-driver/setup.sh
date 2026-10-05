#!/bin/bash
set -e

# Colored indicators
INFO="[ \033[1;34m..\033[0m ]"
OK="[ \033[1;32mOK\033[0m ]"

log_step() {
    local msg="$1"
    shift
    echo -ne "${INFO} ${msg}..."
    if "$@" >/dev/null 2>&1; then
        echo -e "\r${OK} ${msg}"
    else
        echo -e "\r[ \033[1;31mFAIL\033[0m ] ${msg}"
        exit 1
    fi
}

echo "=== Starting Network Setup ==="

log_step "Creating network namespaces" \
    sudo ip netns add ns_client && \
    sudo ip netns add ns_server

log_step "Creating TAP interfaces" \
    sudo ip tuntap add mode tap dev tap0 && \
    sudo ip tuntap add mode tap dev tap1

log_step "Creating veth pairs" \
    sudo ip link add veth_cli type veth peer name veth_cli_host && \
    sudo ip link add veth_srv type veth peer name veth_srv_host

log_step "Moving veth interfaces into namespaces" \
    sudo ip link set veth_cli netns ns_client && \
    sudo ip link set veth_srv netns ns_server

log_step "Creating bridges and attaching interfaces" \
    sudo ip link add name br0 type bridge && \
    sudo ip link set veth_cli_host master br0 && \
    sudo ip link set tap0 master br0 && \
    sudo ip link add name br1 type bridge && \
    sudo ip link set veth_srv_host master br1 && \
    sudo ip link set tap1 master br1

log_step "Bringing up host interfaces and bridges" \
    sudo ip link set tap0 up && \
    sudo ip link set tap1 up && \
    sudo ip link set veth_cli_host up && \
    sudo ip link set veth_srv_host up && \
    sudo ip link set br0 up && \
    sudo ip link set br1 up

log_step "Configuring ns_client network" \
    sudo ip netns exec ns_client ip link set lo up && \
    sudo ip netns exec ns_client ip link set veth_cli up && \
    sudo ip netns exec ns_client ip addr add 192.168.1.2/24 dev veth_cli && \
    sudo ip netns exec ns_client ip route add default via 192.168.1.1

log_step "Configuring ns_server network" \
    sudo ip netns exec ns_server ip link set lo up && \
    sudo ip netns exec ns_server ip link set veth_srv up && \
    sudo ip netns exec ns_server ip addr add 192.168.2.2/24 dev veth_srv && \
    sudo ip netns exec ns_server ip route add default via 192.168.2.1

echo "=== Setup Completed Successfully ==="
