/*
 * Copyright (c) 2017-2019, Texas Instruments Incorporated
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
#include <stdint.h>

#include <ti/net/slnet.h>

#include <ti/drivers/GPIO.h>

#include <ti/drivers/net/wifi/netapp.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/wlan.h>

#include <semaphore.h>
#include <unistd.h>

#include "ti_drivers_config.h"
#include "wificonfig.h"

/* Provisioning inactivity timeout in seconds */
#define PROV_INACTIVITY_TIMEOUT (600)

static uint32_t deviceConnected = false;
static uint32_t ipAcquired = false;
static uint32_t provisioning = false;

static sem_t sem;

extern void startSNTP(void);

static void initWiFi();

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

    /* Connect to the Access Point */
    initWiFi();

    /* Wait for the network stack to initialize and acquire an IP address */
    sem_wait(&sem);

    /* initialize SlNet interface(s) */
    status = ti_net_SlNet_initConfig();
    if (status < 0) {
        /* ti_net_SlNet_initConfig failed */
        while (1);
    }

    /* Turn LED OFF. It will be used as a connection indicator */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

    /* Use SNTP to get the current time, as needed for SSL authentication */
    startSNTP();

    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
}

/*
 *  ======== Network_exit =======
 */
void Network_exit()
{
}

/*
 *
 *  SimpleLink Asynchronous Event Handlers -- Start
 *
 */

/*
 *  ======== SimpleLinkHttpServerEventHandler ========
 */
void SimpleLinkHttpServerEventHandler(
        SlNetAppHttpServerEvent_t *pSlHttpServerEvent,
        SlNetAppHttpServerResponse_t *pSlHttpServerResponse)
{
}

/*
 *  ======== SimpleLinkNetAppRequestEventHandler ========
 */
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
        SlNetAppResponse_t *pNetAppResponse)
{
}

/*
 *  ======== SimpleLinkNetAppRequestMemFreeEventHandler ========
 */
void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{
}

/*
 *  ======== SimpleLinkWlanEventHandler ========
 *  SimpleLink Host Driver callback for handling WLAN connection or
 *  disconnection events.
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    switch (pWlanEvent->Id) {
        case SL_WLAN_EVENT_CONNECT:
            deviceConnected = true;
            break;

        case SL_WLAN_EVENT_DISCONNECT:
            deviceConnected = false;
            break;

        case SL_WLAN_EVENT_PROVISIONING_STATUS:
            switch (pWlanEvent->Data.ProvisioningStatus.ProvisioningStatus) {
                case SL_WLAN_PROVISIONING_CONFIRMATION_WLAN_CONNECT:
                    deviceConnected = true;
                    break;

                case SL_WLAN_PROVISIONING_CONFIRMATION_IP_ACQUIRED:
                    ipAcquired = true;
                    break;

                case SL_WLAN_PROVISIONING_STOPPED:
                    if (pWlanEvent->Data.ProvisioningStatus.Role == ROLE_STA) {
                        if (pWlanEvent->Data.ProvisioningStatus.WlanStatus ==
                                SL_WLAN_STATUS_CONNECTED) {
                            /* The WiFi stack is ready and has an IP address */
                            provisioning = false;
                            sem_post(&sem);
                        }
                    }

                    break;

                default:
                    break;
            }
        default:
            break;
    }
}

/*
 *  ======== SimpleLinkFatalErrorEventHandler ========
 *  This function handles fatal errors
 *
 *  \param[in]  slFatalErrorEvent - Pointer to the fatal error event info
 *
 *  \return     None
 */
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
    switch (slFatalErrorEvent->Id) {
        case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
            /* FATAL ERROR: Abort NWP event detected */
            while (1);

        case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
            /* FATAL ERROR: Driver Abort detected */
            while (1);

        case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
            /* FATAL ERROR: No Cmd Ack detected */
            while (1);

        case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
            /* FATAL ERROR: Sync loss detected */
            while (1);

        case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
            /* FATAL ERROR: Async event timeout detected */
            while (1);

        default:
            /* FATAL ERROR: Unspecified error detected */
            while (1);
    }
}

/*
 *  ======== SimpleLinkNetAppEventHandler ========
 *  SimpleLink Host Driver callback for asynchoronous IP address events.
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if (pNetAppEvent == NULL) {
        return;
    }

    switch (pNetAppEvent->Id) {
        case SL_NETAPP_EVENT_IPV4_ACQUIRED:
        case SL_NETAPP_EVENT_IPV6_ACQUIRED:
            ipAcquired = true;

            /*  Signal that the WiFi stack is ready and has an IP address */
            sem_post(&sem);

            break;

        default:
            break;
    }
}

/*
 *  ======== SimpleLinkSockEventHandler ========
 */
void SimpleLinkSockEventHandler(SlSockEvent_t *pArgs)
{
}

/*
 *  ======== SimpleLinkGeneralEventHandler ========
 */
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *sockEvent)
{
}

/*
 *  ======== provisioningFxn ========
 */
void provisioningFxn()
{
    int32_t status;

    sl_NetAppStart(SL_NETAPP_HTTP_SERVER_ID);

    sl_WlanSetMode(ROLE_AP);

    /* Restart network processor */
    sl_Stop(200);
    status = sl_Start(0, 0, 0);
    if (status < 0) {
        /* Error: Failed to set SimpleLink Wi-Fi to AP mode */
        while(1);
    }

    /* Set connection variables to initial values */
    deviceConnected = false;
    ipAcquired = false;
    provisioning = true;

    /* Start AP+Smart Config provisioning */
    status = sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_START_MODE_APSC,
            ROLE_STA, PROV_INACTIVITY_TIMEOUT, NULL, 0);
    if (status < 0) {
        /* Error: Failed to start provisioning */
        while(1);
    }
}

/*
 *  ======== wlanConnect =======
 *  Secure connection parameters
 */
static int wlanConnect()
{
    SlWlanSecParams_t secParams = {0};
    int ret = 0;

    if (strlen(SSID) != 0) {
        secParams.Key = (signed char *)SECURITY_KEY;
        secParams.KeyLen = strlen((const char *)secParams.Key);
        secParams.Type = SECURITY_TYPE;

        ret = sl_WlanConnect((signed char*)SSID, strlen((const char*)SSID),
                NULL, &secParams, NULL);
    }

    return (ret);
}

/*
 *  ======== initWiFi =======
 *  Start the NWP and connect to AP
 */
static void initWiFi()
{
    int32_t status;
    uint32_t currButton;
    uint32_t prevButton = 0;

    status = sl_WifiConfig();
    if (status < 0) {
        /* sl_WifiConfig failed */
        while (1);
    }

    /* Set connection variables to initial values */
    deviceConnected = false;
    ipAcquired = false;
    provisioning = false;

    /* Host driver starts the network processor */
    if (sl_Start(NULL, NULL, NULL) < 0) {
        /* Error: Could not initialize WiFi */
        while (1);
    }

    if (wlanConnect() < 0) {
        /* Error: Could not connect to WiFi AP */
        while (1);
    }

    /*
     * Wait for the WiFi to connect to an AP. If a profile for the AP in
     * use has not been stored yet, press Board_GPIO_BUTTON0 to start
     * provisioning.
     */
    while ((deviceConnected != true) || (ipAcquired != true) ||
            (provisioning == true)) {
        /*
         *  Start provisioning if a button is pressed. This could be done with
         *  GPIO interrupts, but for simplicity polling is used to check the
         *  button.
         */
        currButton = GPIO_read(CONFIG_GPIO_BUTTON_0);
        if ((currButton == 0) && (prevButton != 0)) {
            provisioningFxn();
        }
        prevButton = currButton;
        usleep(50000);
    }
}
