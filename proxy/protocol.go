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
	"errors"
	"fmt"
	"net"
	"os"

	"github.com/01org/cc-oci-runtime/proxy/api"
)

// XXX: could do with its own package to remove that ugly namespacing
type ProtocolHandler func([]byte, interface{}, *HandlerResponse)

// Encapsulates the different parts of what a handler can return.
type HandlerResponse struct {
	err     error
	results map[string]interface{}
	file    *os.File
}

func (r *HandlerResponse) SetError(err error) {
	r.err = err
}

func (r *HandlerResponse) SetErrorMsg(msg string) {
	r.err = errors.New(msg)
}

func (r *HandlerResponse) SetErrorf(format string, a ...interface{}) {
	r.SetError(fmt.Errorf(format, a...))
}

func (r *HandlerResponse) SetResults(results map[string]interface{}) {
	r.results = results
}

func (r *HandlerResponse) AddResult(key string, value interface{}) {
	if r.results == nil {
		r.results = make(map[string]interface{})
	}
	r.results[key] = value
}

func (r *HandlerResponse) SetFile(f *os.File) {
	r.file = f
}

type Protocol struct {
	handlers map[string]ProtocolHandler
}

func NewProtocol() *Protocol {
	return &Protocol{
		handlers: make(map[string]ProtocolHandler),
	}
}

func (proto *Protocol) Handle(cmd string, handler ProtocolHandler) {
	proto.handlers[cmd] = handler
}

type clientCtx struct {
	conn net.Conn

	userData interface{}
}

func (proto *Protocol) handleRequest(ctx *clientCtx, req *api.Request, hr *HandlerResponse) *api.Response {
	if req.Id == "" {
		return &api.Response{
			Success: false,
			Error:   "no 'id' field in request",
		}
	}

	handler, ok := proto.handlers[req.Id]
	if !ok {
		return &api.Response{
			Success: false,
			Error:   fmt.Sprintf("no payload named '%s'", req.Id),
		}
	}

	handler(req.Data, ctx.userData, hr)
	if hr.err != nil {
		return &api.Response{
			Success: false,
			Error:   hr.err.Error(),
			Data:    hr.results,
		}
	} else {
		return &api.Response{
			Success: true,
			Data:    hr.results,
		}
	}
}

func (proto *Protocol) Serve(conn net.Conn, userData interface{}) {
	ctx := &clientCtx{
		conn:     conn,
		userData: userData,
	}

	for {
		// Parse a request.
		req := api.Request{}
		hr := HandlerResponse{}

		err := api.ReadMessage(conn, &req)
		if err != nil {
			// EOF or the client isn't even sending proper JSON,
			// just kill the connection
			conn.Close()
			return
		}

		// Execute the corresponding handler
		resp := proto.handleRequest(ctx, &req, &hr)

		// Send the response back to the client.
		if err = api.WriteMessage(conn, resp); err != nil {
			// Something made us unable to write the response back
			// to the client (could be a disconnection, ...).
			fmt.Fprintf(os.Stderr, "couldn't encode response: %v\n", err)
			conn.Close()
			return
		}

		// And send a fd if the handler associated a file with the response
		if hr.file != nil {
			if err = api.WriteFd(conn.(*net.UnixConn), int(hr.file.Fd())); err != nil {
				fmt.Fprintf(os.Stderr, "error sending fd: %v\n", err)
			}
			hr.file.Close()
		}

	}
}
