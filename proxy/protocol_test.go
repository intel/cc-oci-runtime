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
	"errors"
	"flag"
	"io"
	"net"
	"os"
	"sync"
	"syscall"
	"testing"

	"github.com/stretchr/testify/assert"
)

// A simple way to mock a net.Conn around syscall.socketpair()
type mockServer struct {
	t                      *testing.T
	proto                  *Protocol
	serverConn, clientConn net.Conn
}

func Socketpair(t *testing.T) (*net.UnixConn, *net.UnixConn) {
	fds, err := syscall.Socketpair(syscall.AF_UNIX, syscall.SOCK_STREAM, 0)
	assert.Nil(t, err)

	// First end
	f0 := os.NewFile(uintptr(fds[0]), "")
	c0, err := net.FileConn(f0)
	assert.Nil(t, err)

	// Second end
	f1 := os.NewFile(uintptr(fds[1]), "")
	c1, err := net.FileConn(f1)
	assert.Nil(t, err)

	// os.NewFile() dups the fd and we're reponsible for closing it
	f0.Close()
	f1.Close()

	return c0.(*net.UnixConn), c1.(*net.UnixConn)
}

func NewMockServer(t *testing.T, proto *Protocol) *mockServer {
	server := &mockServer{
		t:     t,
		proto: proto,
	}

	server.serverConn, server.clientConn = Socketpair(t)

	return server
}

func (server *mockServer) GetClientConn() net.Conn {
	return server.clientConn
}

func (server *mockServer) Serve() {
	server.ServeWithUserData(nil)
}

func (server *mockServer) ServeWithUserData(userData interface{}) {
	server.proto.Serve(server.serverConn, userData)
}

func setupMockServer(t *testing.T, proto *Protocol) (client net.Conn, server *mockServer) {
	server = NewMockServer(t, proto)
	client = server.GetClientConn()
	go server.Serve()

	return client, server
}

// Test that we correctly give back the user data to handlers
type myUserData struct {
	t  *testing.T
	wg sync.WaitGroup
}

var testUserData myUserData

func userDataHandler(data []byte, userData interface{}) (map[string]interface{}, error) {
	p := userData.(*myUserData)
	assert.Equal(p.t, p, &testUserData)

	p.wg.Done()

	return nil, nil
}

func TestUserData(t *testing.T) {
	proto := NewProtocol()
	proto.Handle("foo", userDataHandler)

	server := NewMockServer(t, proto)
	client := server.GetClientConn()
	testUserData.t = t
	go server.ServeWithUserData(&testUserData)

	testUserData.wg.Add(1)
	client.Write([]byte(`{ "id": "foo" }`))

	// make sure the handler runs by waiting for it
	testUserData.wg.Wait()
}

// Tests various behaviours of the protocol main loop and handler dispatching
func simpleHandler(data []byte, userData interface{}) (map[string]interface{}, error) {
	return nil, nil
}

type Echo struct {
	Arg string
}

func echoHandler(data []byte, userData interface{}) (map[string]interface{}, error) {
	echo := Echo{}
	json.Unmarshal(data, &echo)

	return map[string]interface{}{
		"result": echo.Arg,
	}, nil
}

func returnDataHandler(data []byte, userData interface{}) (map[string]interface{}, error) {
	return map[string]interface{}{
		"foo": "bar",
	}, nil
}

func returnErrorHandler(data []byte, userData interface{}) (map[string]interface{}, error) {
	return nil, errors.New("This is an error")
}

func returnDataErrorHandler(data []byte, userData interface{}) (map[string]interface{}, error) {
	return map[string]interface{}{
		"foo": "bar",
	}, errors.New("This is an error")
}

func TestProtocol(t *testing.T) {
	tests := []struct {
		input, output string
	}{
		{`{"id": "simple"}`, `{"success":true}` + "\n"},
		{`{"id": "notfound"}`,
			`{"success":false,"error":"no payload named 'notfound'"}` + "\n"},
		{`{"foo": "bar"}`,
			`{"success":false,"error":"no 'id' field in request"}` + "\n"},
		// Tests return values from handlers
		{`{"id":"returnData", "data": {"arg": "bar"}}`,
			`{"success":true,"data":{"foo":"bar"}}` + "\n"},
		{`{"id":"returnError" }`,
			`{"success":false,"error":"This is an error"}` + "\n"},
		{`{"id":"returnDataError", "data": {"arg": "bar"}}`,
			`{"success":false,"error":"This is an error","data":{"foo":"bar"}}` + "\n"},
		// Tests we can unmarshal payload data
		{`{"id":"echo", "data": {"arg": "ping"}}`,
			`{"success":true,"data":{"result":"ping"}}` + "\n"},
	}

	proto := NewProtocol()
	proto.Handle("simple", simpleHandler)
	proto.Handle("returnData", returnDataHandler)
	proto.Handle("returnError", returnErrorHandler)
	proto.Handle("returnDataError", returnDataErrorHandler)
	proto.Handle("echo", echoHandler)

	client, _ := setupMockServer(t, proto)

	buf := make([]byte, 512)
	for _, test := range tests {
		// request
		n, err := client.Write([]byte(test.input))
		assert.Nil(t, err)
		assert.Equal(t, n, len(test.input))

		// response
		n, err = client.Read(buf)
		assert.Nil(t, err)
		assert.Equal(t, test.output, string(buf[:n]))
	}
}

// Make sure the server closes the connection when encountering an error
func TestCloseOnError(t *testing.T) {
	proto := NewProtocol()
	proto.Handle("simple", simpleHandler)

	client, _ := setupMockServer(t, proto)

	// request
	const garbage string = "sekjewr"
	n, err := client.Write([]byte(garbage))
	assert.Nil(t, err)
	assert.Equal(t, n, len(garbage))

	// response
	buf := make([]byte, 512)
	_, err = client.Read(buf)
	assert.Equal(t, err, io.EOF)
}

func TestMain(m *testing.M) {
	flag.Parse()
	os.Exit(m.Run())
}
