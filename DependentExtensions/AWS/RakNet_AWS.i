%include "RakNet.i"

%{
/* Includes the header in the wrapper code */
#include "AWSSimpleDBInterface.h"
#include "main.h"
%}


%feature("director") AWSSimpleDBInterface;
%include "AWSSimpleDBInterface.h"
%include "main.h"
