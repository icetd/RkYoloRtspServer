#ifndef LOG_H
#define LOG_H

#define LOG(level, fmt, ...) \
	log (level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define COL(x)  "\033[;" #x "m"
#define RED     COL(31)
#define GREEN   COL(32)
#define YELLOW  COL(33)
#define BLUE    COL(34)
#define MAGENTA COL(35)
#define CYAN    COL(36)
#define WHITE   COL(0)

typedef enum {
	EMERG = 0,
	FATAL,
	ALERT,
	CRIT,
	ERROR,
	WARN,
	NOTICE,
	INFO,
	DEBUG,
	NOTSET
} PriorityLevel;

extern int LogLevel;

void log (PriorityLevel level, const char* file, const int line, const char *fmt, ...);
void initLogger(PriorityLevel level);

#endif
