#ifndef WLCLOCK_MISC_H
#define WLCLOCK_MISC_H

#include<stdbool.h>

struct App;

void free_if_set (void *ptr);
void set_string (char **ptr, char *arg);
void printlog (struct App *app, int level, const char *fmt, ...);
bool is_boolean_true (const char *in);
bool is_boolean_false (const char *in);

#endif
