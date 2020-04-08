//Original code from Zhiming Wang https://gist.github.com/zmwangx/
//https://gist.github.com/zmwangx/2bac2af9195cad47069419ccd9ee98d8#file-pty-cc
//https://gist.githubusercontent.com/zmwangx/2bac2af9195cad47069419ccd9ee98d8/raw/582e12b4217a74b1c04d0424ab9042dfdd95c7d6/pty.cc

#include "openpty.h"
#include <string>
#include "common.h"
#include "handlepty.h"

int _stdout;
int _stderr;
int _stdin;

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <jni.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <pwd.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t ttyLock;
    pthread_mutex_t errLock;
    pthread_t ttythread;
    on_ttyout cbOut;
    int ttyFD;
    bool error;
    char *errdesc;
} vbtty_t;

bool ttyInit = false;
vbtty_t vbTTY;
//static int cmdfd;
static pid_t pid;

void die(const char *errstr, ...)
{
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
}

void sigchld(int a)
{
    int stat;
    pid_t p;

    if ((p = waitpid(pid, &stat, WNOHANG)) < 0)
        die("waiting for pid %hd failed: %s\n", pid, strerror(errno));

    if (pid != p)
        return;

    if (WIFEXITED(stat) && WEXITSTATUS(stat))
        die("child exited with status %d\n", WEXITSTATUS(stat));
    else if (WIFSIGNALED(stat))
        die("child terminated due to signal %d\n", WTERMSIG(stat));
    exit(0);
}

void execsh(char *cmd, char **args)
{
    char *sh, *prog;
    const struct passwd *pw;

    errno = 0;
    if ((pw = getpwuid(getuid())) == NULL) {
        if (errno)
            die("getpwuid: %s\n", strerror(errno));
        else
            die("who are you?\n");
    }

    if ((sh = getenv("SHELL")) == NULL)
        sh = (pw->pw_shell[0]) ? pw->pw_shell : cmd;

    if (args)
        prog = args[0];
    else
        {
        prog = sh;
        args = ((char *[]) {prog, NULL});
    }

    unsetenv("COLUMNS");
    unsetenv("LINES");
    unsetenv("TERMCAP");
    setenv("LOGNAME", pw->pw_name, 1);
    setenv("USER", pw->pw_name, 1);
    setenv("SHELL", sh, 1);
    setenv("HOME", pw->pw_dir, 1);

    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGALRM, SIG_DFL);

    execvp(prog, args);
    _exit(1);
}

int ttynew( char *cmd, char *out, char **args)
{
    int m, s;
    int iofd = 1;
    if (out)
    {
        iofd = (!strcmp(out, "-")) ? 1 : open(out, O_WRONLY | O_CREAT, 0666);
        if (iofd < 0) {
            fprintf(stderr, "Error opening %s:%s\n", out, strerror(errno));
            return -1;
        }
    }

    if (openpty(&m, &s, NULL, NULL, NULL) < 0)
        return -1;

    switch (pid = fork()) {
        case -1:
            die("fork failed: %s\n", strerror(errno));
            break;
        case 0:
            close(iofd);
            setsid(); /* create a new process group */
            dup2(s, 0);
            dup2(s, 1);
            dup2(s, 2);
            if (ioctl(s, TIOCSCTTY, NULL) < 0)
                die("ioctl TIOCSCTTY failed: %s\n", strerror(errno));
            close(s);
            close(m);
            execsh(cmd, args);
            break;
        default:
            close(s);
            signal(SIGCHLD, sigchld);
            break;
    }
    return m;
}

void ttywriteraw(int ttyfd, const char *s, size_t n)
{
    fd_set wfd, rfd;
    ssize_t r;
    size_t lim = 256;

    while (n > 0)
    {
        FD_ZERO(&wfd);
        //FD_ZERO(&rfd);
        FD_SET(ttyfd, &wfd);
        //FD_SET(cmdfd, &rfd);

        if (pselect(ttyfd+1, NULL, &wfd, NULL, NULL, NULL) < 0) // Check if we can write.
        //if (pselect(ttyfd+1, &rfd, &wfd, NULL, NULL, NULL) < 0) // Check if we can write.
        {
            if (errno == EINTR)
                continue;
            die("select failed: %s\n", strerror(errno));
        }
        if ((r = write(ttyfd, s, (n < lim)? n : lim)) < 0)
            die("write error on tty: %s\n", strerror(errno));
        if (r >= n)
            break; //All bytes have been written.
    }
}

void ttywrite(int ttyfd, const char *s, size_t n)
{
    const char *next;

    while (n > 0) {
        if (*s == '\r') {
            next = s + 1;
            //ttywriteraw(ttyfd, "\r\n", 2);
            ttywriteraw(ttyfd, "\r", 1);
        } else {
            next = (const char *)memchr((void *)s, '\r', n);
            if(!next) next = s + n;
            ttywriteraw(ttyfd, s, next - s);
        }
        n -= next - s;
        s = next;
    }
}

static void ttyThread(vbtty_t *ttyS)
{
    fd_set rfds;
    struct timeval tv;// { 0, 0 };
    ssize_t readed = 0;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    const int bufsize = 512;
    char buf[512];
    memset(buf, 0, bufsize);
    while(true) //keep reading from ttyfd
    {
        pthread_mutex_lock(&ttyS->ttyLock);
        {
            FD_ZERO(&rfds);
            FD_SET(ttyS->ttyFD, &rfds);
            if (select(ttyS->ttyFD + 1, &rfds, NULL, NULL, &tv))
            {
                readed = read(ttyS->ttyFD, buf, bufsize-1);
                if(readed>0)
                {
                    buf[bufsize-1] = '\0';
                    ttyS->cbOut(buf);
                    memset(buf, 0, bufsize);
                }
            }
            //else sprintf(buf, "terminal not ready\n");
        }
        pthread_mutex_unlock(&ttyS->ttyLock);

        if(ttyS->ttythread==0)
            break;

    }
    pthread_exit(0);
}


int startTTY(on_ttyout cb)
{
    if(!ttyInit) {
        vbTTY.ttyFD = ttynew("/system/bin/sh", "/dev/ptmx",NULL);
        dup2(_stdout, STDOUT_FILENO);
        dup2(_stderr, STDERR_FILENO);
        dup2(_stdin, STDIN_FILENO);
        ttyInit = true;
    }

    vbTTY.cbOut=(on_ttyout)cb;
    if (pthread_mutex_init(&vbTTY.ttyLock, NULL) != 0) {
        printf("ttymutex init fail: %s\n", strerror(errno));
        return -1;
    }
    if (pthread_mutex_init(&vbTTY.errLock, NULL) != 0) {
        printf("errmutex init fail: %s\n", strerror(errno));
        return -1;
    }
    pthread_create(&vbTTY.ttythread, NULL, (void *(*)(void *))ttyThread, &vbTTY);
    return 0;
}

int ttySend(const char *cmd)
{
    char *command = (char *)malloc(strlen(cmd)+2);
    snprintf(command, strlen(cmd)+2, "%s\r", cmd);

    pthread_mutex_lock(&vbTTY.ttyLock);
    ttywrite(vbTTY.ttyFD, command, strlen(command));
    fsync(STDOUT_FILENO);
    fsync(STDERR_FILENO);
    fsync(STDIN_FILENO);
    pthread_mutex_unlock(&vbTTY.ttyLock);
    return 0;
}

/*
int execCommand(const char *cmd, char *buf, uint16_t bufsize)
{
    int master=0;
    if(!master) {
        ttynew("/system/bin/sh", "/dev/ptmx",NULL);
        dup2(_stdout, STDOUT_FILENO);
        dup2(_stderr, STDERR_FILENO);
        dup2(_stdin, STDIN_FILENO);
    }
    else
        vbTTY.ttyFD = master;

    char *command = (char *)malloc(strlen(cmd)+1);
    strcpy(command, cmd);

    strcat(command, "\r");
    fd_set rfds;
    struct timeval tv;// { 0, 0 };
    ssize_t size = 0;
    size_t count = 0;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    memset(buf, 0, bufsize);
    //const char *ripe = "casd /system\r";
    ttywrite(command, strlen(command));

    // Child process terminated; we flush the output and restore stdout.
    fsync(STDOUT_FILENO);
    fsync(STDERR_FILENO);
    fsync(STDIN_FILENO);

    FD_ZERO(&rfds);
    FD_SET(master, &rfds);
    if (select(master + 1, &rfds, NULL, NULL, &tv))
    {
        size = read(master, buf, bufsize);
        buf[size] = '\0';
    } else
        sprintf(buf, "terminal not ready\n");

    LOGI("ret: %s", buf);

    return count;
}
*/