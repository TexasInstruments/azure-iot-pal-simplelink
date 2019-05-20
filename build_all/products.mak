#
#  ======== products.mak ========
#

### Automatically use installation locations from plugin and SDK when possible
ifneq (,$(wildcard ../../../../imports.mak))
$(info Fetching installation locations from imports.mak...)
include ../../../../imports.mak
ifneq (,$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR))
ti.targets.arm.elf.M4  = $(CCS_ARMCOMPILER)
iar.targets.arm.M4     = $(IAR_ARMCOMPILER)
gnu.targets.arm.M4     = $(GCC_ARMCOMPILER)
endif
ifneq (,$(SIMPLELINK_MSP432E4_SDK_INSTALL_DIR))
ti.targets.arm.elf.M4F = $(CCS_ARMCOMPILER)
iar.targets.arm.M4F    = $(IAR_ARMCOMPILER)
gnu.targets.arm.M4F    = $(GCC_ARMCOMPILER)
endif
endif


################################################################
# When not part of the plugin, fill these out manually         #
################################################################
######################## All platforms ########################
XDC_INSTALL_DIR    ?=

######################## Optional ########################
FREERTOS_INSTALL_DIR   ?=

################################################################
# Fill in the variable set corresponding to your platform      #
# Only one set should be used at a time                        #
################################################################

##################### For CC32XX ####################
SIMPLELINK_CC32XX_SDK_INSTALL_DIR   ?=

# Toolchains: Leave assignment empty to disable a toolchain
ti.targets.arm.elf.M4  ?=
iar.targets.arm.M4     ?=
gnu.targets.arm.M4     ?=


#################### For MSP432E4 ###################
SIMPLELINK_MSP432E4_SDK_INSTALL_DIR   ?=

# Toolchains: Leave assignment empty to disable a toolchain
ti.targets.arm.elf.M4F ?=
iar.targets.arm.M4F    ?=
gnu.targets.arm.M4F    ?=
