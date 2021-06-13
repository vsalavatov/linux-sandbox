/*
 * daemonize.c
 * This example daemonizes a process,
 * sleeps 10 seconds and terminates afterwards.
 */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <iostream>

static void skeleton_daemon()
{
    std::cout << "0. Parent pid: " << getpid() << std::endl;
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0) {
        std::cout << "1. Child pid: " << pid << std::endl;
        exit(EXIT_SUCCESS);
    }

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0) {
        std::cout << "2. Child pid: " << pid << std::endl;
        exit(EXIT_SUCCESS);
    }
}

int main()
{
    skeleton_daemon();
    std::cout << "Daemon started." << std::endl;
    sleep (10);
    std::cout << "Daemon terminated." << std::endl;
    return EXIT_SUCCESS;
}
