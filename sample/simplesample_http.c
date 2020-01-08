// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that HTTP only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "serializer.h"
#include "iothub_client_ll.h"
#include "iothubtransporthttp.h"

#include <ti/display/Display.h>

extern Display_Handle display;

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/*String containing Hostname, Device Id & Device Key in the format:             */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
static const char* connectionString = "[device connection string]";

// Define the Model
BEGIN_NAMESPACE(WeatherStation);

DECLARE_MODEL(ContosoAnemometer,
WITH_DATA(ascii_char_ptr, DeviceId),
WITH_DATA(int, WindSpeed),
WITH_DATA(float, Temperature),
WITH_DATA(float, Humidity),
WITH_ACTION(TurnFanOn),
WITH_ACTION(TurnFanOff),
WITH_ACTION(SetAirResistance, int, Position)
);

END_NAMESPACE(WeatherStation);

static char propText[1024];

/*
 * To enable logging in the Azure library the following steps must be done:
 *   1. Link against the debug version of the Azure libraries, e.g.
 *      pal_sl_debug.a, common_sl_debug.a, ...
 *   2. Uncomment the following #define ENABLE_LOGGING
 */
// #define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
void Display_log(LOG_CATEGORY log_category, const char* file, const char* func,
        int line, unsigned int options, const char* format, ...)
{
    if ((display != NULL) && (display->fxnTablePtr != NULL) &&
            (display->fxnTablePtr->vprintfFxn != NULL)) {
        va_list args;

        switch (log_category) {
            case AZ_LOG_ERROR:
                Display_printf(display, 0, 0, "Error: File:%s Func:%s Line:%d ",
                        file, func, line);
                break;
            default:
                break;
        }

        va_start(args, format);
        /*
         * Display currently doesn't provide a Display_vprintf() function
         * (TIDRIVERS-4111), so use the underlying one.
         */
        display->fxnTablePtr->vprintfFxn(display, 0, 0, (char *)format, args);
        va_end(args);
    }
}
#endif // ENABLE_LOGGING

EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device)
{
    (void)device;
    Display_printf(display, 0, 0, "Turning fan on.");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device)
{
    (void)device;
    Display_printf(display, 0, 0, "Turning fan off.");
    return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position)
{
    (void)device;
    Display_printf(display, 0, 0, "Setting Air Resistance Position to %d.", Position);
    return EXECUTE_COMMAND_SUCCESS;
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    unsigned int messageTrackingId = (unsigned int)(uintptr_t)userContextCallback;

    Display_printf(display, 0, 0, "Message Id: %u Received.", messageTrackingId);

    Display_printf(display, 0, 0, "Result Call Back Called! Result is: %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
    static unsigned int messageTrackingId;
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
    if (messageHandle == NULL)
    {
        Display_printf(display, 0, 0, "unable to create a new IoTHubMessage");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
        {
            Display_printf(display, 0, 0, "failed to hand over the message to IoTHubClient");
        }
        else
        {
            Display_printf(display, 0, 0, "IoTHubClient accepted the message for delivery");
        }
        IoTHubMessage_Destroy(messageHandle);
    }
    free((void*)buffer);
    messageTrackingId++;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        Display_printf(display, 0, 0, "unable to IoTHubMessage_GetByteArray");
        result = IOTHUBMESSAGE_ABANDONED;
    }
    else
    {
        /*buffer is not zero terminated*/
        char* temp = malloc(size + 1);
        if (temp == NULL)
        {
            Display_printf(display, 0, 0, "failed to malloc");
            result = IOTHUBMESSAGE_ABANDONED;
        }
        else
        {
            EXECUTE_COMMAND_RESULT executeCommandResult;

            (void)memcpy(temp, buffer, size);
            temp[size] = '\0';
            executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
            result =
                (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
                (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
                IOTHUBMESSAGE_REJECTED;
            free(temp);
        }
    }
    return result;
}

void simplesample_http_run(void)
{
#ifdef ENABLE_LOGGING
    xlogging_set_log_function(Display_log);
#endif // ENABLE_LOGGING

    if (platform_init() != 0)
    {
        Display_printf(display, 0, 0, "Failed to initialize the platform.");
    }
    else
    {
        if (serializer_init(NULL) != SERIALIZER_OK)
        {
            Display_printf(display, 0, 0, "Failed on serializer_init");
        }
        else
        {
            IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
            int avgWindSpeed = 10;
            float minTemperature = 20.0;
            float minHumidity = 60.0;

            srand((unsigned int)time(NULL));

            if (iotHubClientHandle == NULL)
            {
                Display_printf(display, 0, 0, "Failed on IoTHubClient_LL_Create");
            }
            else
            {
                // Because it can poll "after 9 seconds" polls will happen
                // effectively at ~10 seconds.
                // Note that for scalabilty, the default value of minimumPollingTime
                // is 25 minutes. For more information, see:
                // https://azure.microsoft.com/documentation/articles/iot-hub-devguide/#messaging
                unsigned int minimumPollingTime = 9;
                ContosoAnemometer* myWeather;

                if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
                {
                    Display_printf(display, 0, 0, "failure to set option \"MinimumPollingTime\"");
                }

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
                // For mbed add the certificate information
                if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
                {
                    Display_printf(display, 0, 0, "failure to set option \"TrustedCerts\"");
                }
#endif // SET_TRUSTED_CERT_IN_SAMPLES

                myWeather = CREATE_MODEL_INSTANCE(WeatherStation, ContosoAnemometer);
                if (myWeather == NULL)
                {
                    Display_printf(display, 0, 0, "Failed on CREATE_MODEL_INSTANCE");
                }
                else
                {
                    if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, myWeather) != IOTHUB_CLIENT_OK)
                    {
                        Display_printf(display, 0, 0, "unable to IoTHubClient_SetMessageCallback");
                    }
                    else
                    {
                        myWeather->DeviceId = "myFirstDevice";
                        myWeather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
                        myWeather->Temperature = minTemperature + (rand() % 10);
                        myWeather->Humidity = minHumidity + (rand() % 20);
                        {
                            unsigned char* destination;
                            size_t destinationSize;
                            if (SERIALIZE(&destination, &destinationSize, myWeather->DeviceId, myWeather->WindSpeed, myWeather->Temperature, myWeather->Humidity) != CODEFIRST_OK)
                            {
                                Display_printf(display, 0, 0, "Failed to serialize");
                            }
                            else
                            {
                                IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(destination, destinationSize);
                                if (messageHandle == NULL)
                                {
                                    Display_printf(display, 0, 0, "unable to create a new IoTHubMessage");
                                }
                                else
                                {
                                    MAP_HANDLE propMap = IoTHubMessage_Properties(messageHandle);
                                    (void)sprintf_s(propText, sizeof(propText), myWeather->Temperature > 28 ? "true" : "false");
                                    if (Map_AddOrUpdate(propMap, "temperatureAlert", propText) != MAP_OK)
                                    {
                                        Display_printf(display, 0, 0, "ERROR: Map_AddOrUpdate Failed!");
                                    }

                                    if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)1) != IOTHUB_CLIENT_OK)
                                    {
                                        Display_printf(display, 0, 0, "failed to hand over the message to IoTHubClient");
                                    }
                                    else
                                    {
                                        Display_printf(display, 0, 0, "IoTHubClient accepted the message for delivery");
                                    }

                                    IoTHubMessage_Destroy(messageHandle);
                                }
                                free(destination);
                            }
                        }

                        /* wait for commands */
                        while (1)
                        {
                            IoTHubClient_LL_DoWork(iotHubClientHandle);
                            ThreadAPI_Sleep(100);
                        }
                    }

                    DESTROY_MODEL_INSTANCE(myWeather);
                }
                IoTHubClient_LL_Destroy(iotHubClientHandle);
            }
            serializer_deinit();
        }
        platform_deinit();
    }
}
