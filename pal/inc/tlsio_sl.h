// Copyright (c) 2020 Texas Instruments Incorporated. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TLSIO_SL_H
#define TLSIO_SL_H

#include "azure_c_shared_utility/xio.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/optionhandler.h"

#ifdef __cplusplus
extern "C" {
#include <cstddef>
#else
#include <stddef.h>
#endif /* __cplusplus */

extern CONCRETE_IO_HANDLE tlsio_sl_create(void* io_create_parameters);
extern void tlsio_sl_destroy(CONCRETE_IO_HANDLE tls_io);
extern int tlsio_sl_open(CONCRETE_IO_HANDLE tls_io, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context);
extern int tlsio_sl_close(CONCRETE_IO_HANDLE tls_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context);
extern int tlsio_sl_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context);
extern void tlsio_sl_dowork(CONCRETE_IO_HANDLE tls_io);
extern int tlsio_sl_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value);
extern OPTIONHANDLER_HANDLE tlsio_sl_retrieveoptions(CONCRETE_IO_HANDLE handle);

extern const IO_INTERFACE_DESCRIPTION* tlsio_sl_get_interface_description(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TLSIO_SL_H */
