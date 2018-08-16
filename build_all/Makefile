#
# ==== Makefile ====
#

#
# ==== Standard Variables ====
#

include ./products.mak

#
# Set XDCOPTIONS.  Use -v for a verbose build.
#
#XDCOPTIONS=v
export XDCOPTIONS

# Set XDCARGS to some of the variables above.  XDCARGS are passed
# to the XDC build engine... which will load azureiot.bld... which will
# extract these variables and use them to determine what to build and which
# toolchains to use.
XDCARGS= \
    CC32XXSDK=\"$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)\" \
    MSP432E4SDK=\"$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)\" \
    BIOS=\"$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos\" \
    USE_PROV=\"$(use_prov_client)\" \
    ti.targets.arm.elf.M4=\"$(ti.targets.arm.elf.M4)\" \
    iar.targets.arm.M4=\"$(iar.targets.arm.M4)\" \
    gnu.targets.arm.M4=\"$(gnu.targets.arm.M4)\" \
    ti.targets.arm.elf.M4F=\"$(ti.targets.arm.elf.M4F)\" \
    iar.targets.arm.M4F=\"$(iar.targets.arm.M4F)\" \
    gnu.targets.arm.M4F=\"$(gnu.targets.arm.M4F)\"

# Set XDCPATH to contain necessary repositories.
ifneq ($(SIMPLELINK_CC32XX_SDK_INSTALL_DIR),)
XDCPATH = $(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos/packages;
else
XDCPATH = $(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR)/kernel/tirtos/packages;
endif

ifeq (,$(wildcard $(subst ;,,$(XDCPATH))))
    $(error $(subst ;,,$(XDCPATH)) is not a valid directory. \
            Please verify the path to the SDK is correct in products.mak.)
endif
export XDCPATH

#
# Set XDC executable command
# Note that XDCBUILDCFG points to the .bld file which uses
# the arguments specified by XDCARGS
#
XDC = $(XDC_INSTALL_DIR)/xdc $(JOBS) XDCARGS="$(XDCARGS)" XDCBUILDCFG=./azureiot.bld
XS = $(XDC_INSTALL_DIR)/xs

######################################################
## Shouldnt have to modify anything below this line ##
######################################################

all:
	@echo building packages...
	@$(XDC) -Pr ..

clean:
	@echo cleaning packages ...
	@$(XDC) clean -Pr ..
