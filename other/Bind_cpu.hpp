#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int BindCpu(int CpuID)
{
    int cpus = 0;
    int  i = 0;
    cpu_set_t mask;
    cpu_set_t get;

    cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("cpus: %d\n", cpus);

    CPU_ZERO(&mask);    /* 初始化set集，将set置为空*/
    /*将本进程绑定到CPU2上*/
    CPU_SET(CpuID, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        printf("Set CPU affinity failue, ERROR:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}
