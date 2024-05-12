#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ADDR_LEN 16

struct addrinfo{
    uint32_t addr_min;
    uint32_t addr_max;
};

__uint32_t ipstr2int(const char* ipstr)
{
    struct in_addr addr;
    if(inet_aton(ipstr, &addr))
        return addr.s_addr;
    return 0;
}

char* int2ipstr(__uint32_t ipint, char* ipstr, int len)
{
    struct in_addr addr;
    addr.s_addr = ipint;
    const char* addrptr=inet_ntoa(addr);
    if(addrptr == NULL)
        return NULL;
    strncpy(ipstr, addrptr, len);
}

bool chk_mask_valid(char* maskstr)
{
    uint32_t mask = ntohl(ipstr2int(maskstr));
    if(mask == 0)
        return false;
    int i, v;
    bool flag = false;
    for(i = 31; i >= 0; i--) {
        v = (mask >> i) & 0x01;
        if((flag && v != 0))
            return false;
        if(!flag && v == 0)
            flag = true;
    }
    return true;
}

struct addrinfo* calc_iprange(char* addrstr, char* maskstr, struct addrinfo* info)
{
    uint32_t int_addr = ipstr2int(addrstr);
    uint32_t int_mask = ipstr2int(maskstr);
    if(int_addr == 0 || int_mask == 0)
        return NULL;
    int_addr = ntohl(int_addr);
    int_mask = ntohl(int_mask);
    info->addr_min = int_addr & int_mask;
    info->addr_max = int_addr | (~int_mask);
    return info;
}

uint32_t calc_nextpool(char* addrstr, char* maskstr)
{
    uint32_t capacity = ~ntohl(ipstr2int(maskstr)) + 1;
    uint32_t addr = ntohl(ipstr2int(addrstr));
    addr = htonl(addr + capacity);
    int2ipstr(addr, addrstr, ADDR_LEN);
    return capacity;
}

int main(int argc, char** argv)
{
    bool recalc_flag = false;
    char addrstr1[ADDR_LEN], maskstr1[ADDR_LEN], addrstr2[ADDR_LEN], maskstr2[ADDR_LEN], pooladdr[ADDR_LEN];
    memset(addrstr1, 0x0, ADDR_LEN);
    memset(maskstr1, 0x0, ADDR_LEN);
    memset(addrstr2, 0x0, ADDR_LEN);
    memset(maskstr2, 0x0, ADDR_LEN);
    memset(pooladdr, 0x0, ADDR_LEN);
    if(argc != 6) {
        fprintf(stderr, "./calciprange lanip lanmask wanip wanmask pooladdr\n");
        return 2;
    }
    strncpy(addrstr1, argv[1], ADDR_LEN);
    strncpy(maskstr1, argv[2], ADDR_LEN);
    strncpy(addrstr2, argv[3], ADDR_LEN);
    strncpy(maskstr2, argv[4], ADDR_LEN);
    strncpy(pooladdr, argv[5], ADDR_LEN);
    if(!chk_mask_valid(maskstr1) || !chk_mask_valid(maskstr2))
        return 3;
    fprintf(stderr,"lanip:%s/%s;wanip:%s/%s\n", addrstr1, maskstr1, addrstr2, maskstr2);
    struct addrinfo addrinfo1, addrinfo2;
    if(calc_iprange(addrstr2, maskstr2, &addrinfo2) == NULL)
        return 4;
    fprintf(stderr, "addrinfo2, min:%u, max:%u\n", addrinfo2.addr_min, addrinfo2.addr_max);
    uint32_t offset_count = 0;
    do {
        if(calc_iprange(addrstr1, maskstr1, &addrinfo1) == NULL)
            return 5;
        fprintf(stderr, "addrinfo1, min:%u, max:%u\n", addrinfo1.addr_min, addrinfo1.addr_max);
        if((addrinfo1.addr_min >= addrinfo2.addr_min && addrinfo1.addr_min <= addrinfo2.addr_max) ||
           (addrinfo2.addr_min >= addrinfo1.addr_min && addrinfo2.addr_min <= addrinfo1.addr_max)
        ) {
            fprintf(stderr, "两个IP段有重叠!\n");
            recalc_flag = true;
            offset_count += calc_nextpool(addrstr1, maskstr1);
            fprintf(stderr, "下一个地址段:%s %s\n", addrstr1, maskstr1);
        } else {
            fprintf(stderr, "两个IP段未重叠!\n");
            break;
        }
    } while(true);
    int2ipstr(htonl(ntohl(ipstr2int(pooladdr))+offset_count), pooladdr, ADDR_LEN);
    fprintf(stdout, "%s:%s:%s\n", addrstr1, maskstr1, pooladdr);
    return recalc_flag;
}
