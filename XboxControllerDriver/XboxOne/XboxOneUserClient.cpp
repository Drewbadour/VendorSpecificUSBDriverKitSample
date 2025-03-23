//
//  XboxOneUserClient.cpp
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// An interface for user client communication to the driver.
// Includes a simplistic means of managing driver "licensing" to enable and disable the driver.
//

#include <stdio.h>

#include <os/log.h>

#include <DriverKit/DriverKit.h>

#include "XboxOneUserClient.h"
#include "XboxOneInputInterface.h"

#define Log(fmt, ...) os_log(OS_LOG_DEFAULT, "Xbox UserClient - " fmt "\n", ##__VA_ARGS__)

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

/// Enumeration of different message types that the driver accepts
///
/// Values are purely arbitrary, and provided for demonstration purposes only.
typedef enum
{
	ExternalMethodType_Unknown = 0,
	ExternalMethodType_Licensing = 1,
	kNumberOfExternalMethods
} ExternalMethodType;


/// Array defining the external methods that the driver supports.
///
/// For more information on this structure, please consult the "Communicating between a DriverKit extension and a client app" Apple sample.
///
/// In this case, the licensing function takes a single scalar input, and returns a single scalar input,
/// calling the `StaticHandleLicensing` when a call is made on the `ExternalMethodType_Licensing` selector (1).
///
/// Note that this array contains 2 (`kNumberOfExternalMethods`) elements, where index 0 (`ExternalMethodType_Unknown`) is not populated.
const IOUserClientMethodDispatch externalMethodChecks[kNumberOfExternalMethods] =
{
	[ExternalMethodType_Licensing] =
	{
		.function = (IOUserClientMethodFunction) &XboxOneUserClient::StaticHandleLicensing,
		.checkCompletionExists = false,
		.checkScalarInputCount = 1,
		.checkStructureInputSize = 0,
		.checkScalarOutputCount = 1,
		.checkStructureOutputSize = 0,
	},
};




// MARK: - UserClient Lifecycle

/// Stored variables of the user client interface.
struct XboxOneUserClient_IVars
{
	/// A reference to the main controller interface for communication back from the user client interface.
	XboxOneInputInterface* inputInterface = nullptr;
};




// MARK: UserClient Lifecycle - Startup

/// Initializer for the user client interface
bool XboxOneUserClient::init(void)
{
	bool result = false;

	TraceLog(">> init()");

	result = super::init();
	if (result != true)
	{
		Log("init() - super::init failed.");
		goto Exit;
	}

	ivars = IONewZero(XboxOneUserClient_IVars, 1);
	if (ivars == nullptr)
	{
		Log("init() - Failed to allocate memory for ivars.");
		goto Exit;
	}

	TraceLog("<< init()");
	return true;

Exit:
	return false;
}

/// Registration of the user client interface
kern_return_t XboxOneUserClient::Start_Impl(IOService* provider)
{
	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> Start()");

	ret = Start(provider, SUPERDISPATCH);
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - super::Start failed with error: 0x%08x.", ret);
		goto Exit;
	}

	ret = RegisterService();
	if (ret != kIOReturnSuccess)
	{
		Log("Start() - Failed to register service with error: 0x%08x.", ret);
		goto Exit;
	}

	TraceLog("<< Start()");
	ret = kIOReturnSuccess;

Exit:
	return ret;
}

/// Set the `XboxOneInputInterface` that the user client interface can call back.
void XboxOneUserClient::SetInputInterface(IOService* interface)
{
	TraceLog("<< %{public}s", __PRETTY_FUNCTION__);

	ivars->inputInterface = OSDynamicCast(XboxOneInputInterface, interface);
	if (ivars->inputInterface == nullptr)
	{
		Log("%{public}s - Passed interface was null or not of correct type.", __PRETTY_FUNCTION__);
		goto Exit;
	}

Exit:
	TraceLog(">> %{public}s", __PRETTY_FUNCTION__);
}




// MARK: UserClient Lifecycle - Shutdown

/// Shutdown of the user client interface
kern_return_t XboxOneUserClient::Stop_Impl(IOService* provider)
{
	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> Stop()");

	ret = Stop(provider, SUPERDISPATCH);
	if (ret != kIOReturnSuccess)
	{
		Log("Stop() - super::Stop failed with error: 0x%08x.", ret);
	}

	TraceLog("<< Stop()");

	return ret;
}

/// Cleanup of the user client interface
void XboxOneUserClient::free(void)
{
	TraceLog("free()");

	IOSafeDeleteNULL(ivars, XboxOneUserClient_IVars, 1);

	super::free();
}




// MARK: - External Communication

/// Handler for calls to the user client interface.
///
/// For more information on this function, please consult the "Communicating between a DriverKit extension and a client app" Apple sample.
///
/// This code should not be called directly by any other driver code, and is instead called by DriverKit when a selector is called on the user client interface.
kern_return_t XboxOneUserClient::ExternalMethod(uint64_t selector, IOUserClientMethodArguments* arguments, const IOUserClientMethodDispatch* dispatch, OSObject* target, void* reference)
{
	kern_return_t ret = kIOReturnNotFound;

	TraceLog(">> ExternalMethod()");
	DebugLog("ExternalMethod() - Selector: %llu", selector);

	// Ensure that selector is supported by the user client
	if (selector > ExternalMethodType_Unknown)
	{
		if (selector < kNumberOfExternalMethods)
		{
			dispatch = &externalMethodChecks[selector];
			if (!target)
			{
				target = this;
			}
		}

		ret = super::ExternalMethod(selector, arguments, dispatch, target, reference);
	}

	TraceLog("<< ExternalMethod()");
	return ret;
}

/// Static callback that calls back `HandleLicensing` using the context provided in `reference`
///
/// This code should not be called directly by any other driver code, and is instead called by DriverKit when the appropriate selector is called on the user client interface.
/// In the case of this sample, the "appropriate" selector is defined by `externalMethodChecks` and assigned as part of `ExternalMethod`.
kern_return_t XboxOneUserClient::StaticHandleLicensing(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
	TraceLog("StaticHandleLicensing()");

	if (target == nullptr)
	{
		return kIOReturnError;
	}

	return ((XboxOneUserClient*)target)->HandleLicensing(reference, arguments);
}

/// Enables or disables the Xbox controller interface depending on the request to the user client.
///
/// Provided as an example to show how a user client interacts with a hardware-matched interface.
kern_return_t XboxOneUserClient::HandleLicensing(void* reference, IOUserClientMethodArguments* arguments)
{
	(void)reference;

	kern_return_t ret = kIOReturnSuccess;

	TraceLog(">> HandleLicensing()");

	bool enable = (bool)(arguments->scalarInput[0]);
	DebugLog("HandleLicensing() - Attempting to %{public}s driver.", enable ? "enable" : "disable");

	if (ivars->inputInterface != nullptr)
	{
		ivars->inputInterface->SetEnable(enable);
		arguments->scalarOutput[0] = enable;
	}
	else
	{
		Log("HandleLicensing() - Input interface is null.");
		arguments->scalarOutput[0] = false;
	}

	TraceLog("<< HandleLicensing()");

	return ret;
}
