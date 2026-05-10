#ifdef __unix__
#define LINUX(X) X
#else
#define LINUX(X)
#endif

#ifndef __unix__
#define WINDOWS(X) X
#else
#define WINDOWS(X)
#endif

LINUX(int a = 5);
WINDOWS(int b = 6);

LINUX(int main(int argc, char *argv){});
WINDOWS(int WINMAIN(void){})
