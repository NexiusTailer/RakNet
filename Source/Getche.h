

#if   defined(_WIN32) &&  !defined(X360)
#include <conio.h> /* getche() */

#else
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
char getche();
#endif 
