# `cc-shim`

`cc-shim` is a process spawned by the runtime per container workload. The runtime 
provides the pid of the cc-shim process to containerd-shim on OCI create command.

Usage:
	cc-shim -c $(container_id) -p $(proxy_socket_fd)

`cc-shim` forwards all signals to the cc-proxy process to be handled by the agent
in the VM.

TODO:
It should also forward any input received from containerd-shim to cc-proxy and 
forward the stdout/stderr received from the proxy to containerd-shim.
The shim should capture the exit status of the container and exit with that exit code.
