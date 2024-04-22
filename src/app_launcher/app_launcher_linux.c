#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "app_launcher.h"

int launch_process(char* application) {
    pid_t pid_child = fork();
    if (pid_child == 0) {
        int numWords = 0;
        const char delim[] = " ";
        char *token = strtok(application, delim);
        while (token != NULL) {
            numWords++; // count words for malloc
            token = strtok(NULL, delim);
        }

        char **argv = (char **)malloc((numWords + 1) * sizeof(char *));
        if (argv == NULL) {
            return -1;
        }

        token = strtok(application, delim);
        int i = 0;
        while (token != NULL) {
            argv[i] = token;
            i++;
            token = strtok(NULL, delim);
        }
        argv[numWords] = NULL;

        return execv(argv[0], argv);
    }
    return pid_child;
}

int terminate_process(int pid) {
    if (pid <= 0) return -1;            // terminate only for child process
    kill(pid, SIGTERM);                 // try gently kill
    sleep(1);                           // time for cleaning
    if (get_process_state(pid) == 0) {  // still running
        return kill(pid, SIGKILL);
    }
    return 0;
}

int get_process_state(int pid) {
    if (pid <= 0) return -1;            // status only for child process
    int status = 0;
    pid_t result = waitpid(pid, &status, WNOHANG);

    if (result > 0 ) {
        if (WIFEXITED(status)) {
            return 1;
        } else if (WIFSIGNALED(status)) {
            return 2;
        } else {
            return -1;                  // WUNTRACED and so WIFSTOPPED are never used in waitpid()
        }
    }
    return result;                      // -1 error or 0 still running
}

void sleep_wrapper(int seconds) {
    sleep(seconds);
}
