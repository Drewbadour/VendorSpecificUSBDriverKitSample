//
//  XboxOneInterface.cpp
//  XboxControllerDriver
//

#include <os/log.h>
#include <DriverKit/DriverKit.h>
#include <USBDriverKit/USBDriverKit.h>
#include <HIDDriverKit/HIDDriverKit.h>

#include "XboxOneInterface.h"
#include "XboxOneInputInterface.h"

#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "XboxOne Core Interface - " fmt "\n", ##__VA_ARGS__)


#pragma mark - Interface Type Management

// NOTE(Drew): There's only one instance here, but there might be more in the future.
typedef enum : uint8_t
{
    XboxOneInterfaceTypeUnknown = 255,
    XboxOneInterfaceTypeInput = 0,
} XboxOneInterfaceType;


#pragma mark - Driver Lifecycle

struct XboxOneInterface_IVars
{
    XboxOneInterfaceType interfaceType = XboxOneInterfaceTypeUnknown;
    IOUSBHostInterface* interface = nullptr;
};

bool XboxOneInterface::init(void)
{
    bool result = false;

    Log(">> init()");

    result = super::init();
    if (result != true)
    {
        Log("init() - super::init failed.");
        goto Exit;
    }

    ivars = IONewZero(XboxOneInterface_IVars, 1);
    if (ivars == nullptr)
    {
        Log("init() - Failed to allocate memory for ivars.");
        goto Exit;
    }

    Log("<< init()");
    return true;

Exit:
    Log("<< init()");
    return false;
}

// NOTE(Drew): IOUserUSBHostHIDDevice implementers shouldn't override Start,
// But this is implemented for logging and additional pass-down.
kern_return_t XboxOneInterface::Start_Impl(IOService* provider)
{
    kern_return_t ret = kIOReturnSuccess;

    Log(">> Start()");

    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Failed super::Start with error: 0x%08x.", ret);
        goto Exit;
    }
/*
    if (ivars->handlingInterface == nullptr)
    {
        Log("Start() - Handling interface is null.");
        ret = kIOReturnNotFound;
        goto Exit;
    }

    ret = ((XboxOneInputInterface*)ivars->handlingInterface)->Start(provider);
    if (ret != kIOReturnSuccess)
    {
        Log("Start() - Handling interface Start failed with error: 0x%08x.", ret);
        goto Exit;
    }
*/
Exit:
    Log("<< Start()");
    return ret;
}

bool XboxOneInterface::handleStart(IOService* provider)
{
    bool result = false;
    kern_return_t ret = kIOReturnSuccess;

    const IOUSBConfigurationDescriptor* configurationDescriptor = nullptr;
    const IOUSBInterfaceDescriptor* interfaceDescriptor = nullptr;

    Log(">> handleStart()");

    ivars->interface = OSDynamicCast(IOUSBHostInterface, provider);
    if (ivars->interface == nullptr)
    {
        Log("handleStart() - Failed to cast provider to IOUSBHostInterface.");
        result = false;
        goto Exit;
    }

    // NOTE(Drew): The parent Start function will have already retained the interface.
    // NOTE(Drew): The parent Start function will have already called RegisterService.

    // TODO(Drew): Is the preceeding about RegisterService true?

    configurationDescriptor = ivars->interface->CopyConfigurationDescriptor();
    if (configurationDescriptor == nullptr)
    {
        Log("handleStart() - Failed to get configuration descriptor for interface.");
        result = false;
        goto Exit;
    }

    interfaceDescriptor = ivars->interface->GetInterfaceDescriptor(configurationDescriptor);
    if (interfaceDescriptor == nullptr)
    {
        Log("handleStart() - Failed to get interface descriptor for interface.");
        result = false;
        goto Exit;
    }

    Log("handleStart() - Interface number: %d", interfaceDescriptor->bInterfaceNumber);
    switch (interfaceDescriptor->bInterfaceNumber)
    {
        case XboxOneInterfaceTypeInput:
        {
            Log("A");
//            ivars->handlingInterface = (XboxOneInputInterface*)IOMallocZero(sizeof(XboxOneInputInterface));
            Log("B");
        } break;

        case XboxOneInterfaceTypeUnknown:
        default:
        {
            Log("handleStart() - Matched interface with unsupported bInterfaceNumber of %d. This type of interface is not supported.", interfaceDescriptor->bInterfaceNumber);
            result = false;
            goto Exit;
        } break;
    }
/*
    if (ivars->handlingInterface == nullptr)
    {
        Log("handleStart() - Failed to create a valid handling interface.");
        result = false;
        goto Exit;
    }
    Log("C");
    result = ivars->handlingInterface->init();
    if (result == false)
    {
        // NOTE(Drew): Handling interface should have logged already
        goto Exit;
    }
    Log("D");
 */
    result = super::handleStart(provider);
    if (result == false)
    {
        Log("handleStart() - super::handleStart() failed.");
        goto Exit;
    }

    // NOTE(Drew): super::handleStart will:
    // 1. Open the interface.
    // 2. Initialize the pipes.

//    result = ivars->handlingInterface->handleStart(provider);

Exit:
    if (configurationDescriptor != nullptr)
    {
        IOUSBHostFreeDescriptor(configurationDescriptor);
        configurationDescriptor = nullptr;
    }
    Log("<< handleStart()");
    return result;
}

kern_return_t XboxOneInterface::Stop_Impl(IOService* provider)
{
    kern_return_t ret = kIOReturnSuccess;

    Log(">> Stop()");

    // TODO(Drew): Other stop stuff goes here.

    // NOTE(Drew): handlingInterface being null here isn't necissarily an error, so log it and move on.
/*
    if (ivars->handlingInterface != nullptr)
    {
        ivars->handlingInterface->Stop(provider);
    }
    else
    {
        Log("Stop() - Handling interface was null.");
    }
*/
    ret = Stop(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess)
    {
        Log("Stop() - super::Stop failed.");
        goto Exit;
    }

Exit:
    Log("<< Stop()");
    return ret;
}

void XboxOneInterface::free(void)
{
    Log("free()");

    if (ivars != nullptr)
    {
/*
        if (ivars->handlingInterface != nullptr)
        {
            ivars->handlingInterface->free();
            ivars->handlingInterface = nullptr;
        }
 */
        // NOTE(Drew): interface is handled by IOUserUSBHostHIDDevice.
    }

    IOSafeDeleteNULL(ivars, XboxOneInterface_IVars, 1);

    super::free();
}
