/*
 * Copyright (c) 2018, Texas Instruments Incorporated
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

#include <stdbool.h>

#include <ti/net/slnetsock.h>
#include <ti/net/slnetif.h>
#include <ti/ndk/slnetif/slnetifndk.h>
#include <ti/net/slnetutils.h>

#include <ti/drivers/GPIO.h>

#include <semaphore.h>

#include "Board.h"

/* Network interface priority and name */
#define NDK_ETH_IF_PRI (5)
#define NDK_ETH_IF_NAME "eth0"

static sem_t sem;

extern void startSNTP(void);

/*
 *  ======== netIPAddrHook ========
 *  User defined NDK network IP address hook
 */
void netIPAddrHook(uint32_t IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
    if (fAdd) {
        /* Signal that the NDK stack is ready and has an IP address */
        sem_post(&sem);
    }
}

/*
 *  ======== netOpenHook ========
 *  User defined NDK network open hook
 */
void netOpenHook()
{
}

/*
 *  ======== Network_init =======
 */
void Network_init()
{
    if (sem_init(&sem, 0, 0) != 0) {
        /* Error: sem_init failed */
        while (1);
    }
}

/*
 *  ======== Network_startup =======
 */
void Network_startup()
{
    int32_t status;

    /* Wait for the network stack to initialize and acquire an IP address */
    sem_wait(&sem);

    /* The network stack is ready. Initialize the socket layer */
    status = SlNetSock_init(0);
    if (status != 0) {
        /* SlNetSock_init failed */
        while (1);
    }

    status = SlNetIf_init(0);
    if (status != 0) {
        /* SlNetIf_init failed */
        while (1);
    }

    status = SlNetUtil_init(0);
    if (status != 0) {
        /* SlNetUtil_init failed */
        while (1);
    }

    /* Register the NDK ethernet interface with the socket layer */
    status = SlNetIf_add(SLNETIF_ID_2, NDK_ETH_IF_NAME,
            (const SlNetIf_Config_t *)&SlNetIfConfigNDKSec, NDK_ETH_IF_PRI);
    if (status != 0) {
        /* SlNetIf_add failed */
        while (1);
    }

    /* Turn LED OFF. It will be used as a connection indicator */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_OFF);

    /* Use SNTP to get the current time, as needed for SSL authentication */
    startSNTP();

    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
}

/*
 *  ======== Network_exit =======
 */
void Network_exit()
{
}
