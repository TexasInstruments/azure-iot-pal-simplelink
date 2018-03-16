/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== ndk_tirtos.c ========
 */

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/inc/os/oskern.h>

#include <signal.h>
#include <time.h>
#include <stdio.h>

/* Socket file descriptor table */
#define MAXSOCKETS 10
uint32_t ti_ndk_socket_max_fd = MAXSOCKETS;
void *ti_ndk_socket_fdtable[MAXSOCKETS];

/* NDK memory manager page size and number of pages [used by mmAlloc()] */
#define RAW_PAGE_SIZE 3072
#define RAW_PAGE_COUNT 6

const int ti_ndk_config_Global_rawPageSize  = RAW_PAGE_SIZE;
const int ti_ndk_config_Global_rawPageCount = RAW_PAGE_COUNT;

/* P.I.T. (page information table) */
#ifdef __ti__
#pragma DATA_SECTION(ti_ndk_config_Global_pit, ".bss:NDK_MMBUFFER");
#pragma DATA_SECTION(ti_ndk_config_Global_pitBuffer, ".bss:NDK_MMBUFFER");
PITENTRY ti_ndk_config_Global_pit[RAW_PAGE_COUNT];
unsigned char ti_ndk_config_Global_pitBuffer[RAW_PAGE_SIZE * RAW_PAGE_COUNT];
#elif defined (__IAR_SYSTEMS_ICC__)
PITENTRY ti_ndk_config_Global_pit[RAW_PAGE_COUNT];
unsigned char ti_ndk_config_Global_pitBuffer[RAW_PAGE_SIZE * RAW_PAGE_COUNT];
#else
PITENTRY ti_ndk_config_Global_pit[RAW_PAGE_COUNT]
        __attribute__ ((section(".bss:NDK_MMBUFFER")));
unsigned char ti_ndk_config_Global_pitBuffer[RAW_PAGE_SIZE * RAW_PAGE_COUNT]
        __attribute__ ((section(".bss:NDK_MMBUFFER")));
#endif

/* Memory bucket sizes */
#define SMALLEST 48
#define LARGEST (RAW_PAGE_SIZE)

const int ti_ndk_config_Global_smallest = SMALLEST;
const int ti_ndk_config_Global_largest  = LARGEST;

/* Memory Slot Tracking */
uint32_t ti_ndk_config_Global_Id2Size[] =
        {SMALLEST, 96, 128, 256, 512, 1536, LARGEST};

/*
 *  Local Packet Buffer Pool Definitions
 *
 *  The below variables/defines are used to override the defaults that are set
 *  in the Packet Buffer Manager (PBM) file src/stack/pbm/pbm_data.c
 */

/*
 *  Number of buffers in PBM packet buffer free pool
 *
 *  The number of buffers in the free pool can have a significant effect
 *  on performance, especially in UDP packet loss. Increasing this number
 *  will increase the size of the static packet pool use for both sending
 *  and receiving packets.
 */
#define PKT_NUM_FRAMEBUF 10

/* Size of Ethernet frame buffer */
#define PKT_SIZE_FRAMEBUF   1536

const int ti_ndk_config_Global_numFrameBuf = PKT_NUM_FRAMEBUF;
const int ti_ndk_config_Global_sizeFrameBuf = PKT_SIZE_FRAMEBUF;

/* Data space for packet buffers */
#ifdef __ti__
#pragma DATA_ALIGN(ti_ndk_config_Global_pBufMem, 128);
#pragma DATA_SECTION(ti_ndk_config_Global_pBufMem, ".bss:NDK_PACKETMEM");
unsigned char
        ti_ndk_config_Global_pBufMem[PKT_NUM_FRAMEBUF * PKT_SIZE_FRAMEBUF];
#elif defined (__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 128
unsigned char
        ti_ndk_config_Global_pBufMem[PKT_NUM_FRAMEBUF * PKT_SIZE_FRAMEBUF];
#else
unsigned char ti_ndk_config_Global_pBufMem[PKT_NUM_FRAMEBUF * PKT_SIZE_FRAMEBUF]
        __attribute__ ((aligned(128), section(".bss:NDK_PACKETMEM")));
#endif

#ifdef __ti__
#pragma DATA_ALIGN(ti_ndk_config_Global_pHdrMem, 128);
#pragma DATA_SECTION(ti_ndk_config_Global_pHdrMem, ".bss:NDK_PACKETMEM");
unsigned char ti_ndk_config_Global_pHdrMem[PKT_NUM_FRAMEBUF * sizeof(PBM_Pkt)];
#elif defined (__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 128
unsigned char ti_ndk_config_Global_pHdrMem[PKT_NUM_FRAMEBUF * sizeof(PBM_Pkt)];
#else
unsigned char ti_ndk_config_Global_pHdrMem[PKT_NUM_FRAMEBUF * sizeof(PBM_Pkt)]
        __attribute__ ((aligned(128), section(".bss:NDK_PACKETMEM")));
#endif

/* Our NETCTRL callback functions */
static void networkOpen();
static void networkClose();
static void networkIPAddr(uint32_t IPAddr, uint32_t IfIdx, uint32_t fAdd);
static char *hostName = "tisoc";

extern void llTimerTick();

/*
 *  ======== networkOpen ========
 *  This function is called after the configuration has booted
 */
static void networkOpen()
{
    extern void netOpenHook();

    /* call user defined network open hook */
    netOpenHook();
}

/*
 *  ======== networkClose ========
 *  This function is called when the network is shutting down,
 *  or when it no longer has any IP addresses assigned to it.
 */
static void networkClose()
{
    /* call user defined network close hook */
}

/*
 *  ======== networkIPAddr ========
 *  This function is called whenever an IP address binding is
 *  added or removed from the system.
 */
static void networkIPAddr(uint32_t IPAddr, uint32_t IfIdx, uint32_t fAdd)
{
    uint32_t IPTmp;

    if (fAdd) {
        printf("Network Added: ");
    }
    else {
        printf("Network Removed: ");
    }

    /* print the IP address that was added/removed */
    IPTmp = NDK_ntohl(IPAddr);
    printf("If-%ld:%d.%d.%d.%d\n", IfIdx,
            (uint8_t)(IPTmp>>24)&0xFF, (uint8_t)(IPTmp>>16)&0xFF,
            (uint8_t)(IPTmp>>8)&0xFF, (uint8_t)IPTmp&0xFF);

    extern void netIPAddrHook();

    /* call user defined network IP address hook */
    netIPAddrHook(IPAddr, IfIdx, fAdd);
}

/*
 *  ======== serviceReport ========
 *  Function for reporting service status updates.
 */
static char *taskName[] = {"Telnet", "HTTP", "NAT", "DHCPS", "DHCPC", "DNS"};
static char *reportStr[] = {"", "Running", "Updated", "Complete", "Fault"};
static char *statusStr[] =
        {"Disabled", "Waiting", "IPTerm", "Failed","Enabled"};
static void serviceReport(uint32_t item, uint32_t status, uint32_t report,
        void *h)
{
    printf("Service Status: %-9s: %-9s: %-9s: %03ld\n",
            taskName[item - 1], statusStr[status], reportStr[report / 256],
            report & 0xFF);
}

/*
 *  ======== initTcp ========
 *  Configure the stack's TCP settings
 */
static void initTcp(void *hCfg)
{
    int transmitBufSize = 1024;
    int receiveBufSize = 1024;
    int receiveBufLimit = 2048;

    CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPTXBUF, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&transmitBufSize, NULL);
    CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPRXBUF, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&receiveBufSize, NULL);
    CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKTCPRXLIMIT, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&receiveBufLimit, NULL);
}

/*
 *  ======== initIp ========
 *  Configure the stack's IP settings
 */
static void initIp(void *hCfg)
{
    CI_SERVICE_DHCPC dhcpc;
    unsigned char DHCP_OPTIONS[] = { DHCPOPT_SUBNET_MASK };

    /* Add global hostname to hCfg (to be claimed in all connected domains) */
    CfgAddEntry(hCfg, CFGTAG_SYSINFO, CFGITEM_DHCP_HOSTNAME, 0,
            strlen(hostName), (unsigned char *)hostName, NULL);

    /* Use DHCP to obtain IP address on interface 1 */
    memset(&dhcpc, 0, sizeof(dhcpc));
    dhcpc.cisargs.Mode   = CIS_FLG_IFIDXVALID;
    dhcpc.cisargs.IfIdx  = 1;
    dhcpc.cisargs.pCbSrv = &serviceReport;
    dhcpc.param.pOptions = DHCP_OPTIONS;
    dhcpc.param.len = 1;
    CfgAddEntry(hCfg, CFGTAG_SERVICE, CFGITEM_SERVICE_DHCPCLIENT, 0,
            sizeof(dhcpc), (unsigned char *)&dhcpc, NULL);
}

/*
 *  ======== initUdp ========
 *  Configure the stack's UDP settings
 */
void initUdp(void *hCfg)
{
    int receiveBufSize = 2048;

    CfgAddEntry(hCfg, CFGTAG_IP, CFGITEM_IP_SOCKUDPRXLIMIT, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&receiveBufSize, NULL);
}

/*
 *  ======== ndkStackThread ========
 *  NDK stack's main thread function
 */
static void ndkStackThread(uintptr_t arg0, uintptr_t arg1)
{
    void *hCfg;
    int rc;
    timer_t ndkHeartBeat;
    struct sigevent sev;
    struct itimerspec its;
    struct itimerspec oldIts;
    int ndkHeartBeatCount = 0;

    /* create the NDK timer tick */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_value.sival_ptr = &ndkHeartBeatCount;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_notify_function = &llTimerTick;

    rc = timer_create(CLOCK_MONOTONIC, &sev, &ndkHeartBeat);
    if (rc != 0) {
        printf("ndkStackThread: failed to create timer (%d)\n", rc);
    }

    /* start the NDK 100ms timer */
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 100000000;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 100000000;

    rc = timer_settime(ndkHeartBeat, 0, &its, NULL);
    if (rc != 0) {
        printf("ndkStackThread: failed to set time (%d)\n", rc);
    }

    rc = NC_SystemOpen(NC_PRIORITY_LOW, NC_OPMODE_INTERRUPT);
    if (rc) {
        printf("ndkStackThread: NC_SystemOpen Failed (%d)\n", rc);
    }

    /* create and build the system configuration from scratch. */
    hCfg = CfgNew();
    if (!hCfg) {
        printf("ndkStackThread: Unable to create configuration\n");
        goto main_exit;
    }

    /* IP, TCP, and UDP config */
    initIp(hCfg);
    initTcp(hCfg);
    initUdp(hCfg);

    /* config low priority tasks stack size */
    rc = 2048;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKSTKLOW, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&rc, NULL);

    /* config norm priority tasks stack size */
    rc = 2048;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKSTKNORM, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&rc, NULL);

    /* config high priority tasks stack size */
    rc = 2048;
    CfgAddEntry(hCfg, CFGTAG_OS, CFGITEM_OS_TASKSTKHIGH, CFG_ADDMODE_UNIQUE,
            sizeof(uint32_t), (unsigned char *)&rc, NULL);

    do
    {
        rc = NC_NetStart(hCfg, networkOpen, networkClose, networkIPAddr);
    } while(rc > 0);

    /* Shut down the stack */
    CfgFree(hCfg);

main_exit:
    NC_SystemClose();

    /* stop and delete the NDK heartbeat */
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;

    rc = timer_settime(ndkHeartBeat, 0, &its, &oldIts);

    rc = timer_delete(ndkHeartBeat);

    printf("ndkStackThread: exiting ...\n");
}

/*
 * ======== ti_ndk_config_Global_startupFxn ========
 * Called to start up the NDK. In BIOS, this can be called as a BIOS startup
 * function, or from main(). In FreeRTOS, this should be called from main().
 */
void ti_ndk_config_Global_startupFxn()
{
    Task_Params params;
    Task_Handle ndkThread;

    Task_Params_init(&params);
    params.instance->name = "ndkStackThread";
    params.priority = 5;
    params.stackSize = 2048;

    ndkThread = Task_create((Task_FuncPtr)ndkStackThread, &params, NULL);

    if (!ndkThread) {
        /* Error: could not create NDK stack thread */
        while(1);
    }
}
