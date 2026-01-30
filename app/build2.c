#include "fmt.h"

typedef enum {
    Action_Build,
    Action_Format,
    Action_Test,
    Action_Count,
} Action;

static char *action_str[] = {
    "build",
    "format",
    "test",
};

typedef enum {
    Platform_Cross,
    Platform_Linux,
    Platform_Windows,
    Platform_WASM,
    Platform_Count,
} Platform;

static char *platform_str[] = {
    "cross",
    "linux",
    "windows",
    "wasm",
};

typedef enum {
    Mode_Debug,
    Mode_Release,
    Mode_Count,
} Mode;

static char *mode_str[] = {
    "debug",
    "release",
};

typedef struct {
    Action action;
    Platform platform;
    Mode mode;
} Args;

static bool args_parse(Args *args, char *arg) {
    for (Action i = 0; i < Action_Count; ++i) {
        if (str_eq(arg, action_str[i])) {
            args->action = i;
            return 1;
        }
    }

    for (Platform i = 0; i < Platform_Count; ++i) {
        if (str_eq(arg, platform_str[i])) {
            args->platform = i;
            return 1;
        }
    }

    for (Mode i = 0; i < Mode_Count; ++i) {
        if (str_eq(arg, mode_str[i])) {
            args->mode = i;
            return 1;
        }
    }
    return 0;
}

void os_main(u32 argc, char **argv) {
    Args args = {
        .action = Action_Build,
        .platform = Platform_Linux,
        .mode = Mode_Debug,
    };

    bool fail = 0;
    for (u32 i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (args_parse(&args, arg)) continue;
        fmt_ss(fout, " Unkown argument: ", arg, "\n");
        fail = 1;
    }

    if (fail) os_exit(1);

    fmt_s(fout, "Command:\n");
    fmt_ss(fout, "  Action:   ", action_str[args.action], "\n");
    fmt_ss(fout, "  Mode:     ", mode_str[args.mode], "\n");
    fmt_ss(fout, "  Platform: ", platform_str[args.platform], "\n");

    switch (args.action) {
    case Action_Format:
        os_system("clang-format -i --verbose */*.h */*.c");
        break;
    default:
        break;
    }
    os_exit(0);
}
