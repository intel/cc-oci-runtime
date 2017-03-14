# cc-oci-runtime metrics tests

### Prerequisites
Some tests require you to enable the debug mode of cc-oci-runtime. To enable it, please take a look at:
https://github.com/01org/cc-oci-runtime#18running-under-docker

To run the metrics, just run:

```bash
$ cd tests/metrics
$ ./run_docker_metrics
```

This will run all metrics tests and generete a `results` directory.

Each file in the `result` directory contains the result for a test and has the format of a CSV.
At the end of each file you will find the result average of all the data collected by the test.

You can also run each tests script separately. e.g.

```bash
$ cd tests/metrics
$ bash workload_time/cor_create_time.sh <times_to_run>
```

### Metrics tool
The objective of `collect_mem_consmd.sh` tool is to measure the average memory consumption of any
Clear Containers components under a determined stress test. This test launches a user defined number
of Clear Containers (by default 5) simultaneously, and once all containers are running then start
a similar workload inside each CC instance. The number of Clear Containers launched and the workload
can be customized.

**Requirments:**
collectl

| Option | Description                                          |
| ------ | ---------------------------------------------------- |
| -h     | Help Page.                                           |
| -c     | Number of containers.                                |
| -m     | Number of messages (cc workload).                    |
| -s     | Size of the message.                                 |
| -p     | CC component name to be monitered (default cc-proxy) |
| -x     | Get metrics just about exact match.                  |
| -v     | Shows version                                        |

**Usage example:**

```bash
# ./collect_mem_consmd.sh -c 50 -m 1000 -p cc-shim -x
```

### Mapping tool

The `map_mem.sh` tool will report the memory map of a process in a JSON file.
Each mapping has the following information:

- Proportional set size
- Shared memory
- Private memory

**Usage example:**

```bash
# ./map_mem.sh cc-proxy
```
