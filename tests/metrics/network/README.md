# Networking tests for the Intel® Clear Containers runtime

Currently, the Clear Containers have a series of network performance tests. 
Use the tests as a basic reference to measure the following network essentials:

- Bandwidth
- Latency
- Jitter
- Throughput
- CPU % and memory consumption

## Performance tools

- Iperf and Iperf3 are simple tools to measure the bandwidth and the quality of a network 
link. Iperf is multi-threaded and Iperf3 is single threaded.

- Nuttcp is a network performance tool used to determine the raw UDP network layer 
throughput.

- Smem provides users with diverse reports on memory usage.

## Networking tests

### `network-metrics`

`network-metrics` measures the bidirectional network bandwidth using iperf for multi-threaded connections. 
Bidirectional tests are used to test both servers for the maximum amount of throughput.

### `network-latency`

`network-latency` measures the latency using ping. The ping utility measures how long 
it takes one packet to get from one point to another.

### `network-metrics-iperf3`

`network-metrics-iperf3` measures bandwidth, jitter and bidirectional bandwidth using iperf3 on single threaded connections. The 
bandwidth test shows the speed of the data transfer. On the other hand, 
the jitter test measures the variation in the delay of received packets.

### `network-metrics-nuttcp`

`network-metrics-nuttcp` measures the UDP bandwidth using nuttcp. This tool shows the speed of the data
transfer for the UDP protocol.

### `network-nginx-ab-benchmark`

`network-nginx-ab-benchmark` measures the network performance. It uses an nginx container and runs the Apache benchmarking
tool in the host to calculate the requests per second.

### `network-metrics-cpu-consumption`

`network-metrics-cpu-consumption` measures the percentage of CPU consumption used while the maximum network bandwidth with iperf.

### `network-metrics-memory-pss`

`network-metrics-memory-pss` measures the Proportional Set Size (PSS) memory. It uses smem while running the maximum network bandwidth
with iperf. PSS is a meaningful representation of the memory applications use.

### `network-metrics-memory-rss-1g`

`network-metrics-memory-rss-1g` measures the Resident Set Size (RSS) memory using smem while running a transfer of one Gb with nuttcp.
RSS is a standard measure to monitor memory usage in a physical memory scheme.

## Running the networking tests

Before the network tests can be run, some files must be pre-processed.
The following command will both pre-process the required files and then run
the tests:

```
$ sudo -E make metrics-tests

```

Once the necessary files have been pre-processed via the above command,
individual tests may be run by hand, for example:

```
$ cd tests/metrics
$ bash network/network-metrics.sh

```
Note, some tests require `root` priviledges to operate correctly.
