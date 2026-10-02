#ifndef PROJETO_TIMER_H
#define PROJETO_TIMER_H
#ifdef _WIN32
#include <windows.h>
static double tempoMonotonico(void){
    LARGE_INTEGER count, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)frequency.QuadPart;
}
#else
#include <time.h>
static double tempoMonotonico(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#endif
#endif
