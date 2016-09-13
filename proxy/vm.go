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
	"encoding/binary"
	"fmt"
	"net"

	hyper "github.com/hyperhq/runv/hyperstart/api/json"
)

// Represents a single qemu/hyperstart instance on the system
type vm struct {
	containerId string

	ctlSerial, ioSerial string
	ctl, io             net.Conn

	// ctl access is arbitrated by ctlMutex. We can only allow a single
	// "transaction" (write command + read answer) at a time
	ctlMutex sync.Mutex
}

func NewVM(id, ctlSerial, ioSerial string) *vm {
	return &vm{
		containerId: id,
		ctlSerial:   ctlSerial,
		ioSerial:    ioSerial,
	}
}

func (vm *vm) openSockets() error {
	var err error

	vm.ctl, err = net.Dial("unix", vm.ctlSerial)
	if err != nil {
		return err
	}

	vm.io, err = net.Dial("unix", vm.ioSerial)
	if err != nil {
		vm.ctl.Close()
		return err
	}

	return nil
}

func (vm *vm) closeSockets() {
	if vm.ctl != nil {
		vm.ctl.Close()
		vm.ctl = nil
	}
	if vm.io != nil {
		vm.io.Close()
		vm.io = nil
	}
}

func (vm *vm) readMessage() (*hyper.DecodedMessage, error) {
	// see runv's hypervisor/init_comm.go
	needRead := 8
	length := 0
	read := 0
	buf := make([]byte, 512)
	res := []byte{}
	for read < needRead {
		want := needRead - read
		if want > 512 {
			want = 512
		}
		nr, err := vm.ctl.Read(buf[:want])
		if err != nil {
			return nil, err
		}

		res = append(res, buf[:nr]...)
		read = read + nr

		if length == 0 && read >= 8 {
			length = int(binary.BigEndian.Uint32(res[4:8]))
			if length > 8 {
				needRead = length
			}
		}
	}

	return &hyper.DecodedMessage{
		Code:    binary.BigEndian.Uint32(res[:4]),
		Message: res[8:],
	}, nil
}

func (vm *vm) waitForReady() error {
	vm.ctlMutex.Lock()
	defer vm.ctlMutex.Unlock()

	msg, err := vm.readMessage()
	if err != nil {
		return err
	} else if msg.Code != hyper.INIT_READY {
		return fmt.Errorf("Unexpected message %d", msg.Code)
	}

	return nil
}

func (vm *vm) Connect() error {
	if err := vm.openSockets(); err != nil {
		return err
	}
	if err := vm.waitForReady(); err != nil {
		vm.closePipes()
		return err
	}
	return nil
}

func (vm *vm) Close() {
	vm.closeSockets()
}
