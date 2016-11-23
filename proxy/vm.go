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
	"os"
	"sync"

	"github.com/sameo/virtcontainers/hyperstart"
)

// Represents a single qemu/hyperstart instance on the system
type vm struct {
	sync.Mutex

	containerID string

	hyperHandler *hyperstart.Hyperstart

	// Used to allocate globally unique IO sequence numbers
	nextIoBase uint64

	// ios are hashed by their sequence numbers. If 2 sequence numbers are
	// allocated for one process (stdin/stdout and stderr) both sequence
	// numbers appear in this map.
	ioSessions map[uint64]*ioSession

	// Used to wait for all VM-global goroutines to finish on Close()
	wg sync.WaitGroup
}

// A set of I/O streams between a client and a process running inside the VM
type ioSession struct {
	nStreams int
	ioBase   uint64

	// socket connected to the fd sent over to the client
	client net.Conn

	// Used to wait for per-ioSession goroutines. Currently there's only
	// one such goroutine, the one reading stdin data from client socket.
	wg sync.WaitGroup
}

func newVM(id, ctlSerial, ioSerial string) *vm {
	h := hyperstart.NewHyperstart(ctlSerial, ioSerial, "unix")

	return &vm{
		containerID:  id,
		hyperHandler: h,
		nextIoBase:   1,
		ioSessions:   make(map[uint64]*ioSession),
	}
}

// Returns one chunk of data belonging to the seq stream
func readIoMessage(conn net.Conn) (seq uint64, data []byte, err error) {
	needRead := 12
	length := 0
	read := 0
	buf := make([]byte, 512)
	res := []byte{}
	for read < needRead {
		want := needRead - read
		if want > 512 {
			want = 512
		}
		nr, err := conn.Read(buf[:want])
		if err != nil {
			return 0, nil, err
		}

		res = append(res, buf[:nr]...)
		read = read + nr

		if length == 0 && read >= 12 {
			length = int(binary.BigEndian.Uint32(res[8:12]))
			if length > 12 {
				needRead = length
			}
		}
	}

	seq = binary.BigEndian.Uint64(res[:8])
	return seq, res, nil

}

func (vm *vm) findClientConn(seq uint64) net.Conn {
	vm.Lock()
	defer vm.Unlock()

	session := vm.ioSessions[seq]
	if session == nil {
		return nil
	}
	return vm.ioSessions[seq].client
}

// This function runs in a goroutine, reading data from the io channel and
// dispatching it to the right client (the one with matching seq number)
// There's only one instance of this goroutine per-VM
func (vm *vm) ioHyperToClients() {
	for {
		ttyMsg, err := vm.hyperHandler.ReadIoMessage()
		if err != nil {
			// VM process is gone
			break
		}
		length := len(ttyMsg.Message) + 12
		data := make([]byte, length)
		binary.BigEndian.PutUint64(data[:], uint64(ttyMsg.Session))
		binary.BigEndian.PutUint32(data[8:], uint32(length))
		copy(data[12:], ttyMsg.Message)

		client := vm.findClientConn(ttyMsg.Session)
		if client == nil {
			fmt.Fprintf(os.Stderr,
				"couldn't find client with seq number %d\n", ttyMsg.Session)
			continue
		}
		n, err := client.Write(data)
		if err != nil || n != len(data) {
			fmt.Fprintf(os.Stderr,
				"error writing I/O data to client: %v (%d bytes written)\n", err, n)

			break

		}
	}

	vm.wg.Done()
}

func (vm *vm) Connect() error {
	if err := vm.hyperHandler.OpenSockets(); err != nil {
		return err
	}

	if err := vm.hyperHandler.WaitForReady(); err != nil {
		vm.hyperHandler.CloseSockets()
		return err
	}

	vm.wg.Add(1)
	go vm.ioHyperToClients()

	return nil
}

func (vm *vm) SendMessage(cmd string, data []byte) error {
	_, err := vm.hyperHandler.SendCtlMessage(cmd, data)
	return err
}

// This function runs in a goroutine, reading data from the client socket and
// writing data to the hyperstart I/O chanel.
// There's one instance of this goroutine per client having done an allocateIO.
func (vm *vm) ioClientToHyper(session *ioSession) {
	for {
		seq, data, err := readIoMessage(session.client)
		if err != nil {
			// client process is gone
			break
		}

		if seq != session.ioBase {
			fmt.Fprintf(os.Stderr, "stdin seq not matching ioBase %d\n", seq)
			session.client.Close()
			break
		}

		err = vm.hyperHandler.SendIoMessage(seq, data[12:])
		if err != nil {
			fmt.Fprintf(os.Stderr,
				"error writing I/O data to hyperstart: %v\n", err)
			break
		}
	}

	session.wg.Done()
}

func (vm *vm) AllocateIo(n int, c net.Conn) uint64 {
	// Allocate ioBase
	vm.Lock()
	ioBase := vm.nextIoBase
	vm.nextIoBase += uint64(n)

	session := &ioSession{
		nStreams: n,
		ioBase:   ioBase,
		client:   c,
	}

	for i := 0; i < n; i++ {
		vm.ioSessions[ioBase+uint64(i)] = session
	}
	vm.Unlock()

	// Starts stdin forwarding between client and hyper
	session.wg.Add(1)
	go vm.ioClientToHyper(session)

	return ioBase
}

func (session *ioSession) Close() {
	session.client.Close()
	session.wg.Wait()
}

func (vm *vm) CloseIo(seq uint64) {
	vm.Lock()
	session := vm.ioSessions[seq]
	if session == nil {
		vm.Unlock()
		return
	}
	for i := 0; i < session.nStreams; i++ {
		delete(vm.ioSessions, seq+uint64(i))
	}
	vm.Unlock()

	session.Close()
}

func (vm *vm) Close() {
	vm.hyperHandler.CloseSockets()

	// Wait for per-client goroutines
	vm.Lock()
	for seq, session := range vm.ioSessions {
		delete(vm.ioSessions, seq)
		if seq != session.ioBase {
			continue
		}

		session.Close()
	}
	vm.Unlock()

	// Wait for VM global goroutines
	vm.wg.Wait()
}
