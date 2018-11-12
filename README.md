Run a simple C sample on a SimpleLink device
===
---
# Table of Contents

- [Introduction](#Introduction)
- [Step 1: Prerequisites](#Step-1-Prerequisites)
- [Step 2: Build and Run the Sample](#Step-2-Build)
-   [DPS support](#DPS)
-   [Next Steps](#NextSteps)

<a name="Introduction"></a>
# Introduction

**About this document**

This document describes how to connect a TI SimpleLink device to Microsoft Azure using the Azure IoT SDK. This multi-step process includes:
- Configuring Azure IoT Hub
- Registering your IoT device
- Build and deploy Azure IoT SDK on device

The instructions in this file apply specifically to a user who has directly cloned or downloaded the "azure-iot-pal-simplelink" PAL repository. Please note that it is highly recommended to alternatively install the [TI SimpleLink SDK Plugin for Microsoft Azure](http://www.ti.com/tool/SIMPLELINK-SDK-PLUGIN-FOR-AZUREIOT), versus cloning or downloading this repository directly.  The repository by itself only provides a basic example running on TI-RTOS with makefiles for the TI compiler.  Full support for TI, GCC and IAR toolchains and FreeRTOS is provided via the Plugin installation.

If you have installed the Azure SDK Plugin (i.e. did not just clone/download this repository), then please refer to the Quick Start Guide that is included with your Plugin installation (you should be able to access this via the "docs" folder of the Azure SDK Plugin installation). The Quick Start Guide provides more accurate instructions that apply specifically to the Plugin installation.

<a name="Step-1-Prerequisites"></a>
# Step 1: Prerequisites

- Computer with Git client installed and access to [azure-iot-pal-simplelink](https://github.com/TexasInstruments/azure-iot-pal-simplelink), i.e. this BitBucket public repository.
- [CC3220SF LaunchPad](http://www.ti.com/tool/cc3220sf-launchxl) or [CC3220S LaunchPad](http://www.ti.com/tool/cc3220s-launchxl) or [MSP432E4 LaunchPad](http://www.ti.com/tool/MSP-EXP432E401Y)
- [Setup your IoT hub](https://catalog.azureiotsuite.com/docs?title=Azure/azure-iot-device-ecosystem/setup_iothub)
- [Provision your device and get its credentials](https://catalog.azureiotsuite.com/docs?title=Azure/azure-iot-device-ecosystem/manage_iot_hub)

These instructions refer to the folder that contains the Azure PAL repository on your local machine as `<AZURE_PAL_INSTALL_DIR>`.

While not strictly required, we recommend that you install the following tools from TI in the same directory and that you use directory names without any whitespace. This documentation assumes that you install everything in `C:\ti`.

- Install [Code Composer Studio (CCS) IDE, v8.2 or compatible](http://processors.wiki.ti.com/index.php/Download_CCS)

- Install [TI SimpleLink Wi-Fi CC32XX Software Development Kit 2.30 or later](http://www.ti.com/tool/simplelink-cc3220-sdk) (for SimpleLink CC32XX only)

- Install [TI SimpleLink MSP432E4 Software Development Kit 2.30 or later](http://www.ti.com/tool/simplelink-msp432-sdk) (for SimpleLink MSP432E4 only)

Please ensure that your device has been updated with the latest firmware and or service pack (instructions for updating the firmware and/or service pack are included with the SimpleLink SDK installation if applicable).

<a name="Step-2-Build"></a>
# Step 2: Build and Run the sample

<a name="Build-LIBS"></a>
## Build the Azure PAL and SDK libraries
1. Edit the `products.mak` file in `<AZURE_PAL_INSTALL_DIR>\build_all` using your favorite text editor. The variables `XDC_INSTALL_DIR` and `SIMPLELINK_<YOUR DEVICE>_SDK_INSTALL_DIR` must point to the locations where you installed these products. The variable `ti.targets.arm.elf.M4` for CC32xx or `ti.targets.arm.elf.M4F` for MSP432E4 should point to the installation location of the TI ARM compiler, which can be found in CCS. After modification, these variable definitions should look similar to the following. Note the use of "/" in the path.

  ```
  XDC_INSTALL_DIR ?= c:/ti/xdctools_3_50_08_24_core
  SIMPLELINK_CC32XX_SDK_INSTALL_DIR   ?= C:/ti/simplelink_cc32xx_sdk_2_30_00_05
  ti.targets.arm.elf.M4  ?= C:/CCSv8.2.0/ccsv8/tools/compiler/ti-cgt-arm_18.1.3.LTS
  ```
It is also recommended that you add the xdc tools folder (```C:/ti/xdctools_3_50_08_24_core```) to your path in order to avoid errors related to finding `gmake.exe` during the build process.

2. Open a Windows command prompt.

3. In the Windows command prompt, run the following commands (be sure to replace the paths with your installation paths).

  ```
  cd <AZURE_PAL_INSTALL_DIR>\build_all
  C:\ti\xdctools_3_50_08_24_core\gmake.exe clean
  C:\ti\xdctools_3_50_08_24_core\gmake.exe all
  ```

<a name="Build-OS"></a>
## Build the OS
1. Update the settings in the `imports.mak` file of your device's SDK in `<SIMPLELINK_CC32XX_SDK_INSTALL_DIR>` or `<SIMPLELINK_MSP432E4_SDK_INSTALL_DIR>` with the appropriate paths if you have not done so previously.

2. In the Windows command prompt, navigate to the OS directory corresponding to your OS of choice. For example, `<SIMPLELINK_CC32XX_SDK_INSTALL_DIR>/kernel/tirtos/builds/CC3220SF_LAUNCHXL/release/ccs` for tirtos on CC3220SF.

3. In the Windows command prompt, enter the following commands to build the OS:

  ```
  C:\ti\xdctools_3_50_08_24_core\gmake.exe clean
  C:\ti\xdctools_3_50_08_24_core\gmake.exe
  ```

<a name="Build-SAMPLE"></a>
## Build a sample application
The following instructions refer to the simplesample_http sample application in this PAL repository.
Before building the application, complete the following steps:

1. Open the `simplesample_http.c` file from the directory `<AZURE_PAL_INSTALL_DIR>\sample` in a text editor and replace the value of the "connectionString" variable with the device connection string you noted [earlier](#Step-1-Prerequisites).

2. (For CC32xx only) Open the file `CC3220S_LAUNCHXL/wificonfig.h` or `CC3220SF_LAUNCHXL/wificonfig.h` depending on the LaunchPad you have. Search for "USER STEP" and update the WIFI SSID and SECURITY_KEY macros.

3. In the Windows command prompt, enter the following commands to build the application (replace `CC3220SF_LAUNCHXL` with your platform name):

  ```
  cd <AZURE_PAL_INSTALL_DIR>\sample\CC3220SF_LAUNCHXL\tirtos\ccs
  C:\ti\xdctools_3_50_08_24_core\gmake.exe clean
  C:\ti\xdctools_3_50_08_24_core\gmake.exe all
  ```

<a name="Flash-SAMPLE"></a>
## Flash the root certificate
### For CC32xx devices

> Note: In the sample applications, the root CA certificate - "Baltimore CyberTrust Root" is flashed to CC32XX LaunchPad to the location `/cert/ms.pem`. This location must match `SL_SSL_CA_CERT` in `<AZURE_PAL_INSTALL_DIR>\pal\inc\cert_sl.h`, and is used by SimpleLink TLS stack.

Here's why you need the Baltimore root CA - it's the root CA for `*.azure-devices.net`, the IoT Hub endpoint and it's the only way for the device to verify the chain of trust:

![image](https://cloud.githubusercontent.com/assets/6472374/11576321/71207be4-9a1e-11e5-9332-fa99fdbd31f9.png)

The default behavior of the sample application is to automatically write the certificate to flash if it does not exist. This prevents the file from being written unnecessarily to the file system each time the program is run. If you wish to override this behavior, or if you wish to use a new certificate file, you can force the certificate to be flashed by passing "-DOVERWRITE_CERTS" to the compiler in the makefile.

### For MSP432E4 devices

The root CA certificate is simply loaded into memory as a security object with the name `/cert/ms.pem` to match `SL_SSL_CA_CERT` in `<AZURE_PAL_INSTALL_DIR>\pal\inc\cert_sl.h`, and used later when connecting to the IoT Hub endpoint.

<a name="Setup-CCS"></a>
## Setting Up Code Composer Studio Before Running The Examples
We show the procedure for CC32xx devices below, but the same procedure can be followed for MSP432E4 by replacing `CC32xx` with `MSP432E4`.

1. Plug the LaunchPad into a USB port on your PC

2. Open Code Composer Studio.

3. In Code Composer Studio, open the CCS Debug Perspective - Windows menu -> Open Perspective -> CCS Debug

4. Open the Target Configurations View - Windows menu -> Show View -> Target Configurations

5. Right-click on User Defined. Select New Target Configuration.

6. Use `CC32xx.ccxml` as "File name". Hit Finish.

7. In the Basic window, select "Texas Instruments XDS110 USB Debug Probe" as the "Connection", and then type "CC3220" in the "Board or Device" text field. Check the box next to "CC3220SF" or "CC3220S" (depending on your LaunchPad). Hit Save.

8. Right-click "CC32xx.ccxml" in the Target Configurations View. Hit Launch Selected Configuration.

9. Under the Debug View, right-click on "Texas Instruments XDS110 USB Debug Probe_0/Cortex_M4_0". Select "Connect Target".

<a name="Run-SAMPLE"></a>
## Running An Example
1. Disconnect and reconnect the LaunchPad's USB cable to power cycle the hardware, and then reconnect in CCS.

2. Select Run menu -> Load -> Load Program..., and browse to the file `simplesample_http.out` in `<AZURE_PAL_INSTALL_DIR>\sample\CC3220SF_LAUNCHXL\tirtos\ccs` (replace `CC3220SF_LAUNCHXL` depending on your board). Hit OK. This will load the program onto the board.

3. Run the application by pressing F8. The output will appear in your CCS console window, similar to the following:

```
Starting the simplesample_http example
Current time: Fri Jul 20 20:56:27 2018

CC32XX has connected to AP and acquired an IP address.
IP Address: 192.168.1.172
Flashing certificate file ...successfully wrote file /cert/ms.pem to flash
IoTHubClient accepted the message for delivery
Message Id: 1 Received.
Result Call Back Called! Result is: IOTHUB_CLIENT_CONFIRMATION_OK
```

The [Device Explorer](https://github.com/Azure/azure-iot-sdk-csharp/tree/master/tools/DeviceExplorer) can be used to monitor the data sent by the application. Under the "Data" tab in Device Explorer, "Monitor" option should be selected before running the application. Later when the application is run, a message similar to the following message is displayed on "Event Hub Data" window.

```
5/19/2017 5:26:44 PM> Device: [devicee43522cd755d463b9331e55d1cab2f13], Data:[{"DeviceId":"myFirstDevice", "WindSpeed":12, "Temperature":23.000000, "Humidity":68.000000}]Properties:'temperatureAlert': 'false'
```

<a name="DPS"></a>
# Device Provisioning Service support
Support for Device Provisioning Service (DPS) can be enabled in the libraries by setting `use_prov_client` to `true` in the `products.mak` file, and rebuilding the libraries. However, enabling it implies the libraries expect DPS to be used at the application level. As a result, it causes applications that are not using DPS to fail to build, including the simplesample_http sample application in this PAL repository, given it does not implement the HSM interface required by DPS.

If you are interested in turning on DPS support, it is recommended to download and install the [TI SimpleLink SDK Plugin for Microsoft Azure](http://www.ti.com/tool/SIMPLELINK-SDK-PLUGIN-FOR-AZUREIOT). There is an example (`prov_dev_client_ll_sample`) in the Plugin that uses DPS and would be a more suitable starting point.

<a name="NextSteps"></a>
# Next Steps

You have now learned how to run a sample application that collects sensor data and sends it to your IoT hub. To explore how to store, analyze and visualize the data from this application in Azure using a variety of different services, please click on the following lessons:

-   [Manage cloud device messaging with iothub-explorer]
-   [Save IoT Hub messages to Azure data storage]
-   [Use Power BI to visualize real-time sensor data from Azure IoT Hub]
-   [Use Azure Web Apps to visualize real-time sensor data from Azure IoT Hub]
-   [Weather forecast using the sensor data from your IoT hub in Azure Machine Learning]
-   [Remote monitoring and notifications with Logic Apps]

[Manage cloud device messaging with iothub-explorer]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-explorer-cloud-device-messaging
[Save IoT Hub messages to Azure data storage]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-store-data-in-azure-table-storage
[Use Power BI to visualize real-time sensor data from Azure IoT Hub]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-live-data-visualization-in-power-bi
[Use Azure Web Apps to visualize real-time sensor data from Azure IoT Hub]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-live-data-visualization-in-web-apps
[Weather forecast using the sensor data from your IoT hub in Azure Machine Learning]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-weather-forecast-machine-learning
[Remote monitoring and notifications with Logic Apps]: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-monitoring-notifications-with-azure-logic-apps
