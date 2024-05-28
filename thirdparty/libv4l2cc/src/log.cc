#include <log.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void initLogger(PriorityLevel level)
{
	switch (level) {
#define XX(name) \
	case name: \
		printf("Initialize LOG SUCCESS, LEVEL is %s\n" , #name); \
		LogLevel = name; \
		break;

	XX(DEBUG);
	XX(INFO);
	XX(ERROR);
#undef XX
	default:
		LogLevel = NOTICE;
		break;
	}
}

void log (PriorityLevel level, const char* file, const int line, const char *fmt, ...)
{
	char str[1024];
	int len;
	va_list args;
	va_start(args, fmt);
	len = vsprintf(str, fmt, args);
	va_end(args);

	switch (level) {
#define XX(name, color) \
	case name: \
		if (LogLevel >= level) \
			printf(color"[%6s]\033[0m %s\n", #name, str); \
		break;

	XX(EMERG,       RED);
	XX(FATAL,       WHITE);
	XX(ALERT,       WHITE);
	XX(CRIT,        WHITE);
	XX(ERROR,       RED);
	XX(WARN,        YELLOW);
	XX(INFO,        GREEN);
	XX(NOTICE,      CYAN);
	XX(NOTSET,      WHITE);
	
	case DEBUG:
	if(LogLevel >= level)
		printf(BLUE"[ DEBUG]\033[0m %s:%d \n\t%s\n", file, line,str); 
		break;
#undef XX
	default:
		break;
	}
}
