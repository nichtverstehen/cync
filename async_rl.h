/* Cyril Nikolaev */

#include <stddef.h> /* size_t */
#include "async.h"

typedef struct AMY AMY;

AMY* arl_fromfd (int fd);
int arl_close (AMY *my);

a_Function(arl_readline, { AMY* my; char* buf; size_t len; });

