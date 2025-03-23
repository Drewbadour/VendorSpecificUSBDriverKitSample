//
//  XboxOneInputInterface.cpp
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// The driver for the "controller" USB interface of an Xbox One controller.
// An Xbox One controller advertises multiple interfaces
// where different interface drivers would be necessary to handle their individual functions.
// For the sake of this example, only the "controller" interface is provided.
//
// Please do not consider this an exact template for other USB devices.
// Because this is a vendor-specific USB device that is being converted to a HID device,
// this code is actually quite abnormal.
// However, its abnormality can provide insight into a lot of DriverKit edge cases.
//

#include <os/log.h>
#include <DriverKit/DriverKit.h>
#include <USBDriverKit/USBDriverKit.h>
#include <HIDDriverKit/HIDDriverKit.h>

#include <HIDConstants.h>
#include "XboxOneInputInterface.h"
#include "XboxOneInputPackets.h"
#include "XboxOneUserClient.h"

namespace XboxOne {
#include "XboxOneDescriptors.h"
}

#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "XboxOne Input Interface - " fmt "\n", ##__VA_ARGS__)

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

#if DEBUG
void DebugPrintButtonPacket(const uint8_t* data)
{
	Log("HandleControllerReport() - "
		"[ 0x%x 0x%x 0x%x 0x%x ] "
		"Button: [ 0x%x 0x%x ] "
		"TrigL: [ 0x%x 0x%x ] "
		"TrigR: [ 0x%x 0x%x ] "
		"LeftX: [ 0x%x 0x%x ] "
		"LeftY: [ 0x%x 0x%x ] "
		"RightX: [ 0x%x 0x%x ] "
		"RightY: [ 0x%x 0x%x ]",
		data[0], data[1], data[2], data[3],
		data[4], data[5],
		data[6], data[7],
		data[8], data[9],
		data[10], data[11],
		data[12], data[13],
		data[14], data[15],
		data[16], data[17]
	);
}
#else
#define DebugPrintButtonPacket(data)
#endif




// MARK: - Driver Lifecycle

constexpr uint8_t INIT_PACKET[] = { 0x05, 0x20, 0x00, 0x01, 0x00 };
constexpr uint8_t INIT_PACKET_SIZE = sizeof(INIT_PACKET);
constexpr uint8_t SWAP_TO_WIRED_PACKET[] = { 0x05, 0x20, 0x00, 0x0f, 0x06 };
constexpr uint8_t SWAP_TO_WIRED_PACKET_SIZE = sizeof(SWAP_TO_WIRED_PACKET);

/// Stored variables of the Xbox One controller interface
struct XboxOneInputInterface_IVars
{
	/// The handle to the controller USB interface.
	IOUSBHostInterface* interface;

	/// The USB configuration descriptor provided by the Xbox One controller.
	const IOUSBConfigurationDescriptor* configurationDescriptor;
	/// The USB interface descriptor provided by the Xbox One controller.
	const IOUSBInterfaceDescriptor* interfaceDescriptor;

	/// Objects related to the pipes sending data from the Xbox One controller to the Apple device.
	usb_pipe_data inPipe;
	/// Objects related to the pipes sending data from the Apple device to the Xbox One controller.
	usb_pipe_data outPipe;
	/// Function pointer to the data callback `GotData_Impl`.
	OSAction* gotDataAction;

	/// Incrementing counter important for Xbox One controller-specific behavior.
	uint8_t outCounter;
	/// Whether on not the driver should send packets onward. This is controlled via the user client.
	bool enabled;
};




// MARK: Driver Lifecycle - Startup

/// Initializer for the Xbox One controller interface
bool XboxOneInputInterface::init(void)
{
	bool result = false;

	TraceLog(">> init()");

	result = super::init();
	if (result != true)
	{
		Log("init() - super::init failed.");
		goto Exit;
	}

	ivars = IONewZero(XboxOneInputInterface_IVars, 1);
	if (ivars == nullptr)
	{
		Log("init() - Failed to allocate memory for ivars.");
		goto Exit;
	}

	ivars->enabled = true;

	TraceLog("<< init()");
	return true;

Exit:
	TraceLog("<< init()");
	return false;
}

/// Startup of the Xbox One controller interface
kern_return_t XboxOneInputInterface::Start_Impl(IOService* provider)
{
	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> Start()");

	ivars->interface = OSDynamicCast(IOUSBHostInterface, provider);
	if (ivars->interface == nullptr)
	{
		Log("Start() - Failed to cast provider to IOUSBHostInterface.");
		goto Exit;
	}

	ret = ivars->interface->Open(this, 0, 0);
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - Failed to open provider with error: 0x%08x.", ret);
		goto Exit;
	}

	ret = Start(provider, SUPERDISPATCH);
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - super::Start failed.");
		goto Exit;
	}

Exit:
	TraceLog("<< Start()");
	return ret;
}

/// Collects the configuration and interface descriptors for the Xbox One controller interface
inline bool XboxOneInputInterface::InitDescriptors(void)
{
	TraceLog(">> initDescriptors()");

	ivars->configurationDescriptor = ivars->interface->CopyConfigurationDescriptor();
	if (ivars->configurationDescriptor == nullptr)
	{
		Log("initDescriptors() - Failed to copy configuration descriptor.");
		goto Exit;
	}

	ivars->interfaceDescriptor = ivars->interface->GetInterfaceDescriptor(ivars->configurationDescriptor);
	if (ivars->interfaceDescriptor == nullptr)
	{
		Log("initDescriptors() - Failed to get interface descriptor.");
		goto Exit;
	}

	TraceLog("<< initDescriptors()");
	return true;

Exit:
	TraceLog("<< initDescriptors()");
	return false;
}

/// Finds the `IN` and `OUT` interrupt pipes and their descriptors.
inline bool XboxOneInputInterface::InitPipes(void)
{
	kern_return_t ret = kIOReturnSuccess;
	const IOUSBEndpointDescriptor* endpointDescriptor = nullptr;

	TraceLog(">> initPipes()");

	while ((endpointDescriptor = IOUSBGetNextEndpointDescriptor(ivars->configurationDescriptor, ivars->interfaceDescriptor, (const IOUSBDescriptorHeader*)endpointDescriptor)) != NULL)
	{
		IOUSBHostPipe** pipe = nullptr;

		uint_fast8_t endpointType = IOUSBGetEndpointType(endpointDescriptor);
		if (endpointType != kIOUSBEndpointTypeInterrupt)
		{
			DebugLog("initPipes() - Found non-interrupt pipe type of %d", endpointType);
			continue;
		}

		uint_fast8_t endpointDirection = IOUSBGetEndpointDirection(endpointDescriptor);
		if (endpointDirection == kIOUSBEndpointDirectionIn)
		{
			pipe = &(ivars->inPipe.pipe);
			ivars->inPipe.descriptor = endpointDescriptor;
		}
		else if (endpointDirection == kIOUSBEndpointDirectionOut)
		{
			pipe = &(ivars->outPipe.pipe);
			ivars->outPipe.descriptor = endpointDescriptor;
		}
		else
		{
			DebugLog("initPipes() - Got unexpected direction %d", endpointDirection);
			continue;
		}

		uint_fast8_t address = IOUSBGetEndpointAddress(endpointDescriptor);
		ret = ivars->interface->CopyPipe(address, pipe);
		if (ret != kIOReturnSuccess)
		{
			Log("Failed to copy pipe at address %d with error 0x%08x.", address, ret);
			return false;
		}

		if (ivars->inPipe.pipe != nullptr && ivars->outPipe.pipe != nullptr)
		{
			return true;
		}
	}

	TraceLog("<< initPipes()");
	return false;
}

/// Collects all of the relevant data for a pipe into the passed data.
inline bool XboxOneInputInterface::SetupPipe(usb_pipe_data* pipeData)
{
	kern_return_t ret = kIOReturnSuccess;
	uint64_t address = 0;

	TraceLog(">> setupPipe()");

	ret = pipeData->pipe->GetSpeed(&pipeData->speed);
	if (ret != kIOReturnSuccess)
	{
		Log("setupPipe() - Failed to get pipe speed with error: 0x%08x.", ret);
		return false;
	}

	pipeData->interval = IOUSBGetEndpointIntervalFrames(pipeData->speed, pipeData->descriptor);
	if (pipeData->interval == 0)
	{
		Log("setupPipe() - Failed to get pipe interval.");
		return false;
	}

	pipeData->maxPacketSize = IOUSBGetEndpointMaxPacketSize(pipeData->speed, pipeData->descriptor);
	if (pipeData->maxPacketSize == 0)
	{
		Log("setupPipe() - Failed to get pipe max packet size.");
		return false;
	}

	ret = ivars->interface->CreateIOBuffer(kIOMemoryDirectionInOut, pipeData->maxPacketSize, &pipeData->memory.buffer);
	if (ret != kIOReturnSuccess)
	{
		Log("setupPipe() - Failed to create buffer with error: 0x%08x.", ret);
		return false;
	}

	ret = (pipeData->memory.buffer)->Map(0, 0, 0, 0, &address, &pipeData->memory.length);
	if (ret != kIOReturnSuccess)
	{
		Log("setupPipe() - Failed to create buffer with error: 0x%08x.", ret);
		return false;
	}

	pipeData->memory.address = (uint8_t*)address;

	TraceLog("<< setupPipe()");
	return true;
}

/// Collects all of the relevant data for both pipes into the `ivars`.
inline bool XboxOneInputInterface::SetupPipes(void)
{
	bool result = false;
	kern_return_t ret = kIOReturnSuccess;
	OSDictionary* properties = nullptr;

	TraceLog(">> setupPipes()");

	ret = CopyProperties(&properties);
	if (ret != kIOReturnSuccess)
	{
		Log("setupPipes() - Failed to copy properties with error: 0x%08x.", ret);
		goto Exit;
	}

	result = SetupPipe(&ivars->inPipe);
	if (result == false)
	{
		Log("setupPipes() - Failed to setup input pipe.");
		goto Exit;
	}
	ivars->inPipe.reportSize = OSDictionaryGetUInt64Value(properties, kIOHIDMaxInputReportSizeKey);

	result = SetupPipe(&ivars->outPipe);
	if (result == false)
	{
		Log("setupPipes() - Failed to setup output pipe.");
		goto Exit;
	}
	ivars->inPipe.reportSize = OSDictionaryGetUInt64Value(properties, kIOHIDMaxOutputReportSizeKey);

	OSSafeReleaseNULL(properties);
	TraceLog("<< setupPipes()");
	return true;

Exit:
	OSSafeReleaseNULL(properties);
	TraceLog("<< setupPipes()");
	return false;
}

/// Called by DriverKit on startup of the driver, due to being a subclass of `IOUserHIDDevice`.
///
/// This is called toward the end for `Start_Impl` of `IOUserHIDDevice`. So it can be used to do final initialization after the rest of the driver is initialized.
bool XboxOneInputInterface::handleStart(IOService* provider)
{
	bool result = false;
	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> handleStart()");

	result = super::handleStart(provider);
	if (result == false)
	{
		Log("handleStart() - super::handleStart() failed.");
		goto Exit;
	}

	result = InitDescriptors();
	if (result == false)
	{
		Log("handleStart() - Failed to init descriptors.");
		goto Exit;
	}

	result = InitPipes();
	if (result == false)
	{
		Log("handleStart() - Failed to init pipes.");
		goto Exit;
	}

	result = SetupPipes();
	if (result == false)
	{
		Log("handleStart() - Failed to setup pipes.");
		goto Exit;
	}

	// This is a generated function name.
	// When `TYPE(IOUSBHostPipe::CompleteAsyncIO)` is added to a function in the `.iig`, this function will be generated.
	// Some Apple documentation that references this behavior can be found here: https://developer.apple.com/documentation/driverkit/type
	ret = CreateActionGotData(ivars->inPipe.memory.length, &(ivars->gotDataAction));
	if (ret != kIOReturnSuccess)
	{
		Log("handleStart() - Failed to establish callback object for receiving data with error: 0x%08x.", ret);
		goto Exit;
	}

	// This is specific to the Xbox One controller, which requires special packets to start up and send data.
	SendInterruptData(INIT_PACKET, INIT_PACKET_SIZE);
	SendInterruptData(SWAP_TO_WIRED_PACKET, SWAP_TO_WIRED_PACKET_SIZE);

	// Starts listening for USB packets.
	RequestAsyncInterruptData();

	ivars->interface->retain();

	TraceLog("<< handleStart()");
	return true;

Exit:
	TraceLog("<< handleStart()");
	return false;
}




// MARK: Driver Lifecycle - Shutdown

/// Shutdown of the Xbox One controller interface
kern_return_t XboxOneInputInterface::Stop_Impl(IOService* provider)
{
	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> Stop()");

	// If there's somehow nothing to cancel, "Stop" quickly and exit.
	if (ivars->gotDataAction == nullptr)
	{
		ret = Stop(provider, SUPERDISPATCH);
		if (ret != kIOReturnSuccess)
		{
			Log("Stop() - super::Stop failed with error: 0x%08x.", ret);
		}

		TraceLog("<< Stop()");

		return ret;
	}
	// Otherwise, wait for some Cancels to get completed.



	// Retain the driver instance and the provider so the finalization can properly stop the driver
	this->retain();
	provider->retain();

	void (^finalize)(void) = ^{

		kern_return_t status = Stop(provider, SUPERDISPATCH);
		if (status != kIOReturnSuccess)
		{
			Log("Stop() - super::Stop failed with error: 0x%08x.", status);
		}

		TraceLog("<< Stop()");

		this->release();
		provider->release();
	};
	ivars->gotDataAction->Cancel(finalize);

	DebugLog("Stop() - Cancels started, they will stop the dext later.");

	return ret;
}

/// Cleanup of the Xbox Controller interface
void XboxOneInputInterface::free(void)
{
	TraceLog("free()");

	if (ivars != nullptr)
	{
		if (ivars->configurationDescriptor != nullptr)
		{
			IOUSBHostFreeDescriptor(ivars->configurationDescriptor);
		}

		// NOTE: interfaceDescriptor is a `get`, not a `copy`, so doesn't need to be freed.

		OSSafeReleaseNULL(ivars->inPipe.pipe);
		OSSafeReleaseNULL(ivars->inPipe.memory.buffer);
		OSSafeReleaseNULL(ivars->outPipe.pipe);
		OSSafeReleaseNULL(ivars->outPipe.memory.buffer);

		// NOTE: Pipe descriptors are a 'get', not a 'copy', so don't need to be freed.

		OSSafeReleaseNULL(ivars->gotDataAction);
		OSSafeReleaseNULL(ivars->interface);
	}

	IOSafeDeleteNULL(ivars, XboxOneInputInterface_IVars, 1);

	super::free();
}




// MARK: - HID Lifecycle

/// Helper function that creates an OSString for a particular descriptor index and language.
OSString* XboxOneInputInterface::CopyStringAtIndex(uint8_t descriptorIndex, uint16_t descriptorLanguage)
{
	const uint8_t descriptorHeaderSize = sizeof(IOUSBDescriptorHeader);
	const IOUSBStringDescriptor* descriptor = nullptr;
	OSString* string = nullptr;
	uint_fast8_t index = 0;
	char result[256] = {};

	TraceLog(">> CopyStringAtIndex()");

	if (descriptorIndex == 0)
	{
		DebugLog("CopyStringAtIndex() - Asked for index 0.");
		goto Exit;
	}

	descriptor = ivars->interface->CopyStringDescriptor(descriptorIndex, descriptorLanguage);
	if (descriptor == nullptr)
	{
		Log("CopyStringAtIndex() - Failed to copy string at index %d with language %d.", descriptorIndex, descriptorLanguage);
		goto Exit;
	}

	if (descriptor->bLength < descriptorHeaderSize)
	{
		Log("CopyStringAtIndex() - Descriptor bLength invalid %d.", descriptor->bLength);
		goto Exit;
	}


	{
		uint8_t length = (descriptor->bLength - descriptorHeaderSize) / 2;
		uint16_t* bString = (uint16_t*)descriptor->bString;
		for (; index < length; ++index)
		{
			uint16_t value = bString[index];
			if (value == 0)
			{
				break;
			}

			result[index] = (uint8_t)value;
		}

		string = OSStringCreate(result, index);
	}


Exit:
	if (descriptor != nullptr)
	{
		IOUSBHostFreeDescriptor(descriptor);
	}

	TraceLog("<< CopyStringAtIndex()");
	return string;
}

/// Override of the `newDeviceDescription` function from `IOUserHIDDevice`.
/// This is specific to the Xbox One controller, since it doesn't report a HID-compliant USB device description. So this function generates a new device description that is HID-compliant.
/// Most USB drivers shouldn't need to override this function.
OSDictionary* XboxOneInputInterface::newDeviceDescription(void)
{
	OSDictionary* dict = nullptr;
	OSDictionary* properties = nullptr;
	IOUSBHostDevice* device = nullptr;
	const IOUSBDeviceDescriptor* deviceDescriptor = nullptr;

	TraceLog(">> newDeviceDescription");

	if (ivars->interface == nullptr)
	{
		Log("newDeviceDescription() - Interface is null. Something has gone terribly wrong.");
		goto Exit;
	}

	ivars->interface->CopyProperties(&properties);
	if (properties == nullptr)
	{
		Log("newDeviceDescription() - Failed to copy interface properties.");
		goto Exit;
	}

	ivars->interface->CopyDevice(&device);
	if (device == nullptr)
	{
		Log("newDeviceDescription() - Failed to copy the device.");
		goto Exit;
	}

	deviceDescriptor = device->CopyDeviceDescriptor();
	if (deviceDescriptor == nullptr)
	{
		Log("newDeviceDescription() - Failed to copy device descriptor.");
		goto Exit;
	}

	// NOTE: This is saved to last in order to make cleanup easier.
	dict = OSDictionary::withCapacity(16);
	if (dict == nullptr)
	{
		Log("newDeviceDescription() - Failed to create OSDictionary.");
		goto Exit;
	}

	OSDictionarySetValue(dict, kIOHIDRegisterServiceKey, kOSBooleanTrue);
	OSDictionarySetValue(dict, "HIDDefaultBehavior", kOSBooleanTrue);
	OSDictionarySetValue(dict, "AppleVendorSupported", kOSBooleanTrue);

	OSDictionarySetUInt64Value(dict, kIOHIDReportIntervalKey, ivars->inPipe.interval);
	OSDictionarySetUInt64Value(dict, kIOHIDVendorIDKey, USBToHost16(deviceDescriptor->idVendor));
	OSDictionarySetUInt64Value(dict, kIOHIDProductIDKey, USBToHost16(deviceDescriptor->idProduct));
	OSDictionarySetStringValue(dict, kIOHIDTransportKey, "USB");
	OSDictionarySetUInt64Value(dict, kIOHIDVersionNumberKey, USBToHost16(deviceDescriptor->bcdDevice));
	OSDictionarySetUInt64Value(dict, kIOHIDCountryCodeKey, 0);
	OSDictionarySetUInt64Value(dict, kIOHIDRequestTimeoutKey, kUSBHostClassRequestCompletionTimeout * 1000);
	OSDictionarySetUInt64Value(dict, kIOHIDPrimaryUsagePageKey, XboxOne::ReportDescriptor[1]);
	OSDictionarySetUInt64Value(dict, kIOHIDPrimaryUsageKey, XboxOne::ReportDescriptor[3]);

	{
		OSObjectPtr value = OSDictionaryGetValue(properties, kUSBHostPropertyLocationID);
		if (value != nullptr)
		{
			OSDictionarySetValue(dict, kIOHIDLocationIDKey, value);
		}
	}
	{
		OSObjectPtr value = CopyStringAtIndex(deviceDescriptor->iManufacturer, kLanguageIDEnglishUS);
		if (value != nullptr)
		{
			OSDictionarySetValue(dict, kIOHIDManufacturerKey, value);
			OSSafeReleaseNULL(value);
		}
	}
	{
		OSObjectPtr value = CopyStringAtIndex(deviceDescriptor->iProduct, kLanguageIDEnglishUS);
		if (value != nullptr)
		{
			OSDictionarySetValue(dict, kIOHIDProductKey, value);
			OSSafeReleaseNULL(value);
		}
	}
	{
		OSObjectPtr value = CopyStringAtIndex(deviceDescriptor->iSerialNumber, kLanguageIDEnglishUS);
		if (value != nullptr)
		{
			OSDictionarySetValue(dict, kIOHIDSerialNumberKey, value);
			OSSafeReleaseNULL(value);
		}
	}
	{
		uint64_t portType = OSDictionaryGetUInt64Value(properties, kUSBHostMatchingPropertyPortType);
		if (portType == kIOUSBHostPortTypeInternal)
		{
			OSDictionarySetValue(dict, kIOHIDBuiltInKey, kOSBooleanTrue);
		}
	}

Exit:
	if (properties != nullptr)
	{
		properties->release();
		properties = nullptr;
	}
	if (deviceDescriptor != nullptr)
	{
		IOUSBHostFreeDescriptor(deviceDescriptor);
		deviceDescriptor = nullptr;
	}
	if (device != nullptr)
	{
		device->release();
		device = nullptr;
	}

	TraceLog("<< newDeviceDescription");
	return dict;
}

/// Override of the `newReportDescriptor` function from `IOUserHIDDevice`.
/// This is specific to the Xbox One controller, since it doesn't report a HID-compliant USB report descriptor. So this function generates a new report descriptor that is HID-compliant.
/// Most USB drivers shouldn't need to override this function.
OSData* XboxOneInputInterface::newReportDescriptor(void)
{
	TraceLog("newReportDescriptor");

	return OSData::withBytesNoCopy(XboxOne::ReportDescriptor, XboxOne::REPORT_DESCRIPTOR_SIZE);
}




// MARK: - Interface Communication
// MARK: Interface Communication - Data from Device

/// Queues a handler for incoming USB data
/// Calls whatever function is stored in the `gotDataAction` of the `ivars`.
kern_return_t XboxOneInputInterface::RequestAsyncInterruptData(void)
{
	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> RequestAsyncInterruptData()");

	ret = ivars->inPipe.pipe->AsyncIO(ivars->inPipe.memory.buffer, (uint32_t)ivars->inPipe.memory.length, ivars->gotDataAction, 0);
	if (ret != kIOReturnSuccess)
	{
		Log("RequestAsyncInterruptData() - Failed to request packets from the device with error: 0x%08x.", ret);
		goto Exit;
	}

Exit:
	TraceLog("<< RequestAsyncInterruptData()");
	return ret;
}


/// An example of generic USB packet handling.
/// Passes the packet on to `IOUserHIDDevice` via `handleReport`.
/// The OS will then treat the packets according to the HID report descriptor for that packet.
bool XboxOneInputInterface::HandleReportGeneric(void* data, uint32_t actualByteCount, uint64_t completionTimestamp, uint8_t packetType, uint8_t size)
{
	bool result = true;
	kern_return_t ret = kIOReturnSuccess;
	xboxone_report_header* header = (xboxone_report_header*)data;

	TraceLog(">> %{public}s", __PRETTY_FUNCTION__);

	if (header->packetType != packetType)
	{
		DebugLog("%{public}s - Packet type did not match expected type. Expected: %d, Actual: %d", __PRETTY_FUNCTION__, packetType, header->packetType);
		result = false;
		goto Exit;
	}

	if (header->size != size)
	{
		DebugLog("%{public}s - Header size did not match expected size. Expected: %d, Actual: %d", __PRETTY_FUNCTION__, size, header->size);
		result = false;
		goto Exit;
	}

	ret = handleReport(completionTimestamp, ivars->inPipe.memory.buffer, actualByteCount);
	if (ret != kIOReturnSuccess)
	{
		DebugLog("%{public}s - handleReport failed with error: 0x%08x.", __PRETTY_FUNCTION__, ret);
		result = false;
		goto Exit;
	}

Exit:
	TraceLog("<< %{public}s", __PRETTY_FUNCTION__);
	return result;
}

/// An example of generic USB packet handling.
/// Passes the packet on to `IOUserHIDDevice` via `HandleReportGeneric`.
bool XboxOneInputInterface::HandleControllerReport(void* data, uint32_t actualByteCount, uint64_t completionTimestamp)
{
	bool result = false;

	TraceLog(">> HandleControllerReport()");

	result = HandleReportGeneric(data, actualByteCount, completionTimestamp, XBOXONE_IN_BUTTON, XBOXONE_BUTTON_REPORT_SIZE);
	if (result == true)
	{
		DebugLog("HandleControllerReport() - Handled");
		DebugPrintButtonPacket(ivars->inPipe.memory.address);
	}

	TraceLog("<< HandleControllerReport()");

	return result;
}

/// Handles Xbox One controller "guide" button reports.
/// Generates a response packet to the "guide" button report, and sends it to the controller.
bool XboxOneInputInterface::HandleGuideReport(void* data, uint32_t actualByteCount, uint64_t completionTimestamp)
{
	bool result = false;

	TraceLog(">> HandleGuideReport()");

	result = HandleReportGeneric(data, actualByteCount, completionTimestamp, XBOXONE_IN_GUIDE, XBOXONE_GUIDE_REPORT_SIZE);
	if (result == true)
	{
		DebugLog("HandleGuideReport() - Handled");

		xboxone_guide_report* report = (xboxone_guide_report*)data;

		if (report->header.version == 0x30)
		{
			xboxone_guide_response response = {
				.header = {
					.packetType = 0x01,
					.version = 0x20,
					.counter = 0x00,
					.size = 0x09,
				},
				.constData = { 0x00, 0x07, 0x20, 0x02 },
				.padding = {},
			};

			SendInterruptData((const uint8_t*)(&response), XBOXONE_GUIDE_RESPONSE_SIZE);
		}
	}

	TraceLog("<< HandleGuideReport()");

	return result;
}

/// Called when input data received.
/// This only works because a read was established in `RequestAsyncInterruptData` and this function was established as a callback via `CreateActionGotData`.
void XboxOneInputInterface::GotData_Impl(OSAction* action, kern_return_t status, uint32_t actualByteCount, uint64_t completionTimestamp)
{
	(void)action;

	bool handled = false;
	xboxone_report_header* header = nullptr;

	TraceLog(">> GotData()");

	if (status != kIOReturnSuccess)
	{
		DebugLog("GotData() - Called with error: 0x%08x.", status);
		goto Exit;
	}

	if (ivars->enabled == false)
	{
		DebugLog("GotData() - Disabled, ignoring packet.");
		goto Exit;
	}

	header = (xboxone_report_header*)ivars->inPipe.memory.address;
	DebugLog("GotData() - packetType 0x%x, packetSize %d", header->packetType, header->size);

	handled = HandleControllerReport(header, actualByteCount, completionTimestamp);
	if (handled == true)
	{
		DebugLog("GotData() - Reported controller packet.");
		goto Exit;
	}

	handled = HandleGuideReport(header, actualByteCount, completionTimestamp);
	if (handled == true)
	{
		DebugLog("GotData() - Reported guide packet.");
		goto Exit;
	}

Exit:
	RequestAsyncInterruptData();
	TraceLog("<< GotData()");
}




// MARK: Interface Communication - Data to Device

/// Sends data on the `OUT` interrupt pipe to the Xbox One controller.
kern_return_t XboxOneInputInterface::SendInterruptData(const uint8_t* data, uint8_t size)
{
	kern_return_t ret = kIOReturnSuccess;
	uint32_t bytesTransferred = 0;

	TraceLog(">> SendInterruptData()");

	if (size > ivars->outPipe.memory.length)
	{
		// NOTE: This is a pretty cowardly thing to do. But its safe to assume that no packet requires more than one packet size.
		Log("SendInterruptData() - Size of requested packet (%d) is larger than the max packet size allowed for this pipe (%llu). Refusing to send packet.", size, ivars->outPipe.memory.length);
		return kIOReturnBadArgument;
	}

	// The Xbox One controller protocol includes a counter that is incremented every time a packet is sent to the Xbox One controller.
	// This code handles incrementing that counter.
	memcpy(ivars->outPipe.memory.address, data, size);
	ivars->outPipe.memory.address[2] = ivars->outCounter++;

	ret = ivars->outPipe.pipe->IO(ivars->outPipe.memory.buffer, size, &bytesTransferred, 0);
	if (ret != kIOReturnSuccess)
	{
		Log("SendInterruptData() - Failed to send init packet with error: 0x%08x.", ret);
		goto Exit;
	}

	if (bytesTransferred != size)
	{
		DebugLog("SendInterruptData() - Expected to send %d bytes, instead sent %d bytes.", size, (uint8_t)bytesTransferred);
	}
	DebugLog("SendInterruptData() - Transferred %u bytes.", bytesTransferred);

Exit:
	DebugLog("SendInterruptData() - Result of 0x%08x.", ret);
	TraceLog("<< SendInterruptData()");
	return ret;
}




// MARK: - UserClient Communication

/// Called by DriverKit when a new UserClient connects to the driver.
kern_return_t XboxOneInputInterface::NewUserClient_Impl(uint32_t type, IOUserClient** userClient)
{
	(void)type;

	kern_return_t ret = kIOReturnSuccess;
	IOService* client = nullptr;

	TraceLog(">> NewUserClient()");

	ret = Create(this, "UserClientProperties", &client);
	if (ret != kIOReturnSuccess)
	{
		Log("NewUserClient() - Failed to create UserClientProperties with error: 0x%08x.", ret);
		goto Exit;
	}

	*userClient = OSDynamicCast(XboxOneUserClient, client);
	if (*userClient == NULL)
	{
		Log("NewUserClient() - Failed to cast new client.");
		client->release();
		ret = kIOReturnError;
		goto Exit;
	}

	// Gives the user client a pointer back to this object.
	((XboxOneUserClient*)(*userClient))->SetInputInterface(this);

	TraceLog("<< NewUserClient()");

Exit:
	return ret;
}

/// A function available the user client that can enable or disable the driver.
/// See its use in `XboxOneUserClient`.
void XboxOneInputInterface::SetEnable(bool enabled)
{
	TraceLog(">> SetEnable()");

	if (ivars != nullptr)
	{
		ivars->enabled = enabled;
	}

	TraceLog("<< SetEnable()");
}
