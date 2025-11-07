/*
 * pasted - Daemon for yell paste functionality
 * Keeps uinput keyboard device open, listens on Unix socket for paste commands
 * Based on ydotool architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <linux/uinput.h>

#define KEY_LEFTCTRL 29
#define KEY_V 47
#define SOCKET_PATH_LEN 108

static int fd_uinput = -1;
static int fd_socket = -1;
static char socket_path[SOCKET_PATH_LEN] = {0};

void cleanup() {
    if (fd_uinput >= 0) {
        ioctl(fd_uinput, UI_DEV_DESTROY);
        close(fd_uinput);
    }
    if (fd_socket >= 0) {
        close(fd_socket);
    }
    if (socket_path[0]) {
        unlink(socket_path);
    }
}

void emit(int type, int code, int val) {
    struct input_event ie = {
        .type = type,
        .code = code,
        .value = val
    };
    write(fd_uinput, &ie, sizeof(ie));
}

void do_paste() {
    // Ctrl+V
    emit(EV_KEY, KEY_LEFTCTRL, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(8000);
    emit(EV_KEY, KEY_V, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(8000);
    emit(EV_KEY, KEY_V, 0);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(2000);
    emit(EV_KEY, KEY_LEFTCTRL, 0);
    emit(EV_SYN, SYN_REPORT, 0);
}

int setup_uinput() {
    fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_uinput < 0) {
        perror("failed to open /dev/uinput");
        return -1;
    }

    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_LEFTCTRL);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_V);

    struct uinput_setup setup = {0};
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "yell");

    if (ioctl(fd_uinput, UI_DEV_SETUP, &setup) < 0) {
        perror("failed to setup uinput device");
        return -1;
    }
    if (ioctl(fd_uinput, UI_DEV_CREATE) < 0) {
        perror("failed to create uinput device");
        return -1;
    }

    usleep(100000);  // 100ms for device recognition
    return 0;
}

int setup_socket() {
    // Determine socket path (XDG_RUNTIME_DIR or /tmp)
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0]) {
        snprintf(socket_path, SOCKET_PATH_LEN, "%s/.yell_paste_socket", runtime_dir);
    } else {
        snprintf(socket_path, SOCKET_PATH_LEN, "/tmp/.yell_paste_socket");
    }

    // Check for stale socket
    struct stat st;
    if (stat(socket_path, &st) == 0) {
        // Socket exists, check if daemon is alive
        int test_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        struct sockaddr_un test_addr = {.sun_family = AF_UNIX};
        strncpy(test_addr.sun_path, socket_path, sizeof(test_addr.sun_path) - 1);

        if (connect(test_fd, (struct sockaddr*)&test_addr, sizeof(test_addr)) == 0) {
            close(test_fd);
            fprintf(stderr, "pasted is already running\n");
            return -1;
        }
        close(test_fd);

        // Stale socket, remove it
        unlink(socket_path);
    }

    // Create socket
    fd_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd_socket < 0) {
        perror("failed to create socket");
        return -1;
    }

    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(fd_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("failed to bind socket");
        return -1;
    }

    // Set permissions to 0600
    chmod(socket_path, 0600);

    return 0;
}

int main() {
    atexit(cleanup);

    if (setup_uinput() < 0) {
        return 1;
    }

    if (setup_socket() < 0) {
        return 1;
    }

    printf("pasted: listening on %s\n", socket_path);

    // Main loop
    char cmd;
    while (1) {
        ssize_t n = recv(fd_socket, &cmd, 1, 0);
        if (n == 1 && cmd == 'p') {
            do_paste();
        }
    }

    return 0;
}
