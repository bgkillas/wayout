#include<stdbool.h>
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include"wayout.h"

void free_if_set (void *ptr)
{
	if ( ptr != NULL )
		free(ptr);
}

void set_string (char **ptr, char *arg)
{
	free_if_set(*ptr);
	*ptr = strdup(arg);
}

void printlog (struct App *app, int level, const char *fmt, ...)
{
	if ( app != NULL && level > app->verbosity )
		return;

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

bool is_boolean_true (const char *in)
{
	if ( ! strcmp(in, "true") || ! strcmp(in, "yes") || ! strcmp(in, "on") || ! strcmp(in, "1") )
		return true;
	return false;
}

bool is_boolean_false (const char *in)
{
	if ( ! strcmp(in, "false") || ! strcmp(in, "no") || ! strcmp(in, "off") || ! strcmp(in, "0") )
		return true;
	return false;
}

