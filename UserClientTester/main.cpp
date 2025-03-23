//
//  main.cpp
//  UserClientTester
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// A simple C++ program to communicate with the driver's UserClient.
//


#include <iostream>
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDShared.h>

#define kIOPrimaryPortDefault 0

int main(int argc, const char* argv[])
{
	static const char* dextIdentifier = "XboxOneInputInterface";

	kern_return_t ret = kIOReturnSuccess;
	io_iterator_t iterator = IO_OBJECT_NULL;
	io_service_t service = IO_OBJECT_NULL;
	io_connect_t connection = IO_OBJECT_NULL;

	ret = IOServiceGetMatchingServices(kIOPrimaryPortDefault, IOServiceNameMatching(dextIdentifier), &iterator);
	if (ret != kIOReturnSuccess)
	{
		printf("Unable to find service for identifier with error: 0x%08x.\n", ret);
	}

	printf("Searching for dext service...\n");
	while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
	{
		ret = IOServiceOpen(service, mach_task_self_, kIOHIDServerConnectType, &connection);

		if (ret == kIOReturnSuccess)
		{
			printf("\tOpened service.\n");
			break;
		}
		else
		{
			printf("\tFailed opening service with error: 0x%08x.\n", ret);
		}

		IOObjectRelease(service);
	}
	IOObjectRelease(iterator);

	if (service == IO_OBJECT_NULL)
	{
		printf("Failed to match to device.\n");
		return EXIT_FAILURE;
	}

	{
		const uint32_t selector = 1;
		const uint32_t arraySize = 1;
		const uint64_t input[arraySize] = { true };

		uint32_t outputArraySize = arraySize;
		uint64_t output[arraySize] = {};
		
		ret = IOConnectCallScalarMethod(connection, selector, input, arraySize, output, &outputArraySize);
		if (ret != kIOReturnSuccess)
		{
			printf("IOConnectCallScalarMethod failed with error: 0x%08x.\n", ret);
		}
		
		printf("Input of size: %u, data: %llu\n", arraySize, input[0]);
		printf("Output of size: %u, data: %llu\n", outputArraySize, output[0]);
	}

	return 0;
}
