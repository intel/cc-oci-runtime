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

// Package api defines the API cc-proxy exposes to clients, processes
// connecting to the proxy AF_UNIX socket.
package api

import (
	"encoding/json"
)

// The hello payload is issued first after connecting to the proxy socket.
// It is used to let the proxy know about a new container on the system along
// with the paths go hyperstart's command and I/O channels (AF_UNIX sockets).
//
//  {
//    "id": "hello",
//    "data": {
//      "containerId": "756535dc6e9ab9b560f84c8...",
//      "ctlSerial": "/tmp/sh.hyper.channel.0.sock",
//      "ioSerial": "/tmp/sh.hyper.channel.1.sock"
//    }
//  }
type Hello struct {
	ContainerId string `json:"containerId"`
	CtlSerial   string `json:"ctlSerial"`
	IoSerial    string `json:"ioSerial"`
}

// The bye payload does the opposite of what hello does, indicating to the
// proxy it should release resources created by hello. This command has no
// parameter.
//
//  {
//    "id": "bye"
//  }
type Bye struct {
}

// The hyper payload will forward an hyperstart command to hyperstart.
//
//  {
//    "id": "hyper",
//    "data": {
//      "hyperName": "ping"
//    }
//  }
type Hyper struct {
	HyperName string          `json:"hyperName"`
	Data      json.RawMessage `json:"data,omitempty"`
}
