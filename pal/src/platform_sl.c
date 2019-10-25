// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "tlsio_sl.h"
#include "azure_c_shared_utility/platform.h"

int platform_init(void)
{
    return 0;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
    return tlsio_sl_get_interface_description();
}

STRING_HANDLE platform_get_platform_info(PLATFORM_INFO_OPTION options)
{
    (void)options;

    return STRING_construct("(TI SimpleLink)");
}

void platform_deinit(void)
{
    return;
}
