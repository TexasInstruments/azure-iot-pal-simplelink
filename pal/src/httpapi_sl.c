// Copyright (c) Texas Instruments. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <ti/net/http/httpclient.h>

#include "cert_sl.h"

#include "azure_c_shared_utility/httpapi.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#define CONTENT_BUF_LEN     (128 * 10)
#define HTTP_SECURE_PORT    443
#define HEADER_TO_STR(x) (headerFieldStr[(x) & (~HTTPClient_REQUEST_HEADER_MASK)])

typedef struct {
    HTTPClient_Handle cli;
    char *prefixedHostName;
    char *x509Certificate;
    char *x509PrivateKey;
    bool  isConnected;
} HTTPAPI_Object;

static const char * headerFieldStr[] = {
"Age",
"Allow",
"Cache-Control",
"Connection",
"Content-Encoding",
"Content-Language",
"Content-Length",
"Content-Location",
"Content-Range",
"Content-Type",
"Date",
"ETag",
"Expires",
"Last-Modified",
"Location",
"Proxy-Authenticate",
"Retry-After",
"Server",
"Set-Cookie",
"Trailer",
"Transfer-Encoding",
"Upgrade",
"Vary",
"Via",
"Www-Authenticate",
"Warning",
"Accept",
"Accept-Charset",
"Accept-Encoding",
"Accept-Language",
"Authorization",
"Cookie",
"Expect",
"Forwarded",
"From",
"Host",
"If-Match",
"If-Modified-Since",
"If-None-Match",
"If-Range",
"If-Unmodified-Since",
"Origin",
"Proxy-Authorization",
"Range",
"TE",
"User-Agent",
};

static int16_t stringcasecmp (const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    int16_t result;

    if (p1 == p2) {
        return 0;
    }

    while ((result = tolower(*p1) - tolower(*p2++)) == 0) {
        if (*p1++ == '\0') {
            break;
        }
    }
    return result;
}

static const char* getHttpMethod(HTTPAPI_REQUEST_TYPE requestType)
{
    switch (requestType) {
        case HTTPAPI_REQUEST_GET:
            return (HTTP_METHOD_GET);
        case HTTPAPI_REQUEST_POST:
            return (HTTP_METHOD_POST);
        case HTTPAPI_REQUEST_PUT:
            return (HTTP_METHOD_PUT);
        case HTTPAPI_REQUEST_DELETE:
            return (HTTP_METHOD_DELETE);
        default:
            return (NULL);
    }
}

static int splitHeader(char *headerName, char **headerValue)
{
    *headerValue = strchr(headerName, ':');
    if (*headerValue == NULL) {
         return (-1);
    }

    **headerValue = '\0';
    (*headerValue)++;
    while (**headerValue == ' ') {
        (*headerValue)++;
    }

    return (0);
}

HTTPAPI_RESULT HTTPAPI_Init(void)
{
    return (HTTPAPI_OK);
}

void HTTPAPI_Deinit(void)
{
}

HTTP_HANDLE HTTPAPI_CreateConnection(const char* hostName)
{
    bool error = false;
    HTTPAPI_Object * apiH = NULL;
    int16_t statusCode;

    if (hostName == NULL) {
        LogError("Error: hostName is NULL");
        error = true;
        goto error;
    }

    apiH = calloc(sizeof(HTTPAPI_Object), 1);
    if (!apiH) {
        LogError("Error creating HTTPAPI object");
        error = true;
        goto error;
    }

    apiH->cli = HTTPClient_create(&statusCode, 0);
    if (statusCode < 0) {
        LogError("Error creating http client");
        error = true;
        goto error;
    }

    apiH->prefixedHostName = malloc(strlen(hostName) + 9);  /* https:// */
    if (!apiH->prefixedHostName) {
        LogError("Error creating hostname buffer");
        error = true;
        goto error;
    }
    /*
     * Microsoft is assuming we would connect through the secured port.
     * We need to drop a hint for HTTPClient_connect()
     */
    strcpy(apiH->prefixedHostName, "https://");
    strcat(apiH->prefixedHostName, hostName);

error:
    if ((error) && (apiH != NULL)) {
        if (apiH->cli != NULL) {
            HTTPClient_destroy(apiH->cli);
        }
        if (apiH->prefixedHostName != NULL) {
            free(apiH->prefixedHostName);
        }
        free(apiH);
        apiH = NULL;
    }

    return ((HTTP_HANDLE)apiH);
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    HTTPAPI_Object * apiH = (HTTPAPI_Object *)handle;

    if ((apiH) && (apiH->cli != NULL)) {
        if (apiH->isConnected) {
            HTTPClient_disconnect(apiH->cli);
        }
        if (apiH->prefixedHostName != NULL) {
            free(apiH->prefixedHostName);
        }
        if (apiH->x509Certificate != NULL) {
            free(apiH->x509Certificate);
        }
        if (apiH->x509PrivateKey != NULL) {
            free(apiH->x509PrivateKey);
        }
        HTTPClient_destroy(apiH->cli);
    }

    if (apiH != NULL) {
        free(apiH);
    }
}

HTTPAPI_RESULT HTTPAPI_ExecuteRequest(HTTP_HANDLE handle,
        HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
        HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
        size_t contentLength, unsigned int* statusCode,
        HTTP_HEADERS_HANDLE responseHeadersHandle,
        BUFFER_HANDLE responseContent)
{
    HTTPAPI_Object * apiH = (HTTPAPI_Object *)handle;
    HTTPClient_Handle cli;
    int i;
    int ret;
    HTTPAPI_RESULT result = HTTPAPI_OK;
    HTTP_HEADERS_RESULT hResult = HTTP_HEADERS_OK;
    int offset;
    size_t cnt;
    char *contentBuf = NULL;
    uint32_t contentBufLen = CONTENT_BUF_LEN;
    char *hname;
    char *hvalue;
    unsigned char *buffer = NULL;
    const char *method;
    bool moreFlag;
    HTTPClient_extSecParams esParams = {NULL, NULL, SL_SSL_CA_CERT};

    method = getHttpMethod(requestType);

    if ((handle == NULL) || (method == NULL) || (relativePath == NULL)
            || (statusCode == NULL) || (responseHeadersHandle == NULL)) {
        LogError("Invalid arguments: handle=%p, requestType=%d, "
            "relativePath=%p, statusCode=%p, responseHeadersHandle=%p",
            handle, (int)requestType, relativePath, statusCode,
            responseHeadersHandle);
        return (HTTPAPI_INVALID_ARG);
    }
    else if (HTTPHeaders_GetHeaderCount(httpHeadersHandle, &cnt)
            != HTTP_HEADERS_OK) {
        LogError("Cannot get header count");
        return (HTTPAPI_QUERY_HEADERS_FAILED);
    }

    cli = apiH->cli;
    if (cli == NULL) {
        LogError("Invalid argument. Client is null.");
        return (HTTPAPI_INVALID_ARG);
    }

    /*
     * We need to call HTTPClient_connect here as opposed to
     * HTTPAPI_CreateConnection because we might have set some TLS
     * options.
     */
    if (apiH->isConnected == false) {
        esParams.clientCert = apiH->x509Certificate;
        esParams.privateKey = apiH->x509PrivateKey;
        ret = HTTPClient_connect(apiH->cli, apiH->prefixedHostName,
                &esParams, 0);
        if (ret < 0) {
            LogError("HTTPClient_connect failed, ret=%d", ret);
            return (HTTPAPI_OPEN_REQUEST_FAILED);
        }
        else {
            apiH->isConnected = true;
        }
    }

    /* Send the request headers */
    while (cnt--) {
        ret = HTTPHeaders_GetHeader(httpHeadersHandle, cnt, &hname);
        if (ret != HTTP_HEADERS_OK) {
            LogError("Cannot get request header %d", cnt);
            return (HTTPAPI_QUERY_HEADERS_FAILED);
        }

        ret = splitHeader(hname, &hvalue);

        if (ret == 0) {
            /*
             * HOST and Content-Length headers are set by HTTPClient
             * automatically. Note that Content-Length = 0 never gets sent.
             */
            if ((stringcasecmp(hname, "content-length") != 0) &&
                    (stringcasecmp(hname, "host") != 0)) {
                ret = HTTPClient_setHeaderByName(cli,
                        HTTPClient_REQUEST_HEADER_MASK,
                        hname, hvalue,
                        strlen(hvalue) + 1, HTTPClient_HFIELD_NOT_PERSISTENT);
                if (ret < 0) {
                    LogError("Failed setting request header, ret=%d", ret);
                }
            }
        }
        else {
            LogError("Failed to split header");
        }

        free(hname);

        if (ret < 0) {
            return (HTTPAPI_SEND_REQUEST_FAILED);
        }
    }

    /* Send the request */
    ret = HTTPClient_sendRequest(cli, method,
            relativePath, (const char *)content, contentLength, 0);
    if (ret < 0) {
        LogError("HTTPClient_sendRequest failed, ret=%d", ret);
        return (HTTPAPI_SEND_REQUEST_FAILED);
    }

    *statusCode = (unsigned int)ret;

    /* Get the response headers */
    cnt = 0;
    offset = 0;

    contentBuf = (char *)malloc(CONTENT_BUF_LEN);
    if (contentBuf == NULL) {
        LogError("Failed allocating memory for contentBuf");
        result = HTTPAPI_ALLOC_FAILED;
        goto headersDone;
    }

    /*
     * TODO: If there is more than one header of the same name, is this
     * an issue with HTTPClient?
     */
    for (i = 0; i <= HTTPClient_MAX_RESPONSE_HEADER_FILEDS; i++) {
        contentBufLen = CONTENT_BUF_LEN;
        ret = HTTPClient_getHeader(cli, i, contentBuf, &contentBufLen, 0);
        if (ret == HTTPClient_EGETOPTBUFSMALL) {
            /* TODO: content buffer is too small. Enlarge and try again?? */
            LogError("Content buffer is too small for incoming header");
            result = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
        else if (ret < 0) {
            LogError("Failed to get header, ret=%d", ret);
            result = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
        else if (contentBufLen == 0) {
            /* No data for this header */
            continue;
        }

        hResult = HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle,
                HEADER_TO_STR(i), contentBuf);
        if (hResult != HTTP_HEADERS_OK) {
            LogError("Adding the response header failed");
            result = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
    }

headersDone:
    if (result != HTTPAPI_OK) {
        if (contentBuf) {
            free(contentBuf);
        }
        return (result);
    }

    /* Get response body */
    offset = 0;
    cnt = 0;

    do {
        /*
         * Note we would always try to read the response body, even when
         * responseContent is NULL, as it is still possible for the other
         * end to send a body in the latter case based on feedback from
         * Microsoft. This allows us to discard any unexpected body.
         */
        ret = HTTPClient_readResponseBody(cli, contentBuf, CONTENT_BUF_LEN,
                &moreFlag);

        if (ret < 0) {
            LogError("HTTP read response body failed, ret=%d", ret);
            result = HTTPAPI_RECEIVE_RESPONSE_FAILED;
            goto contentDone;
        }

        if ((ret != 0) && (responseContent != NULL)) {
            cnt = ret;
            ret = BUFFER_enlarge(responseContent, cnt);
            if (ret != 0) {
                LogError("Failed enlarging response buffer");
                result = HTTPAPI_ALLOC_FAILED;
                goto contentDone;
            }

            ret = BUFFER_content(responseContent,
                    (const unsigned char **)&buffer);
            if (ret != 0) {
                LogError("Failed getting the response buffer content");
                result = HTTPAPI_ALLOC_FAILED;
                goto contentDone;
            }

            memcpy(buffer + offset, contentBuf, cnt);
            offset += cnt;
        }
    } while (moreFlag);

contentDone:
    if (result != HTTPAPI_OK) {
        if (responseContent != NULL) {
            BUFFER_unbuild(responseContent);
        }
        if (contentBuf) {
            free(contentBuf);
        }
        return (result);
    }

    if (contentBuf) {
        free(contentBuf);
    }

    return (HTTPAPI_OK);
}

HTTPAPI_RESULT HTTPAPI_SetOption(HTTP_HANDLE handle, const char* optionName,
        const void* value)
{
    HTTPAPI_RESULT result;
    HTTPAPI_Object *apiH = (HTTPAPI_Object *)handle;

    if ((apiH == NULL) ||
            (optionName == NULL) ||
            (value == NULL)) {
        result = HTTPAPI_INVALID_ARG;
    }
    else if ((strcmp(OPTION_X509_ECC_CERT, optionName) == 0) ||
            (strcmp(SU_OPTION_X509_CERT, optionName) == 0)) {
        if (apiH->x509Certificate) {
            free(apiH->x509Certificate);
        }

        if (mallocAndStrcpy_s(&(apiH->x509Certificate), value) != 0) {
            result = HTTPAPI_ALLOC_FAILED;
            LogError("unable to allocate memory for the client certificate"
                    " in HTTPAPI_SetOption");
        }
        else {
            result = HTTPAPI_OK;
        }
    }
    else if ((strcmp(OPTION_X509_ECC_KEY, optionName) == 0) ||
            (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0)) {
        if (apiH->x509PrivateKey) {
            free(apiH->x509PrivateKey);
        }

        if (mallocAndStrcpy_s(&(apiH->x509PrivateKey), value) != 0) {
            result = HTTPAPI_ALLOC_FAILED;
            LogError("unable to allocate memory for the client private key"
                    " in HTTPAPI_SetOption");
        }
        else {
            result = HTTPAPI_OK;
        }
    }
    else {
        result = HTTPAPI_INVALID_ARG;
        LogError("unknown option %s", optionName);
    }

    return (result);
}

HTTPAPI_RESULT HTTPAPI_CloneOption(const char* optionName, const void* value,
        const void** savedValue)
{
    HTTPAPI_RESULT result;
    char *tempCert;

    if ((optionName == NULL) ||
            (value == NULL) ||
            (savedValue == NULL)) {
        result = HTTPAPI_INVALID_ARG;
    }
    else if ((strcmp(OPTION_X509_ECC_CERT, optionName) == 0) ||
            (strcmp(SU_OPTION_X509_CERT, optionName) == 0) ||
            (strcmp(OPTION_X509_ECC_KEY, optionName) == 0) ||
            (strcmp(SU_OPTION_X509_PRIVATE_KEY, optionName) == 0)) {
        if (mallocAndStrcpy_s(&tempCert, value) != 0) {
            result = HTTPAPI_ALLOC_FAILED;
            LogError("memory allocation failed in HTTPAPI_CloneOption");
        }
        else {
            *savedValue = tempCert;
            result = HTTPAPI_OK;
        }
    }
    else {
        result = HTTPAPI_INVALID_ARG;
        LogError("unknown option %s", optionName);
    }

    return (result);
}
