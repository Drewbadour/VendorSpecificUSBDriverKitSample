//
//  XboxOneInputPackets.h
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// Structure definition of Xbox One controller packets.
// This code is not specific to DriverKit in any way, and is simply part of the driver functionality.
//

#ifndef XboxOneInputPackets_h
#define XboxOneInputPackets_h

// MARK: - Shared Packet Structure

/// Enumeration defining all of the different packet types the driver will handle.
///
/// The Xbox One controller protocol specifies the packet type as the first byte of data.
/// So the values of this enum correlate to that first byte.
typedef enum {
	XBOXONE_IN_GUIDE = 0x07,
	XBOXONE_IN_BUTTON = 0x20,
} xboxone_in_packet_type;




// MARK: - Packets from Controller

/// The structure of the "header" of an Xbox One controller packet
///
/// All packets sent from the Xbox One controller have this header.
/// `packetType` - Classifies the data content of packet following this header.
/// `version` - The version of the packet. Almost always 0.
/// `counter` -  An incrementing counter to make sure inputs are evaluated in order.
/// `size` - The size in bytes of the packet data following this header.
typedef struct {
	uint8_t packetType;
	uint8_t version;
	uint8_t counter;
	uint8_t size;
} xboxone_report_header;
constexpr uint8_t XBOXONE_REPORT_HEADER_SIZE = sizeof(xboxone_report_header);

/// The structure of a button report sent from the Xbox One controller.
///
/// Whenever any button, trigger, or joystick value changes, the Xbox One controller will send a button update packet.
/// `buttons` - A bitfield representing the buttons, with 1 indicating pressed. See the `xboxone_buttons` enum for more information.
/// `trigL` - How depressed the left trigger is from 0 - 1023.
/// `trigL` - How depressed the right trigger is from 0 - 1023.
/// `leftX` - The orientation of the left stick in the left-to-right direction. Where -32,768 is all the way left, and 32,767 is all the way right.
/// `leftY` - The orientation of the left stick in the down-to-up direction. Where -32,768 is all the way down, and 32,767 is all the way up.
/// `rightX` - The orientation of the right stick in the left-to-right direction. Where -32,768 is all the way left, and 32,767 is all the way right.
/// `rightY` - The orientation of the right stick in the down-to-up direction. Where -32,768 is all the way down, and 32,767 is all the way up.
typedef struct {
	xboxone_report_header header;

	uint16_t buttons;
	uint16_t trigL, trigR;
	int16_t leftX, leftY, rightX, rightY;
} xboxone_button_report;
constexpr uint8_t XBOXONE_BUTTON_REPORT_SIZE = sizeof(xboxone_button_report) - XBOXONE_REPORT_HEADER_SIZE;

/// The structure of a button "guide" button report sent from the Xbox One controller.
///
/// Whenever the glowing "guide" or "xbox button" is pressed, a separate packet is sent from the standard button packet.
/// `guide` - 1 if the guide button is pressed. 0 if the guide button was released.
/// `_reserved1` - Unknown. Always zero.
typedef struct {
	xboxone_report_header header;

	uint8_t guide;
	uint8_t _reserved1;
} xboxone_guide_report;
constexpr uint8_t XBOXONE_GUIDE_REPORT_SIZE = sizeof(xboxone_guide_report) - XBOXONE_REPORT_HEADER_SIZE;

/// Enumeration defining the bitfield orientation of buttons in the `xboxone_button_report`.
///
/// `XBOXONE_SYNC` - The button used to activate the "pairing" functionality of the controller.
/// `XBOXONE_GUIDE` - The glowing "guide" or "Xbox Button".
/// `XBOXONE_MENU` - The three lines or "hamburger" button.
/// `XBOXONE_VIEW` - The two intersecting rectangles button.
/// `XBOXONE_LEFT_THUMB` - The state of the left stick "click".
/// `XBOXONE_RIGHT_THUMB` - The state of the right stick "click".
typedef enum {
	XBOXONE_SYNC           = 0x0001, // Bit 00
	XBOXONE_GUIDE          = 0x0002, // Bit 01
	XBOXONE_MENU           = 0x0004, // Bit 02
	XBOXONE_VIEW           = 0x0008, // Bit 03
	XBOXONE_A              = 0x0010, // Bit 04
	XBOXONE_B              = 0x0020, // Bit 05
	XBOXONE_X              = 0x0040, // Bit 06
	XBOXONE_Y              = 0x0080, // Bit 07
	XBOXONE_DPAD_UP        = 0x0100, // Bit 08
	XBOXONE_DPAD_DOWN      = 0x0200, // Bit 09
	XBOXONE_DPAD_LEFT      = 0x0400, // Bit 10
	XBOXONE_DPAD_RIGHT     = 0x0800, // Bit 11
	XBOXONE_LEFT_SHOULDER  = 0x1000, // Bit 12
	XBOXONE_RIGHT_SHOULDER = 0x2000, // Bit 13
	XBOXONE_LEFT_THUMB     = 0x4000, // Bit 14
	XBOXONE_RIGHT_THUMB    = 0x8000, // Bit 15
} xboxone_buttons;




// MARK: - Packets to Controller

/// The structure of a button "guide" button response sent to the Xbox One controller.
///
/// When the controller sends a "guide" button packet. It expects a response in this format.
/// `constData` - The bytes: 0x00, 0x07, 0x20, 0x02.
/// `padding` - Empty unused bytes.
typedef struct {
	xboxone_report_header header;

	uint8_t constData[4];
	uint8_t padding[5];
} xboxone_guide_response;
constexpr uint8_t XBOXONE_GUIDE_RESPONSE_SIZE = sizeof(xboxone_guide_response) - XBOXONE_REPORT_HEADER_SIZE;

#endif /* XboxOneInputPackets_h */
