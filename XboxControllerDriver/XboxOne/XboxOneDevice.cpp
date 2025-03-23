//
//  XboxOneDevice.cpp
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// The driver for the USB device of an Xbox One controller.
// Since the Xbox One controller's USB device is "vendor specific" (has a bDeviceClass of 0xFF),
// the DriverKit matching rules require that a driver match to the USB device.
// This means that the `Info.plist` matching logic is for the device,
// And interface matching is executed in the device's driver code.
//
// Please do not consider this an exact template for other USB devices.
// Because this is a vendor-specific USB device that is being converted to a HID device,
// this code is actually quite abnormal.
// However, its abnormality can provide insight into a lot of DriverKit edge cases.
//

#include <os/log.h>

#include <DriverKit/DriverKit.h>
#include <USBDriverKit/USBDriverKit.h>

#include "XboxOneDevice.h"

#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "XboxOne Device - " fmt "\n", ##__VA_ARGS__)

#if DEBUG
#define TraceLog(fmt, ...) Log(fmt, ##__VA_ARGS__)
#else
#define TraceLog(fmt, ...)
#endif

#if DEBUG
#define DebugLog(fmt, ...) Log(fmt, ##__VA_ARGS__)
#else
#define DebugLog(fmt, ...)
#endif

constexpr uint8_t TARGET_CONFIGURATION = 1;




// MARK: - Driver Lifecycle

/// Stored variables of the Xbox One controller device
struct XboxOneDevice_IVars
{
};




// MARK: Driver Lifecycle - Startup

/// Initializer for the Xbox One controller device
bool XboxOneDevice::init(void)
{
	bool result = false;

	Log(">> init()");

	result = super::init();
	if (result != true)
	{
		Log("init() - super::init failed.");
		goto Exit;
	}

	ivars = IONewZero(XboxOneDevice_IVars, 1);
	if (ivars == nullptr)
	{
		Log("init() - Failed to allocate memory for ivars.");
		goto Exit;
	}

	Log("<< init()");
	return true;

Exit:
	return false;
}

/// Registration and initialization of the Xbox One controller device
kern_return_t XboxOneDevice::Start_Impl(IOService* provider)
{
	kern_return_t ret = kIOReturnSuccess;
	IOUSBHostDevice* device = nullptr;
	const IOUSBDeviceDescriptor* deviceDescriptor = nullptr;
	const IOUSBConfigurationDescriptor* configurationDescriptor = nullptr;

	Log(">> Start() - New");

	device = OSDynamicCast(IOUSBHostDevice, provider);
	if (device == nullptr)
	{
		Log("Start() - Failed to cast provider to IOUSBHostDevice.");
		ret = kIOReturnError;
		goto Exit;
	}

	deviceDescriptor = device->CopyDeviceDescriptor();
	if (deviceDescriptor == nullptr)
	{
		Log("Start() - Failed to get device descriptor.");
		goto Exit;
	}

	if (deviceDescriptor->bNumConfigurations < 1)
	{
		Log("Start() - Device has no configurations.");
		goto Exit;
	}

	ret = device->Open(this, 0, NULL);
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - Failed to open device with error: 0x%08x.", ret);
		goto Exit;
	}

	// You could iterate over all the configuration descriptors, but the controller interface is always at this index.
	configurationDescriptor = device->CopyConfigurationDescriptorWithValue(TARGET_CONFIGURATION);
	if (configurationDescriptor == nullptr)
	{
		Log("Start() - Device configuration descriptor is null.");
		goto Exit;
	}

	// Sets controller interface to active, so it can be controlled by the interface driver.
	// If it isn't set active, then no driver can match to it.
	// With this configuration set active, DriverKit will now match the interface based on the plist settings.
	ret = device->SetConfiguration(TARGET_CONFIGURATION, true);
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - Failed to set configuration on device with error: 0x%08x.", ret);
		goto Exit;
	}

	ret = Start(provider, SUPERDISPATCH);
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - Failed super::Start with error: 0x%08x.", ret);
		goto Exit;
	}

	// Since this is the device that is matched based on the plist, it needs to be registered.
	// Otherwise the OS will assume that start failed and move on to the next potential driver match.
	// Usually this would be handled by the superclass, but since this extends `IOService`,
	// it will need to register itself.
	ret = RegisterService();
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - Failed to register service with error: 0x%08x.", ret);
		goto Exit;
	}

	Log("<< Start()");
	ret = kIOReturnSuccess;

Exit:
	if (deviceDescriptor != nullptr)
	{
		IOUSBHostFreeDescriptor(deviceDescriptor);
	}

	if (configurationDescriptor != nullptr)
	{
		IOUSBHostFreeDescriptor(configurationDescriptor);
	}

	return ret;
}




// MARK: Driver Lifecycle - Shutdown

/// Shutdown of the Xbox One controller device
kern_return_t XboxOneDevice::Stop_Impl(IOService* provider)
{
	kern_return_t ret = kIOReturnSuccess;

	Log(">> Stop()");

	ret = Stop(provider, SUPERDISPATCH);
	if (ret != kIOReturnSuccess)
	{
		Log("Stop() - super::Stop failed with error: 0x%08x.", ret);
	}

	Log("<< Stop()");

	return ret;
}

/// Cleanup of the Xbox Controller device
void XboxOneDevice::free(void)
{
	Log("free()");

	if (ivars != nullptr)
	{
		// Unused
	}

	IOSafeDeleteNULL(ivars, XboxOneDevice_IVars, 1);

	super::free();
}
