# `cc-proxy`

`cc-proxy` is a daemon offering access to the
[`hyperstart`](https://github.com/hyperhq/hyperstart) VM agent to multiple
clients on the host.

![High-level Architecture Diagram](../documentation/high-level-overview.png)

- The `hyperstart` interface consists of:
    - A control channel on which the [`hyperstart` API]
      (https://github.com/hyperhq/runv/tree/master/hyperstart/api/json) is
      delivered.
    - An I/O channel with the stdin/stout/stderr streams of the processes
      running inside the VM multiplexed onto.
- `cc-proxy`'s main role is to:
    - Arbitrate access to the `hyperstart` control channel between all the
      instances of the OCI runtimes and `cc-shim`.
    - Route the I/O streams between the various shim instances and `hyperstart`.
- There's only one `cc-proxy` per host.
 

`cc-proxy` itself has an API to setup the route to the hypervisor/hyperstart
and to forward `hyperstart` commands. This API is done with a small JSON RPC
protocol on an `AF_UNIX` located at: `${localstatesdir}/run/cc-oci-runtime/proxy.sock`

## Protocol

The proxy protocol is composed of JSON messages: requests and responses. Each
of these message is on a separate line. The snippet below is showing two
requests (`hello` and `hyper` commands) and their corresponding responses:

```
{ "id": "hello", "data": { "containerId": "foo", "ctlSerial": "/tmp/sh.hyper.channel.0.sock", "ioSerial": "/tmp/sh.hyper.channel.1.sock"  } }
{"success":true}
{"id": "hyper", "data": { "name": "ping" }}
{"success":true}
```

Requests have 2 fields: the payload `id` and and its `data`.

```
type Request struct {
	Id    string          `json:"id"`
	Data *json.RawMessage `json:"data"`
}
```

Responses have 3 fields: `success`, `error` and `data`

```
type Response struct {
	Success bool                   `json:"success"`
	Error   string                 `json:"error,omitempty"`
	Data    map[string]interface{} `json:"data,omitempty"`
}
```

Unsurprisingly, the response has the result of a command, with `success`
indicating if the request has succeeded for not. If `success` is `true`, the
response can carry additional return values in `data`. If success if `false`,
`error` may contain an error string.

## Payloads

Payloads are in their own package and [documented there](
https://godoc.org/github.com/01org/cc-oci-runtime/proxy/api)

## `systemd` integration

When compiling in the presence of the systemd pkg-config file, two systemd unit
files are created and installed.

  - `cc-proxy.service`: the usual service unit file
  - `cc-proxy.socket`: the socket activation unit

The proxy doesn't have to run all the time, just when a Clear Container is
running. Socket activation can be used to start the proxy when a client
connects to the socket for the first time.

After having run `make install`, socket action is enabled with:

```
sudo systemctl enable cc-proxy.socket
```

The proxy can output log messages on stderr, which are automatically
handled by systemd and can be viewed with:

```
journalctl -u cc-proxy -f
```
