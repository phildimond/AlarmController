#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <stdio.h>

void log_error_if_nonzero(const char *message, int error_code);
int getLineInput(char buf[], size_t len);

#endif