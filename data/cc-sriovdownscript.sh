mkdir -p /var/run/netns
ln -s /proc/$1/ns/net /var/run/netns/$1
ip link set $2 netns $1
ip netns exec $1 ip link set $2 down
ip netns exec $1 ip link set dev $2 name eth0
