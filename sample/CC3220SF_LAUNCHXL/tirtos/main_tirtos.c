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
 *  ======== main_tirtos.c ========
 */
#include <stdio.h>

#include <ti/sysbios/BIOS.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

#include <ti/drivers/net/wifi/simplelink.h>

#include <pthread.h>

#include <simplesample_http.h>

#include <ti/net/slnetsock.h>
#include <ti/net/slnetif.h>

#include "Board.h"
#include "certs.h"
#include "network.h"

#define AZURE_IOT_ROOT_CA_FILENAME "/cert/ms.pem"
/*
 * The following macro is disabled by default. This is done to prevent the
 * certificate files from being written to flash every time the program
 * is run.  If an update to the cert files are needed, just update the
 * corresponding arrays, and rebuild with this macro defined. Note
 * you must remember to disable it otherwise the files will keep being
 * overwritten each time.
 */
#ifdef OVERWRITE_CERTS
static bool overwriteCerts = true;
#else
static bool overwriteCerts = false;
#endif

/*
 *  ======== flashFile ========
 *  Flashes file to the SFLASH using SimpleLink WiFi APIs
 *
 *  Returns number of bytes written on success or < 0 on failure
 */
int flashFile(const char *path, const uint8_t *buffer, uint32_t len)
{
    int32_t ret = -1;
    int32_t fileHandle;

    if (path && buffer) {
        fileHandle = sl_FsOpen((unsigned char *)path,
                SL_FS_CREATE | SL_FS_CREATE_SECURE | SL_FS_CREATE_NOSIGNATURE
                | SL_FS_CREATE_PUBLIC_WRITE | SL_FS_OVERWRITE
                | SL_FS_CREATE_MAX_SIZE(len), NULL);
        if (fileHandle > 0) {
            ret = sl_FsWrite(fileHandle, 0, (unsigned char *)buffer, len);
            sl_FsClose(fileHandle, NULL, NULL, 0);
        }
    }

    return (ret);
}

/*
 *  ======== flashCerts ========
 *  Utility function to flash the contents of a buffer (PEM format) into the
 *  filename/path specified by certName
 */
void flashCerts(uint8_t *certName, uint8_t *buffer, uint32_t bufflen)
{
    int status = 0;
    int16_t slStatus = 0;
    SlFsFileInfo_t fsFileInfo;

    /* Check if the cert file already exists */
    slStatus = sl_FsGetInfo(certName, 0, &fsFileInfo);

    /* If the cert doesn't exist, write it (or overwrite if specified to) */
    if (slStatus == SL_ERROR_FS_FILE_NOT_EXISTS || overwriteCerts == true) {

        printf("Flashing certificate file ...");

        status = flashFile((const char *)certName, buffer, bufflen);

        if (status < 0) {
            printf("Error: Could not write file %s to flash (%d)\n",
                    certName, status);
            while (1);
        }
        printf("successfully wrote file %s to flash\n", certName);
    }
}

/*
 *  ======== azureThreadFxn ========
 */
void *azureThreadFxn(void *arg0)
{
    SlNetCfgIpV4Args_t ipV4;
    uint16_t           len = sizeof(ipV4);
    uint16_t           dhcpIsOn;

    printf("Starting the simplesample_http example\n");

    /* Wait for an IP address, initialize the socket layer and get the time */
    Network_startup();

    /* Retrieve & print the IP address */
    sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &dhcpIsOn, &len,
            (unsigned char *)&ipV4);
    printf("CC32XX has connected to AP and acquired an IP address.\n");
    printf("IP Address: %ld.%ld.%ld.%ld\n", SL_IPV4_BYTE(ipV4.Ip, 3),
            SL_IPV4_BYTE(ipV4.Ip, 2), SL_IPV4_BYTE(ipV4.Ip, 1),
            SL_IPV4_BYTE(ipV4.Ip, 0));

    /* Flash Certificate Files */
    flashCerts((uint8_t *)AZURE_IOT_ROOT_CA_FILENAME, (uint8_t *)certificates,
            strlen(certificates));

    simplesample_http_run();

    return (NULL);
}

/*
 *  ======== main ========
 */
int main(int argc, char *argv[])
{
    pthread_attr_t pthreadAttrs;
    pthread_t slThread;
    pthread_t azureThread;
    int status;

    Board_initGeneral();
    GPIO_init();
    SPI_init();
    Network_init();

    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);

    /* Create the sl_Task thread */
    pthread_attr_init(&pthreadAttrs);

    status = pthread_attr_setstacksize(&pthreadAttrs, 2048);
    if (status != 0) {
        /* Error setting stack size */
        while (1);
    }

    status = pthread_create(&slThread, &pthreadAttrs, sl_Task, NULL);
    if (status != 0) {
        /* Failed to create sl_Task thread */
        while (1);
    }

    /* Create the AZURE thread */
    status = pthread_attr_setstacksize(&pthreadAttrs, 4096);
    if (status != 0) {
        /* Error setting stack size */
        while (1);
    }

    status = pthread_create(&azureThread, &pthreadAttrs, azureThreadFxn, NULL);
    if (status != 0) {
        /* Failed to create Azure thread */
        while (1);
    }

    pthread_attr_destroy(&pthreadAttrs);

    BIOS_start();

    return (0);
}
