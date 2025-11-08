/*
 * xhisper - Whisper for Linux
 * Combined daemon and client for text input via uinput
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <linux/uinput.h>

#define SOCKET_PATH_LEN 108
#define KEY_LEFTCTRL 29
#define KEY_RIGHTCTRL 97
#define KEY_LEFTALT 56
#define KEY_RIGHTALT 100
#define KEY_LEFTSHIFT 42
#define KEY_RIGHTSHIFT 54
#define KEY_LEFTMETA 125
#define KEY_V 47
#define FLAG_UPPERCASE 0x80000000

// ASCII to Linux keycode mapping
static const int32_t ascii2keycode_map[128] = {
	// 00 - 0f
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,KEY_TAB,KEY_ENTER,-1,-1,-1,-1,-1,
	// 10 - 1f
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	// 20 - 2f
	KEY_SPACE,KEY_1|FLAG_UPPERCASE,KEY_APOSTROPHE|FLAG_UPPERCASE,KEY_3|FLAG_UPPERCASE,KEY_4|FLAG_UPPERCASE,KEY_5|FLAG_UPPERCASE,KEY_7|FLAG_UPPERCASE,KEY_APOSTROPHE,
	KEY_9|FLAG_UPPERCASE,KEY_0|FLAG_UPPERCASE,KEY_8|FLAG_UPPERCASE,KEY_EQUAL|FLAG_UPPERCASE,KEY_COMMA,KEY_MINUS,KEY_DOT,KEY_SLASH,
	// 30 - 3f
	KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,
	KEY_8,KEY_9,KEY_SEMICOLON|FLAG_UPPERCASE,KEY_SEMICOLON,KEY_COMMA|FLAG_UPPERCASE,KEY_EQUAL,KEY_DOT|FLAG_UPPERCASE,KEY_SLASH|FLAG_UPPERCASE,
	// 40 - 4f
	KEY_2|FLAG_UPPERCASE,KEY_A|FLAG_UPPERCASE,KEY_B|FLAG_UPPERCASE,KEY_C|FLAG_UPPERCASE,KEY_D|FLAG_UPPERCASE,KEY_E|FLAG_UPPERCASE,KEY_F|FLAG_UPPERCASE,KEY_G|FLAG_UPPERCASE,
	KEY_H|FLAG_UPPERCASE,KEY_I|FLAG_UPPERCASE,KEY_J|FLAG_UPPERCASE,KEY_K|FLAG_UPPERCASE,KEY_L|FLAG_UPPERCASE,KEY_M|FLAG_UPPERCASE,KEY_N|FLAG_UPPERCASE,KEY_O|FLAG_UPPERCASE,
	// 50 - 5f
	KEY_P|FLAG_UPPERCASE,KEY_Q|FLAG_UPPERCASE,KEY_R|FLAG_UPPERCASE,KEY_S|FLAG_UPPERCASE,KEY_T|FLAG_UPPERCASE,KEY_U|FLAG_UPPERCASE,KEY_V|FLAG_UPPERCASE,KEY_W|FLAG_UPPERCASE,
	KEY_X|FLAG_UPPERCASE,KEY_Y|FLAG_UPPERCASE,KEY_Z|FLAG_UPPERCASE,KEY_LEFTBRACE,KEY_BACKSLASH,KEY_RIGHTBRACE,KEY_6|FLAG_UPPERCASE,KEY_MINUS|FLAG_UPPERCASE,
	// 60 - 6f
	KEY_GRAVE,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,
	KEY_H,KEY_I,KEY_J,KEY_K,KEY_L,KEY_M,KEY_N,KEY_O,
	// 70 - 7f
	KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,
	KEY_X,KEY_Y,KEY_Z,KEY_LEFTBRACE|FLAG_UPPERCASE,KEY_BACKSLASH|FLAG_UPPERCASE,KEY_RIGHTBRACE|FLAG_UPPERCASE,KEY_GRAVE|FLAG_UPPERCASE,-1
};

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

void type_char(unsigned char c) {
    if (c >= 128) return;

    int32_t kdef = ascii2keycode_map[c];
    if (kdef == -1) return;

    uint16_t keycode = kdef & 0xffff;

    if (kdef & FLAG_UPPERCASE) {
        emit(EV_KEY, KEY_LEFTSHIFT, 1);
        emit(EV_SYN, SYN_REPORT, 0);
        usleep(2000);
    }

    emit(EV_KEY, keycode, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(8000);

    emit(EV_KEY, keycode, 0);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(2000);

    if (kdef & FLAG_UPPERCASE) {
        emit(EV_KEY, KEY_LEFTSHIFT, 0);
        emit(EV_SYN, SYN_REPORT, 0);
    }
}

void do_backspace() {
    emit(EV_KEY, KEY_BACKSPACE, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(8000);
    emit(EV_KEY, KEY_BACKSPACE, 0);
    emit(EV_SYN, SYN_REPORT, 0);
}

void do_key(int keycode) {
    emit(EV_KEY, keycode, 1);
    emit(EV_SYN, SYN_REPORT, 0);
    usleep(8000);
    emit(EV_KEY, keycode, 0);
    emit(EV_SYN, SYN_REPORT, 0);
}

int setup_uinput() {
    fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_uinput < 0) {
        perror("failed to open /dev/uinput");
        return -1;
    }

    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);

    // Register letters
    for (int i = KEY_Q; i <= KEY_P; i++) ioctl(fd_uinput, UI_SET_KEYBIT, i);
    for (int i = KEY_A; i <= KEY_L; i++) ioctl(fd_uinput, UI_SET_KEYBIT, i);
    for (int i = KEY_Z; i <= KEY_M; i++) ioctl(fd_uinput, UI_SET_KEYBIT, i);

    // Register numbers
    for (int i = KEY_1; i <= KEY_0; i++) ioctl(fd_uinput, UI_SET_KEYBIT, i);

    // Register special keys
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_SPACE);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_MINUS);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_EQUAL);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_LEFTBRACE);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_RIGHTBRACE);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_SEMICOLON);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_APOSTROPHE);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_GRAVE);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_BACKSLASH);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_COMMA);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_DOT);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_SLASH);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_TAB);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_ENTER);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_BACKSPACE);

    // Register modifiers
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_LEFTCTRL);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_RIGHTCTRL);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_LEFTALT);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_RIGHTALT);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_LEFTSHIFT);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_RIGHTSHIFT);
    ioctl(fd_uinput, UI_SET_KEYBIT, KEY_LEFTMETA);

    struct uinput_setup setup = {0};
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "xhisper");

    if (ioctl(fd_uinput, UI_DEV_SETUP, &setup) < 0) {
        perror("failed to setup uinput device");
        return -1;
    }
    if (ioctl(fd_uinput, UI_DEV_CREATE) < 0) {
        perror("failed to create uinput device");
        return -1;
    }

    usleep(100000);
    return 0;
}

int setup_socket() {
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0]) {
        snprintf(socket_path, SOCKET_PATH_LEN, "%s/.xhisper_socket", runtime_dir);
    } else {
        snprintf(socket_path, SOCKET_PATH_LEN, "/tmp/.xhisper_socket");
    }

    struct stat st;
    if (stat(socket_path, &st) == 0) {
        int test_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        struct sockaddr_un test_addr = {.sun_family = AF_UNIX};
        strncpy(test_addr.sun_path, socket_path, sizeof(test_addr.sun_path) - 1);

        if (connect(test_fd, (struct sockaddr*)&test_addr, sizeof(test_addr)) == 0) {
            close(test_fd);
            fprintf(stderr, "xhispertoold is already running\n");
            return -1;
        }
        close(test_fd);
        unlink(socket_path);
    }

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

    chmod(socket_path, 0600);
    return 0;
}

// Daemon mode
int run_daemon() {
    atexit(cleanup);

    if (setup_uinput() < 0) {
        return 1;
    }

    if (setup_socket() < 0) {
        return 1;
    }

    printf("xhispertoold: listening on %s\n", socket_path);

    char buf[2];
    while (1) {
        ssize_t n = recv(fd_socket, buf, sizeof(buf), 0);
        if (n >= 1) {
            char cmd = buf[0];
            if (cmd == 'p') {
                do_paste();
            } else if (cmd == 't' && n == 2) {
                type_char((unsigned char)buf[1]);
            } else if (cmd == 'b') {
                do_backspace();
            } else if (cmd == 'r') {
                do_key(KEY_RIGHTALT);
            } else if (cmd == 'L') {
                do_key(KEY_LEFTALT);
            } else if (cmd == 'C') {
                do_key(KEY_LEFTCTRL);
            } else if (cmd == 'R') {
                do_key(KEY_RIGHTCTRL);
            } else if (cmd == 'S') {
                do_key(KEY_LEFTSHIFT);
            } else if (cmd == 'T') {
                do_key(KEY_RIGHTSHIFT);
            } else if (cmd == 'M') {
                do_key(KEY_LEFTMETA);
            }
        }
    }

    return 0;
}

// Client mode
void show_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  xhispertool paste            - Paste from clipboard (Ctrl+V)\n");
    fprintf(stderr, "  xhispertool type <char>      - Type a single ASCII character\n");
    fprintf(stderr, "  xhispertool backspace        - Press backspace\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input switching keys:\n");
    fprintf(stderr, "  xhispertool leftalt          - Press left alt\n");
    fprintf(stderr, "  xhispertool rightalt         - Press right alt\n");
    fprintf(stderr, "  xhispertool leftctrl         - Press left ctrl\n");
    fprintf(stderr, "  xhispertool rightctrl        - Press right ctrl\n");
    fprintf(stderr, "  xhispertool leftshift        - Press left shift\n");
    fprintf(stderr, "  xhispertool rightshift       - Press right shift\n");
    fprintf(stderr, "  xhispertool super            - Press super (Windows key)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Daemon:\n");
    fprintf(stderr, "  xhispertoold                 - Run daemon (or xhispertool --daemon)\n");
}

int run_client(int argc, char *argv[]) {
    if (argc < 2) {
        show_usage();
        return 1;
    }

    char socket_path[SOCKET_PATH_LEN];
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && runtime_dir[0]) {
        snprintf(socket_path, SOCKET_PATH_LEN, "%s/.xhisper_socket", runtime_dir);
    } else {
        snprintf(socket_path, SOCKET_PATH_LEN, "/tmp/.xhisper_socket");
    }

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("failed to create socket");
        return 1;
    }

    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        int err = errno;
        fprintf(stderr, "failed to connect to xhispertoold: %s\n", strerror(err));

        switch (err) {
            case ENOENT:
            case ECONNREFUSED:
                fprintf(stderr, "Please check if xhispertoold is running.\n");
                fprintf(stderr, "Start it with: xhispertoold &\n");
                break;
            case EACCES:
            case EPERM:
                fprintf(stderr, "Permission denied. Check socket permissions.\n");
                break;
        }
        close(fd);
        return 2;
    }

    char buf[2];
    ssize_t len = 0;

    if (strcmp(argv[1], "paste") == 0) {
        buf[0] = 'p';
        len = 1;
    } else if (strcmp(argv[1], "backspace") == 0) {
        buf[0] = 'b';
        len = 1;
    } else if (strcmp(argv[1], "rightalt") == 0) {
        buf[0] = 'r';
        len = 1;
    } else if (strcmp(argv[1], "leftalt") == 0) {
        buf[0] = 'L';
        len = 1;
    } else if (strcmp(argv[1], "leftctrl") == 0) {
        buf[0] = 'C';
        len = 1;
    } else if (strcmp(argv[1], "rightctrl") == 0) {
        buf[0] = 'R';
        len = 1;
    } else if (strcmp(argv[1], "leftshift") == 0) {
        buf[0] = 'S';
        len = 1;
    } else if (strcmp(argv[1], "rightshift") == 0) {
        buf[0] = 'T';
        len = 1;
    } else if (strcmp(argv[1], "super") == 0) {
        buf[0] = 'M';
        len = 1;
    } else if (strcmp(argv[1], "type") == 0) {
        if (argc != 3 || strlen(argv[2]) != 1) {
            fprintf(stderr, "Error: 'type' requires exactly one character argument\n");
            show_usage();
            close(fd);
            return 1;
        }
        buf[0] = 't';
        buf[1] = argv[2][0];
        len = 2;
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        show_usage();
        close(fd);
        return 1;
    }

    if (write(fd, buf, len) != len) {
        perror("failed to send command");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    // Detect mode: daemon or client
    char *prog = basename(argv[0]);

    if (strcmp(prog, "xhispertoold") == 0 ||
        (argc > 1 && strcmp(argv[1], "--daemon") == 0)) {
        return run_daemon();
    } else {
        return run_client(argc, argv);
    }
}
