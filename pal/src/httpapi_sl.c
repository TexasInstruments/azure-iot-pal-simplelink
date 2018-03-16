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
    HTTPAPI_Object * apiH;
    int16_t statusCode;

    apiH = calloc(sizeof(HTTPAPI_Object), 1);
    if (!apiH) {
        LogError("Error creating HTTPAPI object\n");
        error = true;
        goto error;
    }

    apiH->cli = HTTPClient_create(&statusCode, 0);
    if (statusCode < 0) {
        LogError("Error creating http client\n");
        error = true;
        goto error;
    }

    apiH->prefixedHostName = malloc(strlen(hostName) + 9);  /* https:// */
    if (!apiH->prefixedHostName) {
        LogError("Error creating hostname buffer\n");
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
    if (error) {
        if (apiH->cli != NULL) {
            HTTPClient_destroy(apiH->cli);
        }
        if (apiH->prefixedHostName != NULL) {
            free(apiH->prefixedHostName);
        }
        if (apiH != NULL) {
            free(apiH);
            apiH = NULL;
        }
    }

    return ((HTTP_HANDLE)apiH);
}

void HTTPAPI_CloseConnection(HTTP_HANDLE handle)
{
    HTTPAPI_Object * apiH = (HTTPAPI_Object *)handle;

    if (apiH->cli != NULL) {
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
    HTTPClient_Handle cli = apiH->cli;
    int i;
    int ret;
    int offset;
    size_t cnt;
    char contentBuf[CONTENT_BUF_LEN] = {0};
    uint32_t contentBufLen = CONTENT_BUF_LEN;
    char *hname;
    char *hvalue;
    const char *method;
    bool moreFlag;
    HTTPClient_extSecParams esParams = {NULL, NULL, SL_SSL_CA_CERT};

    method = getHttpMethod(requestType);

    if ((cli == NULL) || (method == NULL) || (relativePath == NULL)
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

        free(hname);
        hname = NULL;

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
            ret = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
        else if (ret < 0) {
            LogError("Failed to get header, ret=%d", ret);
            ret = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
        else if (contentBufLen == 0) {
            /* No data for this header */
            continue;
        }

        ret = HTTPHeaders_AddHeaderNameValuePair(responseHeadersHandle,
                HEADER_TO_STR(i), contentBuf);
        if (ret != HTTP_HEADERS_OK) {
            LogError("Adding the response header failed");
            ret = HTTPAPI_HTTP_HEADERS_FAILED;
            goto headersDone;
        }
        offset = 0;
    }

headersDone:
    hname = NULL;
    if (ret != 0) {
        return ((HTTPAPI_RESULT)ret);
    }

    /* Get response body */
    if (responseContent != NULL) {
        offset = 0;
        cnt = 0;

        do {
            ret = HTTPClient_readResponseBody(cli, contentBuf, CONTENT_BUF_LEN,
                    &moreFlag);

            if (ret < 0) {
                LogError("HTTP read response body failed, ret=%d", ret);
                ret = HTTPAPI_RECEIVE_RESPONSE_FAILED;
                goto contentDone;
            }

            if (ret != 0) {
                cnt = ret;
                ret = BUFFER_enlarge(responseContent, cnt);
                if (ret != 0) {
                    LogError("Failed enlarging response buffer");
                    ret = HTTPAPI_ALLOC_FAILED;
                    goto contentDone;
                }

                ret = BUFFER_content(responseContent,
                        (const unsigned char **)&hname);
                if (ret != 0) {
                    LogError("Failed getting the response buffer content");
                    ret = HTTPAPI_ALLOC_FAILED;
                    goto contentDone;
                }

                memcpy(hname + offset, contentBuf, cnt);
                offset += cnt;
            }
        } while (moreFlag);

    contentDone:
        if (ret < 0) {
            BUFFER_unbuild(responseContent);
            return ((HTTPAPI_RESULT)ret);
        }
    }

    return (HTTPAPI_OK);
}

HTTPAPI_RESULT HTTPAPI_SetOption(HTTP_HANDLE handle, const char* optionName,
        const void* value)
{
    HTTPAPI_RESULT result;
    HTTPAPI_Object *apiH = (HTTPAPI_Object *)handle;
    int len;

    if ((apiH == NULL) ||
            (optionName == NULL) ||
            (value == NULL)) {
        result = HTTPAPI_INVALID_ARG;
    }
    else if ((strcmp("x509EccCertificate", optionName) == 0) ||
            (strcmp("x509certificate", optionName) == 0)) {
        if (apiH->x509Certificate) {
            free(apiH->x509Certificate);
        }

        len = strlen((char *)value);
        apiH->x509Certificate = (char *)malloc((len + 1) * sizeof(char));
        if (apiH->x509Certificate == NULL) {
            result = HTTPAPI_ALLOC_FAILED;
            LogInfo("unable to allocate memory for the client certificate"
                    " in HTTPAPI_SetOption");
        }
        else {
            strcpy(apiH->x509Certificate, (const char *)value);
            result = HTTPAPI_OK;
        }
    }
    else if ((strcmp("x509EccAliasKey", optionName) == 0) ||
            (strcmp("x509privatekey", optionName) == 0)) {
        if (apiH->x509PrivateKey) {
            free(apiH->x509PrivateKey);
        }

        len = strlen((char *)value);
        apiH->x509PrivateKey = (char *)malloc((len + 1) * sizeof(char));
        if (apiH->x509PrivateKey == NULL) {
            result = HTTPAPI_ALLOC_FAILED;
            LogInfo("unable to allocate memory for the client private key"
                    " in HTTPAPI_SetOption");
        }
        else {
            strcpy(apiH->x509PrivateKey, (const char*)value);
            result = HTTPAPI_OK;
        }
    }
    else {
        result = HTTPAPI_INVALID_ARG;
        LogInfo("unknown option %s", optionName);
    }

    return (result);
}

HTTPAPI_RESULT HTTPAPI_CloneOption(const char* optionName, const void* value,
        const void** savedValue)
{
    HTTPAPI_RESULT result;
    size_t certLen;
    char *tempCert;

    if ((optionName == NULL) ||
            (value == NULL) ||
            (savedValue == NULL)) {
        result = HTTPAPI_INVALID_ARG;
    }
    else if ((strcmp("x509EccCertificate", optionName) == 0) ||
            (strcmp("x509certificate", optionName) == 0) ||
            (strcmp("x509EccAliasKey", optionName) == 0) ||
            (strcmp("x509privatekey", optionName) == 0)) {
        certLen = strlen((const char *)value);
        tempCert = (char *)malloc((certLen + 1) * sizeof(char));
        if (tempCert == NULL) {
            result = HTTPAPI_ALLOC_FAILED;
        }
        else {
            strcpy(tempCert, (const char*)value);
            *savedValue = tempCert;
            result = HTTPAPI_OK;
        }
    }
    else {
        result = HTTPAPI_INVALID_ARG;
        LogInfo("unknown option %s", optionName);
    }

    return (result);
}
