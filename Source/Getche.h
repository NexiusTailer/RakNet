#if !defined(linux) && !defined(__GNUC__) && !defined(__GCCXML__)
#include <conio.h> /* getche() */

#elif !defined(_PS3) && !defined(__PS3__) && !defined(SN_TARGET_PS3)

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
char getche();

#endif 
