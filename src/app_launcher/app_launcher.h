#ifndef REMOTE_APPLICATION_LAUNCH_APP_LAUNCHER_H
#define REMOTE_APPLICATION_LAUNCH_APP_LAUNCHER_H

/**
 * Launch application in new process.
 * @param command application
 * @param argv arguments for application
 * @return PID of child process
 */
int launch_process(const char* command, char* const* argv);

/**
 * Terminate child process.
 * @return 0 on success, -1 on error
 */
int terminate_process(int pid);

/**
 * Get child process status.
 * @return
 * 0 if child is still running,\n
 * 1 if child is exited normally,\n
 * 2 if child is terminated,\n
 * -1 on error
 */
int get_process_state(int pid);

/**
 * Wrapper function for sleep(s) for Linux and Sleep(ms) for Windows
 * @param seconds time to sleep in seconds
 */
void sleep_wrapper(int seconds);

#endif //REMOTE_APPLICATION_LAUNCH_APP_LAUNCHER_H
