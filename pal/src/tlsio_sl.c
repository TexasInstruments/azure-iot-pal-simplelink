// Copyright (c) Texas Instruments. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <errno.h>
#include <ti/net/slnetsock.h>
#include <ti/net/slnetif.h>
#include <ti/net/slnetutils.h>
#include <ti/net/slneterr.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "cert_sl.h"
#include "tlsio_sl.h"

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"

/*
 * Receive buffer size. Set to 64 as it seems to be the size used in most of
 * the other reference implementations. Increasing this would increase
 * performance at the cost of stack space requirement.
 */
#define RECV_BUFFER_SIZE 64

typedef enum TLSIO_STATE_ENUM_TAG
{
    TLSIO_STATE_NOT_OPEN,
    TLSIO_STATE_OPENING,
    TLSIO_STATE_OPEN,
    TLSIO_STATE_CLOSING,
    TLSIO_STATE_ERROR
} TLSIO_STATE_ENUM;

typedef struct TLS_IO_INSTANCE_TAG
{
    ON_BYTES_RECEIVED on_bytes_received;
    ON_IO_OPEN_COMPLETE on_io_open_complete;
    ON_IO_CLOSE_COMPLETE on_io_close_complete;
    ON_IO_ERROR on_io_error;
    void* on_bytes_received_context;
    void* on_io_open_complete_context;
    void* on_io_close_complete_context;
    void* on_io_error_context;
    TLSIO_STATE_ENUM tlsio_state;
    ON_SEND_COMPLETE on_send_complete;
    void* on_send_complete_callback_context;
    char* hostname;
    int port;
    int sock;
    SlNetSockSecAttrib_t *sec_attrib_hdl;
} TLS_IO_INSTANCE;

static const IO_INTERFACE_DESCRIPTION tlsio_sl_interface_description =
{
    tlsio_sl_retrieveoptions,
    tlsio_sl_create,
    tlsio_sl_destroy,
    tlsio_sl_open,
    tlsio_sl_close,
    tlsio_sl_send,
    tlsio_sl_dowork,
    tlsio_sl_setoption
};

static int init_sockaddr(struct sockaddr *addr, int port, const char *hostname)
{
    struct sockaddr_in taddr = {0};
    uint32_t ipAddr;
    uint16_t addrLen = sizeof(ipAddr);

    if (SlNetUtil_getHostByName(0, (char *)hostname, strlen(hostname),
            &ipAddr, &addrLen, AF_INET) < 0) {
        return (-1);
    }

    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(port);
    taddr.sin_addr.s_addr = htonl(ipAddr);
    *addr = *((struct sockaddr *)&taddr);

    return (0);
}

OPTIONHANDLER_HANDLE tlsio_sl_retrieveoptions(CONCRETE_IO_HANDLE handle)
{
    (void)handle;
    return NULL;
}

CONCRETE_IO_HANDLE tlsio_sl_create(void* io_create_parameters)
{
    TLSIO_CONFIG* tls_io_config = io_create_parameters;
    TLS_IO_INSTANCE* result;

    if (tls_io_config == NULL) {
        LogError("NULL tls_io_config");
        result = NULL;
    }
    else {
        result = malloc(sizeof(TLS_IO_INSTANCE));
        if (result == NULL) {
            LogError("NULL TLS_IO_INSTANCE");
        }
        else {
            memset(result, 0, sizeof(TLS_IO_INSTANCE));

            mallocAndStrcpy_s(&result->hostname, tls_io_config->hostname);

            result->port = tls_io_config->port;

            result->on_bytes_received = NULL;
            result->on_bytes_received_context = NULL;

            result->on_io_open_complete = NULL;
            result->on_io_open_complete_context = NULL;

            result->on_io_close_complete = NULL;
            result->on_io_close_complete_context = NULL;

            result->on_io_error = NULL;
            result->on_io_error_context = NULL;

            result->on_send_complete = NULL;
            result->on_send_complete_callback_context = NULL;
            result->sec_attrib_hdl = SlNetSock_secAttribCreate();
            result->tlsio_state = TLSIO_STATE_NOT_OPEN;
        }
    }

    return result;
}

void tlsio_sl_destroy(CONCRETE_IO_HANDLE tls_io)
{
    TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

    if (tls_io == NULL) {
        LogError("NULL tls_io");
    }
    else {
        if ((tls_io_instance->tlsio_state == TLSIO_STATE_OPENING) ||
            (tls_io_instance->tlsio_state == TLSIO_STATE_OPEN) ||
            (tls_io_instance->tlsio_state == TLSIO_STATE_CLOSING)) {
            LogError("TLS destroyed with a SSL connection still active.");
        }
        if (tls_io_instance->hostname != NULL) {
            free(tls_io_instance->hostname);
        }
        if (tls_io_instance->sec_attrib_hdl != NULL) {
            SlNetSock_secAttribDelete(tls_io_instance->sec_attrib_hdl);
        }
        free(tls_io_instance);
    }
}

int tlsio_sl_open(CONCRETE_IO_HANDLE tls_io,
                     ON_IO_OPEN_COMPLETE on_io_open_complete,
                     void* on_io_open_complete_context,
                     ON_BYTES_RECEIVED on_bytes_received,
                     void* on_bytes_received_context,
                     ON_IO_ERROR on_io_error,
                     void* on_io_error_context)
{
    int                   result = 0;
    bool                  error = false;
    int                   status = 0;
    uint16_t              clientSd;
    socklen_t             sdlen = sizeof(clientSd);
    TLS_IO_INSTANCE      *instance = (TLS_IO_INSTANCE*)tls_io;

    if (tls_io == NULL) {
        LogError("NULL tls_io");
        result = __FAILURE__;
        return (result);
    }
    else {
        int ret;

        if (instance->tlsio_state != TLSIO_STATE_NOT_OPEN) {
            LogError("IO should not be open: %d\n", instance->tlsio_state);
            result =  __FAILURE__;
            return (result);
        }
        else {
            instance->on_bytes_received = on_bytes_received;
            instance->on_bytes_received_context = on_bytes_received_context;

            instance->on_io_open_complete = on_io_open_complete;
            instance->on_io_open_complete_context = on_io_open_complete_context;

            instance->on_io_error = on_io_error;
            instance->on_io_error_context = on_io_error_context;

            instance->tlsio_state = TLSIO_STATE_OPENING;
            instance->sock = -1;

            struct sockaddr sa;
            ret = init_sockaddr(&sa, instance->port, instance->hostname);
            if (ret != 0) {
                LogError("Cannot resolve hostname");
                error = true;
                goto cleanup;
            }
            else {
                instance->sock = socket(sa.sa_family, SOCK_STREAM,
                                        0);
                if (instance->sock >= 0) {
                    if (getsockopt(instance->sock, SLNETSOCK_LVL_SOCKET,
                            SLNETSOCK_OPSOCK_SLNETSOCKSD,
                            &clientSd, &sdlen) < 0) {
                        LogError("getsockopt failed");
                        error = true;
                        goto cleanup;
                    }

                    status = SlNetSock_secAttribSet(instance->sec_attrib_hdl,
                            SLNETSOCK_SEC_ATTRIB_PEER_ROOT_CA, SL_SSL_CA_CERT,
                            sizeof(SL_SSL_CA_CERT));
                    if (status < 0) {
                        LogError("SlNetSock_secAttribSet failed");
                        error = true;
                        goto cleanup;
                    }

                    status = SlNetSock_startSec(clientSd,
                            instance->sec_attrib_hdl,
                            SLNETSOCK_SEC_BIND_CONTEXT_ONLY);
                    if (status < 0) {
                        LogError("SlNetSock_startSec failed to bind context");
                        error = true;
                        goto cleanup;
                    }

                    ret = connect(instance->sock, &sa,
                            sizeof(struct sockaddr_in));
                    if (ret < 0) {
                        LogError("Cannot connect");
                        error = true;
                        goto cleanup;
                    }
                    /* setup for nonblocking */
                    SlNetSock_Nonblocking_t nb;
                    nb.nonBlockingEnabled = 1;
                    setsockopt(instance->sock, SOL_SOCKET,
                            SO_NONBLOCKING, &nb,
                            sizeof(nb));
                }
                else {
                    LogError("Cannot open socket");
                    error = true;
                    goto cleanup;
                }
            }

            status = SlNetSock_startSec(clientSd,
                    instance->sec_attrib_hdl,
                    SLNETSOCK_SEC_START_SECURITY_SESSION_ONLY);
            if (status < 0) {
                LogError("SlNetSock_startSec failed to start session\n");
                error = true;
                goto cleanup;
            }

            IO_OPEN_RESULT oresult = result == __FAILURE__ ? IO_OPEN_ERROR :
                                                             IO_OPEN_OK;
            instance->tlsio_state = TLSIO_STATE_OPEN;
            instance->on_io_open_complete(instance->on_io_open_complete_context,
                                          oresult);
            if (oresult == IO_OPEN_ERROR) {
                if (on_io_error != NULL) {
                    (void)on_io_error(on_io_error_context);
                }
            }
        }
    }

cleanup:

    if (error) {
        instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
        result = __FAILURE__;
        if (instance->sock >= 0) {
            close(instance->sock);
        }
    }

    return result;
}

int tlsio_sl_close(CONCRETE_IO_HANDLE tls_io,
                        ON_IO_CLOSE_COMPLETE on_io_close_complete,
                        void* callback_context)
{
    int result = 0;

    if (tls_io == NULL) {
        LogError("NULL tls_io");
        result = __FAILURE__;
    }
    else {
        TLS_IO_INSTANCE* instance = (TLS_IO_INSTANCE*)tls_io;

        if ((instance->tlsio_state == TLSIO_STATE_NOT_OPEN) ||
            (instance->tlsio_state == TLSIO_STATE_CLOSING)) {
            LogError("Invalid state in tlsio_sl_close");
            result = __FAILURE__;
        }
        else {
            instance->tlsio_state = TLSIO_STATE_CLOSING;
            instance->on_io_close_complete = on_io_close_complete;
            instance->on_io_close_complete_context = callback_context;

            close(instance->sock);

            instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
            instance->on_io_close_complete(
                                       instance->on_io_close_complete_context);
        }
    }

    return result;
}

int tlsio_sl_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size,
                     ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    int result;

    if (tls_io == NULL) {
        LogError("NULL tls_io");
        result = __FAILURE__;
    }
    else {
        TLS_IO_INSTANCE* instance = (TLS_IO_INSTANCE*)tls_io;

        if (instance->tlsio_state != TLSIO_STATE_OPEN) {
            LogError("Invalid state in tlsio_sl_send");
            result = __FAILURE__;
        }
        else {
            const char* buf = (const char*)buffer;
            instance->on_send_complete = on_send_complete;
            instance->on_send_complete_callback_context = callback_context;

            result = 0;
            while (size) {
                int res = send(instance->sock, buf, size, 0);
                if ((res < 0) && (errno != EAGAIN)) {
                    result = __FAILURE__;
                    break;
                }
                else if (((res < 0) && (errno == EAGAIN)) || (res < size)) {
                    /* no more space left, lets wait for more to become
                     * available and try again.
                     */
                    ThreadAPI_Sleep(10);
                }
                if (res > 0) {
                    size -= res;
                    buf += res;
                }
            }
            IO_SEND_RESULT oresult = result == __FAILURE__ ? IO_SEND_ERROR :
                                                             IO_SEND_OK;
            instance->on_send_complete(
                          instance->on_send_complete_callback_context, oresult);
        }
    }

    return result;
}

void tlsio_sl_dowork(CONCRETE_IO_HANDLE tls_io)
{
    if (tls_io != NULL) {
        TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

        if ((tls_io_instance->tlsio_state != TLSIO_STATE_NOT_OPEN) &&
            (tls_io_instance->tlsio_state != TLSIO_STATE_ERROR)) {
            unsigned char buffer[RECV_BUFFER_SIZE];
            int rcv_bytes = 1;

            while (rcv_bytes > 0) {
                rcv_bytes =  recv(tls_io_instance->sock, buffer,
                        sizeof(buffer), 0);
                if (rcv_bytes > 0) {
                    if (tls_io_instance->on_bytes_received != NULL) {
                        tls_io_instance->on_bytes_received(
                                     tls_io_instance->on_bytes_received_context,
                                     buffer, rcv_bytes);
                    }
                }
            }
        }
    }
}

const IO_INTERFACE_DESCRIPTION* tlsio_sl_get_interface_description(void)
{
    return &tlsio_sl_interface_description;
}

int tlsio_sl_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName,
        const void* value)
{
    int result = 0;
    TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

    /*
     * We are expecting 'value' to be the name of the secure object/file
     * containing the certificate/key. This is to allow users the
     * flexibility of flashing the certificates using Uniflash on
     * platforms where this is supported, as opposed to necessarily
     * bundling them into the executable.
     */
    if ((strcmp("x509EccCertificate", optionName) == 0) ||
           (strcmp("x509certificate", optionName) == 0)) {
        result = SlNetSock_secAttribSet(tls_io_instance->sec_attrib_hdl,
                SLNETSOCK_SEC_ATTRIB_LOCAL_CERT, (void *)value,
                strlen(value));
    }
    else if ((strcmp("x509EccAliasKey", optionName) == 0) ||
            (strcmp("x509privatekey", optionName) == 0)) {
        result = SlNetSock_secAttribSet(tls_io_instance->sec_attrib_hdl,
                SLNETSOCK_SEC_ATTRIB_PRIVATE_KEY, (void *)value,
                strlen(value));
    }

    return result;
}
