#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <io.h>

typedef SOCKET socket_t;

#define get_socket_error() WSAGetLastError()

#define read _read
#define write _write

#define EWOULDBLOCK WSAEWOULDBLOCK

#else
#include <unistd.h> // close
#include <fcntl.h> // O_CLOEXEC
#include <sys/socket.h> // socklen_t
#include <sys/un.h> // AF_UNIX
#include <sys/stat.h> // fstat
#include <netdb.h> // getnameinfo
#include <netinet/tcp.h> // TCP_NODELAY

typedef int socket_t;

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#define get_socket_error() errno

#endif
