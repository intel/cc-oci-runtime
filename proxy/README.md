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

The proxy protocol is composed of messages: requests and responses. These form
a small RPC protocol, requests being similar to a function call and responses
encoding the result of the call.

Each message is composed of a header and some data:

```
  ┌────────────────┬────────────────┬──────────────────────────────┐
  │  Data Length   │    Reserved    │  Data (request or response)  │
  │   (32 bits)    │    (32 bits)   │     (data length bytes)      │
  └────────────────┴────────────────┴──────────────────────────────┘
```

- `Data Length` is in bytes and encoded in network order.
- `Reserved` is reserved for future use.
- `Data` is the JSON-encoded request or response data

On top of of this request/response mechanism, the proxy defines `payloads`,
which are effectively the various function calls defined in the API.

Requests have 2 fields: the payload `id` (function name) and its `data`
(function argument(s))

```
type Request struct {
	Id    string          `json:"id"`
	Data *json.RawMessage `json:"data,omitempty"`
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

As a concrete example, here is an exchange between a client and the proxy:

```
{ "id": "hello", "data": { "containerId": "foo", "ctlSerial": "/tmp/sh.hyper.channel.0.sock", "ioSerial": "/tmp/sh.hyper.channel.1.sock"  } }
{"success":true}
```

- The client starts by calling the `hello` payload, registered the container
  `foo` and asking the proxy to connect to hyperstart communication channels
  given
- The proxy answers the function call has succeeded

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
