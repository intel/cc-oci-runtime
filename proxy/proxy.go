// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"sync"
	"sync/atomic"

	"github.com/01org/cc-oci-runtime/proxy/api"

	"github.com/golang/glog"
)

// Main struct holding the proxy state
type proxy struct {
	// Protect concurrent accesses from separate client goroutines to this
	// structure fields
	sync.Mutex

	// proxy socket
	listener net.Listener

	// vms are hashed by their containerID
	vms map[string]*vm
}

// Represents a client, either a cc-oci-runtime or cc-shim process having
// opened a socket to the proxy
type client struct {
	id    uint64
	proxy *proxy
	vm    *vm

	conn net.Conn
}

func (c *client) info(lvl glog.Level, msg string) {
	if !glog.V(lvl) {
		return
	}
	glog.Infof("[client #%d] %s", c.id, msg)
}

func (c *client) infof(lvl glog.Level, fmt string, a ...interface{}) {
	if !glog.V(lvl) {
		return
	}
	a = append(a, 0)
	copy(a[1:], a[0:])
	a[0] = c.id
	glog.Infof("[client #%d] "+fmt, a...)
}

// "hello"
func helloHandler(data []byte, userData interface{}, response *handlerResponse) {
	client := userData.(*client)
	hello := api.Hello{}

	if err := json.Unmarshal(data, &hello); err != nil {
		response.SetError(err)
		return
	}

	if hello.ContainerID == "" || hello.CtlSerial == "" || hello.IoSerial == "" {
		response.SetErrorMsg("malformed hello command")
	}

	proxy := client.proxy
	proxy.Lock()
	if _, ok := proxy.vms[hello.ContainerID]; ok {

		proxy.Unlock()
		response.SetErrorf("%s: container already registered",
			hello.ContainerID)
		return
	}

	client.infof(1, "hello(containerId=%s,ctlSerial=%s,ioSerial=%s", hello.ContainerID,
		hello.CtlSerial, hello.IoSerial)

	vm := newVM(hello.ContainerID, hello.CtlSerial, hello.IoSerial)
	proxy.vms[hello.ContainerID] = vm
	proxy.Unlock()

	if err := vm.Connect(); err != nil {
		proxy.Lock()
		delete(proxy.vms, hello.ContainerID)
		proxy.Unlock()
		response.SetError(err)
		return
	}

	client.vm = vm
}

// "attach"
func attachHandler(data []byte, userData interface{}, response *handlerResponse) {
	client := userData.(*client)
	proxy := client.proxy

	attach := api.Attach{}
	if err := json.Unmarshal(data, &attach); err != nil {
		response.SetError(err)
		return
	}

	proxy.Lock()
	vm := proxy.vms[attach.ContainerID]
	proxy.Unlock()

	if vm == nil {
		response.SetErrorf("unknown containerID: %s", attach.ContainerID)
		return
	}

	client.infof(1, "attach(containerId=%s)", attach.ContainerID)

	client.vm = vm
}

// "bye"
func byeHandler(data []byte, userData interface{}, response *handlerResponse) {
	client := userData.(*client)
	proxy := client.proxy

	bye := api.Bye{}
	if err := json.Unmarshal(data, &bye); err != nil {
		response.SetError(err)
		return
	}

	proxy.Lock()
	vm := proxy.vms[bye.ContainerID]
	proxy.Unlock()

	if vm == nil {
		response.SetErrorf("unknown containerID: %s", bye.ContainerID)
		return
	}

	client.info(1, "bye()")

	proxy.Lock()
	delete(proxy.vms, vm.containerID)
	proxy.Unlock()

	client.vm = nil
	vm.Close()
}

// "allocateIO"
func allocateIoHandler(data []byte, userData interface{}, response *handlerResponse) {
	client := userData.(*client)
	vm := client.vm

	allocateIo := api.AllocateIo{}
	if err := json.Unmarshal(data, &allocateIo); err != nil {
		response.SetError(err)
		return
	}

	if allocateIo.NStreams < 1 || allocateIo.NStreams > 2 {
		response.SetErrorf("asking for unexpected number of streams (%d)",
			allocateIo.NStreams)
	}

	if vm == nil {
		response.SetErrorMsg("client not attached to a vm")
		return
	}

	client.infof(1, "allocateIo(nStreams=%d)", allocateIo.NStreams)

	// We'll send c0 to the client, keep c1
	c0, c1, err := Socketpair()
	if err != nil {
		response.SetError(err)
		return
	}

	f0, err := c0.File()
	if err != nil {
		response.SetError(err)
		return
	}

	ioBase := vm.AllocateIo(allocateIo.NStreams, client.id, c1)

	client.infof(1, "-> %d streams allocated, ioBase=%d", allocateIo.NStreams, ioBase)

	response.AddResult("ioBase", ioBase)
	response.SetFile(f0)

	// File() dups the underlying fd, so it's safe to close c0 here (will
	// keep the c0 <-> c1 connection alive).
	c0.Close()
}

// "hyper"
func hyperHandler(data []byte, userData interface{}, response *handlerResponse) {
	client := userData.(*client)
	hyper := api.Hyper{}
	vm := client.vm

	if err := json.Unmarshal(data, &hyper); err != nil {
		response.SetError(err)
		return
	}

	if vm == nil {
		response.SetErrorMsg("client not attached to a vm")
		return
	}

	client.infof(1, "cmd=%s, data=%s)", hyper.HyperName, hyper.Data)

	err := vm.SendMessage(hyper.HyperName, hyper.Data)
	response.SetError(err)
}

func newProxy() *proxy {
	return &proxy{
		vms: make(map[string]*vm),
	}
}

// This variable is populated at link time with the value of:
//   ${locatestatedir}/run/cc-oci-runtime/proxy
var socketPath string

func (proxy *proxy) init() error {
	var l net.Listener
	var err error

	fds := listenFds()

	if len(fds) > 1 {
		return fmt.Errorf("too many activated sockets (%d)", len(fds))
	} else if len(fds) == 1 {
		fd := fds[0]
		l, err = net.FileListener(fd)
		if err != nil {
			return fmt.Errorf("couldn't listen on socket: %v", err)
		}
	} else {
		if err = os.Remove(socketPath); err != nil && !os.IsNotExist(err) {
			return fmt.Errorf("couldn't remove exiting socket: %v", err)
		}
		l, err = net.ListenUnix("unix", &net.UnixAddr{Name: socketPath, Net: "unix"})
		if err != nil {
			return fmt.Errorf("couldn't create AF_UNIX socket: %v", err)
		}
		if err = os.Chmod(socketPath, 0666|os.ModeSocket); err != nil {
			return fmt.Errorf("couldn't set mode on socket: %v", err)
		}
	}

	proxy.listener = l

	return nil
}

var nextClientID = uint64(1)

func (proxy *proxy) serveNewClient(proto *protocol, newConn net.Conn) {
	newClient := &client{
		id:    nextClientID,
		proxy: proxy,
		conn:  newConn,
	}

	atomic.AddUint64(&nextClientID, 1)

	// Unfortunately it's hard to find out information on the peer
	// at the other end of a unix socket. We use a per-client ID to
	// identify connections.
	newClient.info(1, "client connected")

	if err := proto.Serve(newConn, newClient); err != nil && err != io.EOF {
		newClient.infof(1, "error serving client: %v", err)
	}

	newConn.Close()
	newClient.info(1, "connection closed")
}

func (proxy *proxy) serve() {

	// Define the client (runtime/shim) <-> proxy protocol
	proto := newProtocol()
	proto.Handle("hello", helloHandler)
	proto.Handle("attach", attachHandler)
	proto.Handle("bye", byeHandler)
	proto.Handle("allocateIO", allocateIoHandler)
	proto.Handle("hyper", hyperHandler)

	glog.V(1).Info("proxy started")

	for {
		conn, err := proxy.listener.Accept()
		if err != nil {
			fmt.Fprintln(os.Stderr, "couldn't accept connection:", err)
			continue
		}

		go proxy.serveNewClient(proto, conn)
	}
}

func proxyMain() {
	proxy := newProxy()
	if err := proxy.init(); err != nil {
		fmt.Fprintln(os.Stderr, "init:", err.Error())
		os.Exit(1)
	}
	proxy.serve()
}

func initLogging() {
	// We print logs on stderr by default.
	flag.Set("logtostderr", "true")

	// It can be practical to use an environment variable to trigger a verbose output
	level := os.Getenv("CC_PROXY_LOG_LEVEL")
	if level != "" {
		flag.Set("v", level)
	}
}

func main() {
	initLogging()

	flag.Parse()
	defer glog.Flush()

	proxyMain()
}
