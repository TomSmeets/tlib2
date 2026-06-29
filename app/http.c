#include "fmt.h"
#include "io.h"
#include "os_headers.h"
#include "os_main.h"

#if OS_LINUX

typedef enum {
    AF_UNIX = 1,
    AF_INET = 2,
} sock_family;

typedef enum {
    SOCK_STREAM = 1,
    SOCK_DGRAM = 2,
} sock_type;

static i64 linux_socket(sock_family family, sock_type type, int protocol) {
    return linux_syscall3(0x29, family, type, protocol);
}

typedef struct {
    u16 sin_family;
    u16 sin_port; // Big endian
    u8 sin_addr[4];
    u8 pad[16 - 8];
} sockaddr_in;

static i64 linux_bind(int fd, sockaddr_in *addr) {
    return linux_syscall3(0x31, fd, (i64)(void *)addr, sizeof(sockaddr_in));
}

static i64 linux_listen(int fd, int backlog) {
    return linux_syscall2(0x32, fd, backlog);
}

static i64 linux_accept(int fd, sockaddr_in *addr, int flags) {
    int len = sizeof(sockaddr_in);
    return linux_syscall4(0x120, fd, (intptr_t)addr, (intptr_t)&len, flags);
}

static u32 net_ip4(u8 a, u8 b, u8 c, u8 d) {
    u32 res = 0;
    res |= (u32)a << 0;
    res |= (u32)b << 8;
    res |= (u32)c << 16;
    res |= (u32)d << 24;
    return res;
}

static u16 net_port(u16 port) {
    return (u16)((port >> 8) & 0xff) | ((port & 0xff) << 8);
}

#endif

static u16 u16_swap(u16 x) {
    return (x >> 8) | (x << 8);
}

static void os_main(void) {
    int fd = linux_socket(AF_INET, SOCK_STREAM, 0);
    check(fd >= 0);

    // Initialize the details of the server socket
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr[0] = 127;
    addr.sin_addr[1] = 0;
    addr.sin_addr[2] = 0;
    addr.sin_addr[3] = 1;
    addr.sin_port = u16_swap(4444);
    print("Bind: ", linux_bind(fd, &addr));
    print("Listen: ", linux_listen(fd, 1));

    sockaddr_in client_addr = {};
    int client_fd = linux_accept(fd, &client_addr, 0);
    print("Accept: ", client_fd);
    print("Port: ", u16_swap(client_addr.sin_port));
    print("Host: ", client_addr.sin_addr[0], ".", client_addr.sin_addr[1], ".", client_addr.sin_addr[2], ".", client_addr.sin_addr[3]);
    char buffer[64];
    sys_read(client_fd, buffer, sizeof(buffer));
    sys_write(client_fd, buffer, sizeof(buffer));
    sys_close(client_fd);
    sys_close(fd);
    os_exit();
}
