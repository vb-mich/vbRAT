//Original code from Zhiming Wang https://gist.github.com/zmwangx/
//https://gist.github.com/zmwangx/2bac2af9195cad47069419ccd9ee98d8#file-pty-cc
//https://gist.githubusercontent.com/zmwangx/2bac2af9195cad47069419ccd9ee98d8/raw/582e12b4217a74b1c04d0424ab9042dfdd95c7d6/pty.cc

#include "openpty.h"
#include <string>
#include "common.h"
#include "vbTTY.h"

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
    int error;
    char errdesc[256];
    bool ttyLive;
} vbtty_t;

bool ttyInit = false;
vbtty_t vbTTY;
static pid_t pidp;

void setError(const char *errstr, ...)
{
    pthread_mutex_lock(&vbTTY.errLock);
    {
        va_list ap;
        va_start(ap, errstr);
        vsnprintf(vbTTY.errdesc, 256, errstr, ap);
        va_end(ap);
    }
    pthread_mutex_unlock(&vbTTY.errLock);
}

const char* vbTTY_getError()
{
    const char *ret;
    pthread_mutex_lock(&vbTTY.errLock);
    {
        if (vbTTY.error == 0)
            ret = NULL;
        else
            ret = vbTTY.errdesc;
    }
    pthread_mutex_unlock(&vbTTY.errLock);
    return ret;
}

void sigchld(int a)
{
    int stat;
    pid_t p;

    if ((p = waitpid(pidp, &stat, WNOHANG)) < 0)
        setError("waiting for pid %hd failed: %s\n", pidp, strerror(errno));

    if (pidp != p)
        return;

    if (WIFEXITED(stat) && WEXITSTATUS(stat))
        setError("child exited with status %d\n", WEXITSTATUS(stat));
    else if (WIFSIGNALED(stat))
        setError("child terminated due to signal %d\n", WTERMSIG(stat));
}

int execsh(char *cmd, char **args)
{
    char *sh, *prog;
    const struct passwd *pw;

    errno = 0;
    if ((pw = getpwuid(getuid())) == NULL) {
        if (errno)
            setError("getpwuid: %s\n", strerror(errno));
        else
            setError("who are you?\n");
        _exit(1);
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

    _exit(0);
}

int ttynew( char *cmd, char *out, char **args)
{
    int m, s;
    int iofd = 1;
    if (out)
    {
        iofd = (!strcmp(out, "-")) ? 1 : open(out, O_WRONLY | O_CREAT, 0666);
        if (iofd < 0) {
            setError("Error opening %s:%s\n", out, strerror(errno));
            return -1;
        }
    }

    if (openpty(&m, &s, NULL, NULL, NULL) < 0)
        return -1;

    switch (pidp = fork()) {
        case -1:
            setError("fork failed: %s\n", strerror(errno));
            return -1;
            break;
        case 0:
            close(iofd);
            setsid(); //create a new process group
            dup2(s, STDIN_FILENO);
            dup2(s, STDOUT_FILENO);
            dup2(s, STDERR_FILENO);
            if (ioctl(s, TIOCSCTTY, NULL) < 0)
                setError("ioctl TIOCSCTTY failed: %s\n", strerror(errno));
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
    fd_set wfd;
    ssize_t r;
    size_t lim = 256;

    while (n > 0)
    {
        FD_ZERO(&wfd);
        FD_SET(ttyfd, &wfd);

        if (pselect(ttyfd+1, NULL, &wfd, NULL, NULL, NULL) < 0) // Check if we can write.
        {
            if (errno == EINTR)
                continue;
            setError("select failed: %s\n", strerror(errno));
        }
        if ((r = write(ttyfd, s, (n < lim)? n : lim)) < 0)
            setError("write error on tty: %s\n", strerror(errno));
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
    bool shutdown=false;
    memset(buf, 0, bufsize);
    while(true) //keep reading from ttyfd
    {
        pthread_mutex_lock(&ttyS->ttyLock);
        {
            if(ttyS->ttyLive)
            {
                FD_ZERO(&rfds);
                FD_SET(ttyS->ttyFD, &rfds);
                if (select(ttyS->ttyFD + 1, &rfds, NULL, NULL, &tv))
                {
                    readed = read(ttyS->ttyFD, buf, bufsize - 1);
                    if (readed > 0)
                    {
                        buf[bufsize - 1] = '\0';
                        ttyS->cbOut(buf);
                        memset(buf, 0, bufsize);
                    }
                }
                //else sprintf(buf, "terminal not ready\n");
            }
            else
                shutdown = true;
        }
        pthread_mutex_unlock(&ttyS->ttyLock);

        if(shutdown)
            break;
    }
    pthread_exit(0);
}

bool vbTTY_isOpen()
{
    bool ret;
    pthread_mutex_lock(&vbTTY.ttyLock);
    {
        ret = vbTTY.ttyLive;
    }
    pthread_mutex_unlock(&vbTTY.ttyLock);
    return ret;
}

int vbTTY_stopShell()
{
    pthread_mutex_lock(&vbTTY.ttyLock);
    {
        vbTTY.ttyLive = false;
        close(vbTTY.ttyFD);
    }
    pthread_mutex_unlock(&vbTTY.ttyLock);
    kill(pidp, SIGKILL);
    pthread_join(vbTTY.ttythread, NULL);
    return 0;
};

int vbTTY_startShell(on_ttyout cb)
{
    if(!ttyInit)
    {
        vbTTY.ttyLive = 0;
        ttyInit = true;
    }
    if(!vbTTY.ttyLive)
    {
        vbTTY.cbOut = (on_ttyout) cb;
        vbTTY.ttyFD = 0;
        vbTTY.ttyFD = ttynew("/system/bin/sh", "/dev/ptmx", NULL);

        if(vbTTY.ttyFD < 0)
            return 1;

        if (pthread_mutex_init(&vbTTY.ttyLock, NULL) != 0)
        {
            printf("ttymutex init fail: %s\n", strerror(errno));
            return 1;
        }
        if (pthread_mutex_init(&vbTTY.errLock, NULL) != 0)
        {
            printf("errmutex init fail: %s\n", strerror(errno));
            return 1;
        }
        pthread_create(&vbTTY.ttythread, NULL, (void *(*)(void *)) ttyThread, &vbTTY);
        vbTTY.ttyLive = 1;
    }
    return 0;
}

int vbTTY_send(const char *cmd)
{
    char *command = (char *)malloc(strlen(cmd)+2);
    snprintf(command, strlen(cmd)+2, "%s\r", cmd);

    pthread_mutex_lock(&vbTTY.ttyLock);
    {
        ttywrite(vbTTY.ttyFD, command, strlen(command));
        fsync(STDOUT_FILENO);
        fsync(STDERR_FILENO);
        fsync(STDIN_FILENO);
    }
    pthread_mutex_unlock(&vbTTY.ttyLock);
    return 0;
}
