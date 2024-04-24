#ifndef PERR
#define PERR
#include <stdio.h>
#define PERROR(s) fprintf(stderr, #s " (%d)\n%s %s:%d\n", errno, \
                          __FILE__, __FUNCTION__ , __LINE__);
#endif // PERR
