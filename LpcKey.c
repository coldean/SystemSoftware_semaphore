#include "Lpc.h"

key_t __makeKeyByName(const char *pathname) { return ftok(pathname, 0); }
