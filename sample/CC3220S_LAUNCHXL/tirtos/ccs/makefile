#
#  Copyright (c) 2017-2018, Texas Instruments Incorporated
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  *  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#  *  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  *  Neither the name of Texas Instruments Incorporated nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

TREE_ROOT = ../../../..
include $(TREE_ROOT)/build_all/products.mak
include makedefs

CFLAGS :=  -I../.. -I../../.. -I$(TREE_ROOT)/sdk/inc/ -I$(TREE_ROOT)/sdk/certs/ -I$(TREE_ROOT)/sdk/c-utility/inc/ -I$(TREE_ROOT)/sdk/c-utility/inc/azure_c_shared_utility -I$(TREE_ROOT)/sdk/iothub_client/inc -I$(TREE_ROOT)/sdk/serializer/inc -g $(CFLAGS) --cmd_file=$(CONFIGPKG)/compiler.opt

LIBS =  $(TREE_ROOT)/build_all/sdk/lib/ccs/m4/common_sl_release.a \
    $(TREE_ROOT)/build_all/sdk/lib/ccs/m4/iotclient_sl_release.a \
    $(TREE_ROOT)/build_all/sdk/lib/ccs/m4/serializer_sl_release.a \
    $(TREE_ROOT)/build_all/pal/lib/ccs/m4/pal_sl_release.a

VPATH = ..:../..:../../..:$(TREE_ROOT)/certs

%.o : %.c

%.o : %.c $(CONFIGPKG)/compiler.opt
	@echo $(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -c $<

all: simplesample_http.out

$(CONFIGPKG)/linker.cmd $(CONFIGPKG)/compiler.opt:
	@ echo
	$(error "$(CONFIGPKG) is not built. You can build it by issuing \
            $(MAKE) in $(CONFIGPKG).")
	@ echo

simplesample_http.out: main_tirtos.o certs.o startsntp.o simplesample_http.o network.o CC3220S_LAUNCHXL.o CC3220S_LAUNCHXL_TIRTOS.cmd $(CONFIGPKG)/linker.cmd $(LIBS)
	@echo building $@ ..
	@$(LD) -o $@ $^ -x -m $@.map $(LFLAGS)

clean:
	@echo cleaning ..
	@ $(RM) *.o *.out *.map
	@ $(RMDIR)
