#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

#define RLIMIT_RTTIME 15

struct limits
{
    char *description;
    char *units;
    int  resource;
    char option;
};
static struct limits limits[] =
{
    { "Max cpu time",           "seconds",    RLIMIT_CPU,         't' },
    { "Max file size",          "bytes",      RLIMIT_FSIZE,       'f' },
    { "Max data size",          "bytes",      RLIMIT_DATA,        'd' },
    { "Max stack size",         "bytes",      RLIMIT_STACK,       's' },
    { "Max core file size",     "bytes",      RLIMIT_CORE,        'c' },
    { "Max resident set",       "bytes",      RLIMIT_RSS,         'm' },
    { "Max processes",          "processes",  RLIMIT_NPROC,       'u' },
    { "Max open files",         "files",      RLIMIT_NOFILE,      'n' },
    { "Max locked memory",      "bytes",      RLIMIT_MEMLOCK,     'l' },
    { "Max address space",      "bytes",      RLIMIT_AS,          'v' },
    { "Max file locks",         "locks",      RLIMIT_LOCKS,       'x' },
    { "Max pending signals",    "signals",    RLIMIT_SIGPENDING,  'i' },
    { "Max msgqueue size",      "bytes",      RLIMIT_MSGQUEUE,    'q' },
    { "Max nice priority",      "priority",   RLIMIT_NICE,        'e' },
    { "Max realtime priority",  "priority",   RLIMIT_RTPRIO,      'r' },
    { "Max realtime timeout",   "useconds",   RLIMIT_RTTIME,      'y' },
    { NULL,                     NULL,         0,                  0   }
};

int usage()
{
    int index = 0;

    fprintf(stderr, "usage: prlimit [pid] [-ah");
    while (limits[index].description)
    {
        fprintf(stderr, "%c", limits[index].option);
        index++;
    }
    fprintf(stderr, "] [limit]\n");
    return -1;
}

int main(int argc, char **argv)
{
    struct rlimit limit;
    char          soft[22], hard[22];
    pid_t         pid;
    int           index = 0;

    if (argc < 2 || !strcmp(argv[1], "-h"))
    {
        return usage();
    }
    if (!(pid = atol(argv[1])) || (!geteuid() && kill(pid, 0)))
    {
        fprintf(stderr, "invalid PID %s\n", argv[1]);
        return -2;
    }
    if (argc < 3)
    {
        return usage();
    }
    if (!strcmp(argv[2], "-a"))
    {
        printf("Limit                     Soft Limit           Hard Limit           Units          Option\n");
        while (limits[index].description)
        {
            if (!prlimit(pid, limits[index].resource, NULL, &limit))
            {
                 if (limit.rlim_cur != RLIM_INFINITY)
                 {
                     sprintf(soft, "%ld", limit.rlim_cur);
                 }
                 else
                 {
                     strcpy(soft, "unlimited");
                 }
                 if (limit.rlim_max != RLIM_INFINITY)
                 {
                     sprintf(hard, "%ld", limit.rlim_max);
                 }
                 else
                 {
                     strcpy(hard, "unlimited");
                 }
            }
            else
            {
                 strcpy(soft, "unknown");
                 strcpy(hard, "unknown");
            }
            printf("%-26.26s%-21.21s%-21.21s%-15.15s-%c\n", limits[index].description, soft, hard, limits[index].units, limits[index].option);
            index++;
        }
        return 0;
    }
    if (argc < 4 || strlen(argv[2]) != 2 || argv[2][0] != '-')
    {
        return usage();
    }
    while (limits[index].description)
    {
        if (argv[2][1] == limits[index].option)
        {
            break;
        }
        index++;
    }
    if (!limits[index].description)
    {
        fprintf(stderr, "unknown option %s\n", argv[2]);
        return usage();
    }
    limit.rlim_cur = limit.rlim_max = atol(argv[3]);
    if (prlimit(pid, limits[index].resource, &limit, NULL))
    {
        perror("rlimit");
        return -3;
    }
    return 0;
}