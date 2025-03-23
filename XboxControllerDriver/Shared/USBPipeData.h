//
//  USBPipeData.h
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// Some structures used to organize the properties of an `IOBufferMemoryDescriptor` and `IOUSBHostPipe`.
// Provided for convenience by the original author, but is by no means required.
//

#ifndef USBPipeData_h
#define USBPipeData_h

#include <stdint.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <USBDriverKit/USBDriverKit.h>

/// Structure organizing all of the important properties of an `IOBufferMemoryDescriptor`
///
/// `buffer` - The `IOBufferMemoryDescriptor` that this structure describes.
/// `address` - An address in the driver's memory context where `buffer`'s data can be accessed.
/// `length` - The length of the data at `address`.
struct buffer_memory_descriptor {
	IOBufferMemoryDescriptor* buffer = nullptr;

	uint8_t* address = nullptr;
	uint64_t length = 0;
};

/// Structure organizing all of the important properties of an `IOUSBHostPipe`
///
/// `pipe` - The `IOUSBHostPipe` that this structure describes.
/// `speed` - An integer indicating the connection speed of the pipe. See: https://developer.apple.com/documentation/usbdriverkit/tiousbhostconnectionspeed
/// `interval` - The pipe interval in frames.
/// `reportSize` - The maximum report size the pipe can send.
/// `maxPacketSize` - The maximum packet size possible on the pipe.
/// `descriptor` - The endpoint descriptor for the pipe, mostly present for memory management purposes.
/// `memory` - The `IOBufferMemoryDescriptor` (and friends) associated with this pipe.
struct usb_pipe_data {
	IOUSBHostPipe* pipe = nullptr;

	uint8_t speed = 0;
	uint32_t interval = 0;
	uint64_t reportSize = 0;
	uint64_t maxPacketSize = 0;

	const IOUSBEndpointDescriptor* descriptor = nullptr;

	buffer_memory_descriptor memory = {};
};

#endif /* USBPipeData_h */
