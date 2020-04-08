//Original code from Paddy Byers, https://github.com/paddybyers
//https://github.com/paddybyers/pty.git

#include "openpty.h"
#include <fcntl.h>
#include <stdlib.h>
#include <cstring>
#include <dirent.h>
#include "common.h"

int openpty (int *amaster, int *aslave, char *name, struct termios *termp, struct winsize *winp)
{
  int master, slave;
#ifdef LACKS_PTSNAME_R
    char *name_slave;
#else
    char name_slave[64];
#endif
  char tolog[128];
  //master = open("/dev/ptmx", O_RDWR | O_NONBLOCK);
/*
  char addr[10];
  addr[9]=NULL;
  for(int i=0; i<9; i++)
  {
    switch (i)
    {
        case 0:
        case 4: addr[i]=0x2F; break;
        case 1: addr[i]=0x64; break;
        case 2: addr[i]=addr[1]+1; break;
        case 3: addr[i]=0x76; break;
        case 5: addr[i]=addr[3]-6; break;
        case 6: addr[i]=addr[5]+4; break;
        case 7: addr[i]=0x6D; break;
        case 8: addr[i]=addr[6]+4; break;
    }
  }
*/
  //master = open(addr, O_RDWR | O_CLOEXEC);
  master = open("/dev/ptmx", O_RDWR | O_CLOEXEC);
  if (master < 0)
  {
      LOGI(("Fail to open master"));
	  return -1;
  }

  if (grantpt(master) || unlockpt(master) || ptsname_r(master, name_slave, sizeof(name_slave)))
    goto fail;

 // name_slave = ptsname(master);

    struct termios tios;
    tcgetattr(master, &tios);
    tios.c_iflag |= IUTF8;
    //tios.c_iflag &= ~(IUTF8);
    tios.c_iflag &= ~(IXON | IXOFF);
    tcsetattr(master, TCSANOW, &tios);

    /** Set initial winsize.
    struct winsize sz;// = { .ws_row = 25, .ws_col = 80 };
    sz.ws_col=80;
    sz.ws_row=25;

    ioctl(master, TIOCSWINSZ, &sz);
     */
  //sprintf(tolog, "openpty: slave name %s", name_slave);
  LOGI(("slave opened"));
  slave = open(name_slave, O_RDWR);
  if (slave == -1)
      goto fail;

  if(termp)
    tcsetattr(slave, TCSAFLUSH, termp);
  if (winp)
    ioctl (slave, TIOCSWINSZ, &winp);

  *amaster = master;
  *aslave = slave;
  if (name != NULL)
    strcpy(name, name_slave);

  return 0;

 fail:
  close (master);
  return -1;
}


