//
//  XboxOneDescriptors.h
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// Structure definition of Xbox One controller packets.
// This code is not specific to DriverKit in any way, and is simply part of the driver functionality.
// A driver should not need to define its own report descriptor, and instead should get it from the device.
// But this driver demonstrates converting a generic USB device to HID.
// So a report descriptor must be defined in the driver, since there is none on the device.
//

#ifndef XboxOneDescriptors_h
#define XboxOneDescriptors_h

// Report that makes the device appear as an Xbox One controller
uint8_t ReportDescriptor[] = {
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,                    // USAGE (Game Pad)
	0xa1, 0x01,                    // COLLECTION (Application)
	0xa1, 0x00,                    //   COLLECTION (Physical)
	0x85, 0x20,                    //     REPORT_ID (32)

	// Header content
	0x75, 0x08,                    //     REPORT_SIZE (8)
	0x95, 0x03,                    //     REPORT_COUNT (3)
	0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)

	// Sync
	0x05, 0x09,                    //     USAGE_PAGE (Button)
	0x09, 0x0b,                    //     USAGE (Button 11)
	0x15, 0x01,                    //     LOGICAL_MINIMUM (1)
	0x75, 0x01,                    //     REPORT_SIZE (1)
	0x95, 0x01,                    //     REPORT_COUNT (1)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// Dummy (always zero)
	0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)

	// Menu and view buttons
	0x19, 0x09,                    //     USAGE_MINIMUM (Button 9)
	0x29, 0x0a,                    //     USAGE_MAXIMUM (Button 10)
	0x95, 0x02,                    //     REPORT_COUNT (2)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// A, B, X & Y buttons
	0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
	0x29, 0x04,                    //     USAGE_MAXIMUM (Button 4)
	0x95, 0x04,                    //     REPORT_COUNT (4)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// D-Pad up, down, left & right
	0x19, 0x0c,                    //     USAGE_MINIMUM (Button 12)
	0x29, 0x0f,                    //     USAGE_MAXIMUM (Button 15)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// Left & right bumpers
	0x19, 0x05,                    //     USAGE_MINIMUM (Button 5)
	0x29, 0x06,                    //     USAGE_MAXIMUM (Button 6)
	0x95, 0x02,                    //     REPORT_COUNT (2)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// Left & right stick buttons
	0x19, 0x07,                    //     USAGE_MINIMUM (Button 7)
	0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// Left & right triggers
	0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
	0x09, 0x32,                    //     USAGE (Z)
	0x09, 0x35,                    //     USAGE (Rz)
	0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x03,              //     LOGICAL_MAXIMUM (1023)
	0x75, 0x10,                    //     REPORT_SIZE (16)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	// Left & right sticks (H & -V)
	0x09, 0x30,                    //     USAGE (X)
	0x09, 0x31,                    //     USAGE (Y)
	0x09, 0x33,                    //     USAGE (Rx)
	0x09, 0x34,                    //     USAGE (Ry)
	0x16, 0x00, 0x80,              //     LOGICAL_MINIMUM (-32768)
	0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
	0x95, 0x04,                    //     REPORT_COUNT (4)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)
	0xc0,                          //   END_COLLECTION


	0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
	0x09, 0x05,                    //   USAGE (Game Pad)
	0xa1, 0x00,                    //   COLLECTION (Physical)
	0x85, 0x07,                    //     REPORT_ID (7)

	0x75, 0x08,                    //     REPORT_SIZE (8)
	0x95, 0x03,                    //     REPORT_COUNT (3)
	0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)

	// Xbox button
	0x05, 0x09,                    //     USAGE_PAGE (Button)
	0x09, 0x10,                    //     USAGE (Button 16)
	0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
	0x75, 0x01,                    //     REPORT_SIZE (1)
	0x95, 0x01,                    //     REPORT_COUNT (1)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)

	0x75, 0x0a,                    //     REPORT_SIZE (15)
	0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
	0xc0,                          //   END_COLLECTION
	0xc0,                          // END_COLLECTION
};

uint32_t REPORT_DESCRIPTOR_SIZE = sizeof(ReportDescriptor);

#endif /* XboxOneDescriptors_h */
