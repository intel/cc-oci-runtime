# cc-oci-runtime metrics tests

## Overview
A number of metrics tests can be found living in the
[metrics](.) directory.
These metrics can be utilised to provide information on aspects of the runtime,
such as memory footprint and execution times.

## Running the tests

### Prerequisites
Some tests require you to enable the debug mode of cc-oci-runtime. To enable it, please take a look at:
[Running under Docker](https://github.com/01org/cc-oci-runtime#18running-under-docker)


### Run all metrics tests
To run the metrics, please use the Makefile rule `metrics-tests` on the top level of this repository:

```bash
$ sudo -E PATH=$PATH make metrics-tests
```

This will run all metrics tests and place the results in the `lib/metrics/results` directory.

Each file in the `results` directory contains the result for a test as a comma separated value (CSV).

### Run a single metrics test
Some metrics tests require certain files to be pre-processed before they can be executed. To pre-process
these files execute:

```bash
$ make metrics-tests
```
Note, this command will also try to execute all the tests - you may wish to halt execution once the
tests start running, as all the pre-processed files will have been generated at that point.

To then run an individual test, you can for example:

```bash
$ cd tests/metrics
$ bash workload_time/cor_create_time.sh <times_to_run>
```

Note: some tests require `root` privilege to execute correctly.

## Internals
This section covers some of the internal APIs and tools used by the metrics tests.

### Results registration API
The metrics tests register their results via the `save_results` function located in the common test
library code. `save_results` takes four arguments:
```
save_results "Name" "TestParams" "Result" "Units
```

| Argument   | Description                                        |
| ---------- | -------------------------------------------------- |
| Name       | The name of the test. This name is generally used to form a unique test identifer if the data is ultimately stored |
| TestParams | Any arguments that were passed to the test that need recording |
| Result     | The result of the test (without unit tags)         |
| Units      | The units the Result argument is measured in (KB, Seconds etc.) |

The back end of the `save_results` function calls into the `send_results.sh` script found in the
`tests/lib` directory. The default script provided collates results into csv files under the
`tests/metrics/results` directory. The intention is that this script is replaceable by other scripts
to store results into different mediums.
`save_results` requires the `send_results.sh` script to support four arguments. These arguments
correspond directly to those of the `save_results` function above:

| Argument | Description                                        |
| -------- | -------------------------------------------------- |
| -n       | The name of the test                               |
| -a       | The test parameters                                |
| -r       | The test results                                   |
| -u       | The test result unit of measurement                |

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
