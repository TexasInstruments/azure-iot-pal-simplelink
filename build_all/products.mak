#
#  ======== products.mak ========
#

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
