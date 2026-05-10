// Copyright (c) 2026 - Tom Smeets <tom@tsmeets.nl>
// proc.h - Running Commands and Processes
#pragma once
#include "error.h"
#include "os_headers.h"

typedef struct Process Process;

// Execute a shell command, returns the exit code
static void proc_shell(char *command) {
    IF_LINUX({ check(system(command) == 0); });
    IF_WINDOWS({ check(system(command) == 0); });
    IF_WASM({});
}

// Execute a process without waiting for it to finish
// - argv is a null terminated list of strings
static Process *proc_exec(char **argv) {
    Process *ret = 0;

    IF_LINUX({
        i32 pid = fork();

        // pid == 0 -> We are the child process
        if (pid == 0) {
            execvp(argv[0], argv);
            linux_exit_group(127);
        }

        // pid == -1 -> failed
        check_or(pid >= 0) return 0;

        // pid > 0 -> parent process, pid is child PID
        ret = fd_to_handle(pid);
    })

    check(ret);
    return ret;
}

// Wait for process to exit and return exit code
static i32 proc_wait(Process *proc) {
    IF_LINUX({
        i32 pid = fd_from_handle(proc);
        i32 status = 0;
        if (waitpid(pid, &status, 0) < 0) return -1;
        u32 sig = status & 0x7f;
        u32 exit_code = (status >> 8) & 0xff;
        if (sig) return -1;
        return exit_code;
    })

    IF_WINDOWS({ return 0; })
    IF_WASM({ return 0; })
}
