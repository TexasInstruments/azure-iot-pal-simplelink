/*
 * Copyright (c) 2018-2020, Texas Instruments Incorporated
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
 *  ======== main_tirtos.c ========
 */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* Driver configuration */
#include <ti/display/Display.h>
#include <ti/drivers/Board.h>
#include <ti/drivers/GPIO.h>

#include <pthread.h>

#include <simplesample_http.h>

#include <ti/ndk/inc/netmain.h>
#include <ti/net/slnetsock.h>
#include <ti/net/slnetif.h>

#include "ti_drivers_config.h"
#include "certs.h"
#include "network.h"

#define AZURE_IOT_ROOT_CA_OBJNAME "/cert/ms.pem"

Display_Handle display;

extern void ti_ndk_config_Global_startupFxn();

/*
 *  ======== azureThreadFxn ========
 */
void *azureThreadFxn(void *arg0)
{
    int32_t status;

    /* Open an NDK file descriptor session */
    fdOpenSession(Task_self());

    Display_printf(display, 0, 0, "Starting the simplesample_http example");

    /* Wait for an IP address, initialize the socket layer and get the time */
    Network_startup();

    /* Load certificates */
    status = SlNetIf_loadSecObj(SLNETIF_SEC_OBJ_TYPE_CERTIFICATE,
            AZURE_IOT_ROOT_CA_OBJNAME, strlen(AZURE_IOT_ROOT_CA_OBJNAME),
            (uint8_t *)certificates, strlen(certificates) + 1, SLNETIF_ID_2);
    if (status < 0) {
        Display_printf(display, 0, 0, "Failed to load certificate object");
        while (1);
    }

    simplesample_http_run();

    /* Close the NDK file descriptor session */
    fdCloseSession(Task_self());

    return (NULL);
}

/*
 *  ======== main ========
 */
int main(int argc, char *argv[])
{
    pthread_attr_t pthreadAttrs;
    pthread_t azureThread;
    int status;

    Board_init();
    GPIO_init();
    Display_init();
    Network_init();

    /* Open the Display for output */
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL) {
        /* Failed to open the Display driver */
        while (1);
    }

    ti_ndk_config_Global_startupFxn();

    /* Create the AZURE thread */
    pthread_attr_init(&pthreadAttrs);

    status = pthread_attr_setstacksize(&pthreadAttrs, 8192);
    if (status != 0) {
        /* Error setting stack size */
        while (1);
    }

    status = pthread_create(&azureThread, &pthreadAttrs, azureThreadFxn, NULL);
    if (status != 0) {
        /* Failed to create AZURE thread */
        while (1);
    }

    pthread_attr_destroy(&pthreadAttrs);

    BIOS_start();

    return (0);
}
