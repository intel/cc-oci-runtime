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
	"fmt"
	"net"
	"os"
	"sync"

	"github.com/01org/cc-oci-runtime/proxy/api"
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
	proxy *proxy
	vm    *vm

	conn net.Conn
}

// "hello"
func helloHandler(data []byte, userData interface{}, response *HandlerResponse) {
	client := userData.(*client)
	hello := api.Hello{}

	if err := json.Unmarshal(data, &hello); err != nil {
		response.SetError(err)
		return
	}

	if hello.ContainerId == "" || hello.CtlSerial == "" || hello.IoSerial == "" {
		response.SetErrorMsg("malformed hello command")
	}

	proxy := client.proxy
	proxy.Lock()
	if _, ok := proxy.vms[hello.ContainerId]; ok {

		proxy.Unlock()
		response.SetErrorf("%s: container already registered",
			hello.ContainerId)
		return
	}
	vm := NewVM(hello.ContainerId, hello.CtlSerial, hello.IoSerial)
	proxy.vms[hello.ContainerId] = vm
	proxy.Unlock()

	if err := vm.Connect(); err != nil {
		proxy.Lock()
		delete(proxy.vms, hello.ContainerId)
		proxy.Unlock()
		response.SetError(err)
		return
	}

	client.vm = vm
}

// "attach"
func attachHandler(data []byte, userData interface{}, response *HandlerResponse) {
	client := userData.(*client)
	proxy := client.proxy

	attach := api.Attach{}
	if err := json.Unmarshal(data, &attach); err != nil {
		response.SetError(err)
		return
	}

	proxy.Lock()
	vm := proxy.vms[attach.ContainerId]
	proxy.Unlock()

	if vm == nil {
		response.SetErrorf("unknown containerId: %s", attach.ContainerId)
		return
	}

	client.vm = vm
}

// "bye"
func byeHandler(data []byte, userData interface{}, response *HandlerResponse) {
	client := userData.(*client)
	proxy := client.proxy
	vm := client.vm

	if vm == nil {
		response.SetErrorMsg("client not attached to a vm")
		return
	}

	proxy.Lock()
	delete(proxy.vms, vm.containerId)
	proxy.Unlock()

	client.vm = nil
	vm.Close()
}

// "allocateIO"
func allocateIoHandler(data []byte, userData interface{}, response *HandlerResponse) {
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

	ioBase := vm.AllocateIo(allocateIo.NStreams, c1)

	response.AddResult("ioBase", ioBase)
	response.SetFile(f0)

	// File() dups the underlying fd, so it's safe to close c0 here (will
	// keep the c0 <-> c1 connection alive).
	c0.Close()
}

// "hyper"
func hyperHandler(data []byte, userData interface{}, response *HandlerResponse) {
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

	err := vm.SendMessage(hyper.HyperName, hyper.Data)
	response.SetError(err)
}

func NewProxy() *proxy {
	return &proxy{
		vms: make(map[string]*vm),
	}
}

// This variable is populated at link time with the value of:
//   ${locatestatedir}/run/cc-oci-runtime/proxy
var socketPath string

func (proxy *proxy) Init() error {
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
			return fmt.Errorf("couldn't remove exiting socket: %v\n", err)
		}
		l, err = net.ListenUnix("unix", &net.UnixAddr{socketPath, "unix"})
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

func (proxy *proxy) Serve() {

	// Define the client (runtime/shim) <-> proxy protocol
	proto := NewProtocol()
	proto.Handle("hello", helloHandler)
	proto.Handle("attach", attachHandler)
	proto.Handle("bye", byeHandler)
	proto.Handle("allocateIO", allocateIoHandler)
	proto.Handle("hyper", hyperHandler)

	for {
		conn, err := proxy.listener.Accept()
		if err != nil {
			fmt.Fprintln(os.Stderr, "couldn't accept connection:", err)
			continue
		}

		newClient := &client{
			proxy: proxy,
			conn:  conn,
		}

		go proto.Serve(conn, newClient)
	}
}

func proxyMain() {
	proxy := NewProxy()
	if err := proxy.Init(); err != nil {
		fmt.Fprintln(os.Stderr, "init:", err.Error())
		os.Exit(1)
	}
	proxy.Serve()
}

func main() {
	proxyMain()
}
