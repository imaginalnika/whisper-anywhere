/*
 * paste - Client for pasted daemon
 * Sends paste command to daemon via Unix socket
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH_LEN 108

int main() {
    char socket_path[SOCKET_PATH_LEN];

    // Determine socket path (same logic as daemon)
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0]) {
        snprintf(socket_path, SOCKET_PATH_LEN, "%s/.yell_paste_socket", runtime_dir);
    } else {
        snprintf(socket_path, SOCKET_PATH_LEN, "/tmp/.yell_paste_socket");
    }

    // Create socket
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("failed to create socket");
        return 1;
    }

    // Connect to daemon
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        int err = errno;
        fprintf(stderr, "failed to connect to pasted daemon: %s\n", strerror(err));

        switch (err) {
            case ENOENT:
            case ECONNREFUSED:
                fprintf(stderr, "Please check if pasted is running.\n");
                fprintf(stderr, "Start it with: ./pasted &\n");
                break;
            case EACCES:
            case EPERM:
                fprintf(stderr, "Permission denied. Check socket permissions.\n");
                break;
        }
        close(fd);
        return 2;
    }

    // Send paste command
    char cmd = 'p';
    if (write(fd, &cmd, 1) != 1) {
        perror("failed to send paste command");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
