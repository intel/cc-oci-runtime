# Networking image for Intel® Clear Containers performance metrics

In order to build the network image with the general tooling
and system dependencies which are used in our networking tests,
follow these steps:

1. Building the network image with the following command:

```
$ docker build -t cc-network .
```

2. Verify the network image with the following command:

```
$ docker run -ti cc-network bash

```
