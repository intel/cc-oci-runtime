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
	"errors"
	"fmt"
	"math"
	"net"
	"os"
	"sync"

	hyper "github.com/hyperhq/runv/hyperstart/api/json"
)

// Represents a single qemu/hyperstart instance on the system
type vm struct {
	sync.Mutex

	containerID string

	ctlSerial, ioSerial string
	ctl, io             net.Conn

	// ctl access is arbitrated by ctlMutex. We can only allow a single
	// "transaction" (write command + read answer) at a time
	ctlMutex sync.Mutex

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

func NewVM(id, ctlSerial, ioSerial string) *vm {
	return &vm{
		containerID: id,
		ctlSerial:   ctlSerial,
		ioSerial:    ioSerial,
		nextIoBase:  1,
		ioSessions:  make(map[uint64]*ioSession),
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
	}
	if vm.io != nil {
		vm.io.Close()
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

func (vm *vm) writeMessage(m *hyper.DecodedMessage) (err error) {
	length := len(m.Message) + 8
	// XXX: Support sending messages by chunks to support messages over
	// 10240 bytes. That limit is from hyperstart src/init.c,
	// hyper_channel_ops, rbuf_size.
	if length > 10240 {
		return fmt.Errorf("message too long %d", length)
	}
	msg := make([]byte, length)
	binary.BigEndian.PutUint32(msg[:], uint32(m.Code))
	binary.BigEndian.PutUint32(msg[4:], uint32(length))
	copy(msg[8:], m.Message)

	_, err = vm.ctl.Write(msg)
	if err != nil {
		return err
	}

	// Consume the ack (NEXT) messages until we've acked the full message
	// we've just written
	acked := 0
	for acked < length {
		next, err := vm.readMessage()
		if err != nil {
			return nil
		}
		if next.Code != hyper.INIT_NEXT {
			return errors.New("didn't receive NEXT from hyperstart")
		}
		if len(next.Message) != 4 {
			return errors.New("wrong size for NEXT message")
		}
		acked += int(binary.BigEndian.Uint32(next.Message[0:4]))
	}

	// Somehow hyperstart acked more than it should?
	if acked != length {
		fmt.Fprintf(os.Stderr,
			"wrong acked size by NEXT message (expected %d, got %d)\n", length, acked)
	}

	return nil
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

// Returns one chunk of data belonging to the seq stream
func readIoMessage(conn net.Conn) (seq uint64, data []byte, err error) {
	// see runv's hypervisor/tty.go
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
		seq, data, err := readIoMessage(vm.io)
		if err != nil {
			// VM process is gone
			break
		}

		client := vm.findClientConn(seq)
		if client == nil {
			fmt.Fprintf(os.Stderr,
				"couldn't find client with seq number %d\n", seq)
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
	if err := vm.openSockets(); err != nil {
		return err
	}
	if err := vm.waitForReady(); err != nil {
		vm.closeSockets()
		vm.ctl = nil
		vm.io = nil
		return err
	}

	vm.wg.Add(1)
	go vm.ioHyperToClients()

	return nil
}

func cmdFromSring(cmd string) (uint32, error) {
	// Commands not supported by proxy:
	//   hyper.INIT_STOPPOD_DEPRECATED
	//   hyper.INIT_WRITEFILE
	//   hyper.INIT_READFILE
	switch cmd {
	case "version":
		return hyper.INIT_VERSION, nil
	case "startpod":
		return hyper.INIT_STARTPOD, nil
	case "getpod":
		return hyper.INIT_GETPOD, nil
	case "destroypod":
		return hyper.INIT_DESTROYPOD, nil
	case "restartcontainer":
		return hyper.INIT_RESTARTCONTAINER, nil
	case "execcmd":
		return hyper.INIT_EXECCMD, nil
	case "finishcmd":
		return hyper.INIT_FINISHCMD, nil
	case "ready":
		return hyper.INIT_READY, nil
	case "ack":
		return hyper.INIT_ACK, nil
	case "error":
		return hyper.INIT_ERROR, nil
	case "winsize":
		return hyper.INIT_WINSIZE, nil
	case "ping":
		return hyper.INIT_PING, nil
	case "finishpod":
		return hyper.INIT_FINISHPOD, nil
	case "next":
		return hyper.INIT_NEXT, nil
	case "newcontainer":
		return hyper.INIT_NEWCONTAINER, nil
	case "killcontainer":
		return hyper.INIT_KILLCONTAINER, nil
	case "onlinecpumem":
		return hyper.INIT_ONLINECPUMEM, nil
	case "setupinterface":
		return hyper.INIT_SETUPINTERFACE, nil
	case "setuproute":
		return hyper.INIT_SETUPROUTE, nil
	default:
		return math.MaxUint32, fmt.Errorf("unknown command '%s'", cmd)
	}
}

func (vm *vm) SendMessage(name string, data []byte) error {
	vm.ctlMutex.Lock()
	defer vm.ctlMutex.Unlock()

	cmd, err := cmdFromSring(name)
	if err != nil {
		return err
	}

	// Write message
	msg := &hyper.DecodedMessage{
		Code:    cmd,
		Message: data,
	}
	if err := vm.writeMessage(msg); err != nil {
		return err
	}

	// Wait for answer
	resp, err := vm.readMessage()
	if err != nil {
		return err
	}
	if resp.Code == hyper.INIT_ERROR {
		return errors.New("hyperstart returned an error")
	} else if resp.Code != hyper.INIT_ACK {
		return fmt.Errorf("unexpected message from hyperstart '%d'", msg.Code)
	}

	return nil
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

		n, err := vm.io.Write(data)
		if err != nil || n != len(data) {
			fmt.Fprintf(os.Stderr,
				"error writing I/O data to hyperstart: %v (%d bytes written)\n", err, n)
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
	vm.closeSockets()

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
	vm.ctl = nil
	vm.io = nil
}
