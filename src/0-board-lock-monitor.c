// SPDX-License-Identifier: MIT — see LICENSE file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static bool is_process_running(const char *name) {
    DIR *dir = opendir("/proc");
    if (!dir) return false;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        // PID directories are entirely numeric
        long pid = strtol(entry->d_name, NULL, 10);
        if (pid > 0) {
            char cmdpath[256];
            snprintf(cmdpath, sizeof(cmdpath), "/proc/%ld/comm", pid);
            FILE *f = fopen(cmdpath, "r");
            if (f) {
                char comm[256];
                if (fgets(comm, sizeof(comm), f)) {
                    comm[strcspn(comm, "\n")] = '\0';
                    if (strcmp(comm, name) == 0) {
                        fclose(f);
                        closedir(dir);
                        return true;
                    }
                }
                fclose(f);
            }
        }
    }
    closedir(dir);
    return false;
}

static void restart_0board() {
    // Read PID of running instance
    FILE *f = fopen("/tmp/0-board.pid", "r");
    if (f) {
        pid_t pid = 0;
        if (fscanf(f, "%d", &pid) == 1 && pid > 0) {
            kill(pid, SIGTERM);
        }
        fclose(f);
    }
    usleep(200000); // 0.2s
    
    // Launch new instance
    pid_t child = fork();
    if (child == 0) {
        // Detach from parent
        setsid();
        execl("/home/leonardo/.local/bin/0-board", "0-board", NULL);
        exit(1);
    }
}

int main() {
    // Set DISPLAY to ensure X11 apps know where to open
    setenv("DISPLAY", ":0", 1);
    
    bool locked = is_process_running("kdesktop_lock");
    printf("0-board-lock-monitor started. Initial locked state: %d\n", locked);
    
    while (1) {
        bool current = is_process_running("kdesktop_lock");
        if (current != locked) {
            locked = current;
            printf("Lock state transition detected. New state: %d. Restarting 0-board...\n", locked);
            restart_0board();
        }
        sleep(2);
    }
    return 0;
}
