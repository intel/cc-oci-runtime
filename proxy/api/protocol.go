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

package api

import (
	"encoding/json"
)

// A Request is a JSON message sent from a client to the proxy. This message
// embed a payload identified by "id". A payload can have data associated with
// it. It's useful to think of Request as an RPC call with "id" as function
// name and "data" as arguments.
//
// The list of possible payloads are documented in this package.
//
// Each Request has a corresponding Response message sent back from the proxy.
type Request struct {
	Id   string          `json:"id"`
	Data json.RawMessage `json:"data,omitempty"`
}

// A Response is a JSON message sent back from the proxy to a client after a
// Request has been issued. The Response holds the result of the Request,
// including its success state and optional data. It's useful to think of
// Response as the result of an RPC call with ("success", "error") describing
// if the call has been successul and "data" holding the optional results.
type Response struct {
	Success bool                   `json:"success"`
	Error   string                 `json:"error,omitempty"`
	Data    map[string]interface{} `json:"data,omitempty"`
}
