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

#include <stdio.h>

/* The shim would be handling fixed number of predefined fds.
 * This would be signal fd, stdin fd, proxy socket fd and an I/O
 * fd passed by the runtime
 */
#define MAX_POLL_FDS 4

struct cc_shim {
	char       *container_id;
	int         proxy_sock_fd;
	int         proxy_io_fd;
	uint64_t    io_seq_no;
	uint64_t    err_seq_no;
};

// Hyperstart control cmd ids : apiversion 4242
enum {
	GETVERSION,
	STARTPOD,
	GETPOD,
	STOPPOD_DEPRECATED,
	DESTROYPOD,
	RESTARTCONTAINER,
	EXECCMD,
	CMDFINISHED,
	READY,
	ACK,
	ERROR,
	WINSIZE,
	PING,
	PODFINISHED,
	NEXT,
	WRITEFILE,
	READFILE,
	NEWCONTAINER,
	KILLCONTAINER,
	ONLINECPUMEM,
	SETUPINTERFACE,
	SETUPROUTE,
	REMOVECONTAINER,
};

