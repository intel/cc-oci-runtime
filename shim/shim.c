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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <assert.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

/* The shim would be handling fixed number of predefined fds.
 * This would be signal fd, stdin fd, proxy socket fd and an I/O
 * fd passed by the runtime
 */
#define MAX_POLL_FDS 4

/* globals */
/* Pipe used for capturing signal occurence */
int signal_pipe_fd[2];

/*!
 * Add file descriptor to the array of polled descriptors
 *
 * \param poll_fds Array of polled fds
 * \param nfds Number of fds present in the array
 * \param fd File descriptor to add
 * \param events Events for the fd that should be polled
 */
void add_pollfd(struct pollfd *poll_fds, nfds_t *nfds, int fd,  short events) {
	struct pollfd pfd = { 0 };

	assert(*nfds < MAX_POLL_FDS);
	pfd.fd = fd;
	pfd.events = events;
	poll_fds[*nfds] = pfd;
	(*nfds)++;
}

/*!
 * Signal handler for the signals that should be caught and 
 * forwarded to the proxy
 *
 * \param signal_no Signal number of the signal caught
 */
static void
signal_handler(int signal_no)
{
	int savedErrno;                     /* In case we change 'errno' */
 
	savedErrno = errno;
	/* Write signal number to pipe, so that the signal can be identfied later */
	if (write(signal_pipe_fd[1], &signal_no, sizeof(signal_no)) == -1 && errno != EAGAIN) {
		return;
	}
	errno = savedErrno;
}

/*!
 * Print formatted message to stderr and exit with EXIT_FAILURE
 *
 * \param format Format that specifies how subsequent arguments are
 *  converted for output
 */
void
err_exit(const char *format, ...)
{
	va_list	args;

	if ( !format) {
		return;
	}
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

/*!
 * Set file descriptor as non-blocking
 *
 * \param fd File descriptor to set as non-blocking
 */
void
set_fd_nonblocking(int fd)
{
	int flags;

	if (fd < 0) {
		return;
	}

	flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		err_exit("Error getting status flags for fd: %s\n", strerror(errno));
	}
	flags |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) == -1) {
		err_exit("Error setting fd as nonblocking: %s\n", strerror(errno));
	}
}

/*!
 * Read data from infd and write to outfd
 *
 * \param infd File descriptor to read from
 * \param outfd File descriptor to write to
 */
void
handle_data(int infd, int outfd)
{
	ssize_t    nread;
	char       buf[BUFSIZ];
	int        ret;

	if ( infd < 0 || outfd < 0) {
		return;
	}

	while ((nread = read(infd, buf, BUFSIZ))!= -1) {
		ret = (int)write(outfd, buf, (size_t)nread);
		if (ret == -1) {
			fprintf(stderr, "Error writing from fd %d to fd %d: %s\n", infd, outfd, strerror(errno));
			return;
		}
	}
}

int
main(int argc, char **argv)
{
	int proxy_sockfd;
	struct sockaddr_un proxy_address;
	char* container_id;
	char* proxy_sock_path;
	struct pollfd poll_fds[MAX_POLL_FDS];
	nfds_t nfds = 0;
	int ret;
	struct sigaction sa;

	if (argc != 3) {
		printf("Usage: cc-shim container_id proxy_socket_path");
		exit(EXIT_FAILURE);
	}

	container_id = argv[1];
	proxy_sock_path = argv[2];

	/* Using self pipe trick to handle signals in the main loop, other strategy
	 * would be to clock signals and use signalfd()/ to handle signals synchronously
	 */
	if (pipe(signal_pipe_fd) == -1) {
		printf("Error creating pipe\n");
		exit(EXIT_FAILURE);
	}

	// Add read end of pipe to pollfd list and make it non-bocking
	add_pollfd(poll_fds, &nfds, signal_pipe_fd[0], POLLIN | POLLPRI);
	set_fd_nonblocking(signal_pipe_fd[0]);
	set_fd_nonblocking(signal_pipe_fd[1]);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;           /* Restart interrupted reads()s */
	sa.sa_handler = signal_handler;

	/* TODO: Add signal handler for all signals */
	if (sigaction(SIGINT, &sa, NULL) == -1)
		err_exit("sigaction");
	if (sigaction(SIGTERM, &sa, NULL) == -1)
		err_exit("sigaction");
	if (sigaction(SIGWINCH, &sa, NULL) == -1)
		err_exit("sigaction");

	set_fd_nonblocking(STDIN_FILENO);
	add_pollfd(poll_fds, &nfds, STDIN_FILENO, POLLIN | POLLPRI);

#if 0
	// Connect to the cc-proxy AF_UNIX server
	proxy_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	strcpy(proxy_address.sun_path, proxy_sock_path);
	//block waiting to connect to the cc_proxy
	connect(proxy_sockfd, (struct sockaddr *)&proxy_address, sizeof(proxy_address));

	// Add the proxy socket to the list of pollfds
	add_pollfd(poll_fds, &nfds, proxy_sockfd, POLLIN | POLLPRI);
#endif

	/*	0 =>signal_pipe_fd[0]
		1 =>stdin
		2 =>sockfd
	*/

	while (1) {
		ret = poll(poll_fds, nfds, -1);
		if (ret == -1 && errno != EINTR) {
			err_exit("Error in poll : %s\n", strerror(errno));
		}

		/* check if signal was received first */
		if (poll_fds[0].revents != 0) {
			//TODO: Handle signal
		}

		// check stdin fd
		if (poll_fds[1].revents != 0) {
			// Sending stdin to stdout, this should go to proxy tty fd later
			handle_data((int)STDIN_FILENO, (int)STDOUT_FILENO);
		}

#if 0
		// check for proxy sockfd
		if (poll_fds[2].revents != 0) {
		}
#endif
	}
	return 0;
}
