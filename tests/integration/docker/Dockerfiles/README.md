# NGINX image for Intel® Clear Containers swarm tests

In order to build the nginx image which is used in our swarm tests,
follow these steps:

1. Build the nginx image with the following command:

```
$ docker build -t $name -f Dockerfile.nginx .
```

2. Verify the nginx image with the following command:

```
$ docker run -ti $name bash

```
