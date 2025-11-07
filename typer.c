/*
 * typer - Client for typed daemon
 * Sends type commands to daemon via Unix socket
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH_LEN 108

void show_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  typer <character>  - Type a single ASCII character\n");
    fprintf(stderr, "  typer backspace    - Press backspace\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        show_usage();
        return 1;
    }

    char socket_path[SOCKET_PATH_LEN];

    // Determine socket path (same logic as daemon)
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0]) {
        snprintf(socket_path, SOCKET_PATH_LEN, "%s/.yell_type_socket", runtime_dir);
    } else {
        snprintf(socket_path, SOCKET_PATH_LEN, "/tmp/.yell_type_socket");
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
        fprintf(stderr, "failed to connect to typed daemon: %s\n", strerror(err));

        switch (err) {
            case ENOENT:
            case ECONNREFUSED:
                fprintf(stderr, "Please check if typed is running.\n");
                fprintf(stderr, "Start it with: ./typed &\n");
                break;
            case EACCES:
            case EPERM:
                fprintf(stderr, "Permission denied. Check socket permissions.\n");
                break;
        }
        close(fd);
        return 2;
    }

    // Prepare command
    char buf[2];

    if (strcmp(argv[1], "backspace") == 0) {
        // Send backspace command
        buf[0] = 'b';
        buf[1] = 0;
    } else if (strlen(argv[1]) == 1) {
        // Send type character command
        buf[0] = 't';
        buf[1] = argv[1][0];
    } else {
        fprintf(stderr, "Error: Argument must be a single character or 'backspace'\n");
        show_usage();
        close(fd);
        return 1;
    }

    // Send command
    if (write(fd, buf, 2) != 2) {
        perror("failed to send command");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
