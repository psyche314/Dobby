#include <stdint.h>

#ifdef _WIN32

#include <io.h>
#define open _open
#define read _read
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC



#define ssize_t int

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#else

#include <unistd.h>

#endif
