

#if !defined(linux) && !defined(__GNUC__) && !defined(__GCCXML__)


#elif !defined(_PS3) && !defined(__PS3__) && !defined(SN_TARGET_PS3)

#include "Getche.h"

char getche()
{


  struct termios oldt,
                 newt;
  char            ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;

} 
#endif 