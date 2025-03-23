//
//  HIDConstants.h
//  XboxControllerDriver
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// Some HID constants needed by the driver that are not provided in all versions of DriverKit.
//

#ifndef HIDConstants_h
#define HIDConstants_h

// MARK: - Apple Kernel Constants
// NOTE: Apple has some internal kernel codes that aren't available in DriverKit for one reason or another.
// This file sets sane defaults for them.

#ifndef kUSBHostClassRequestCompletionTimeout
constexpr uint64_t kUSBHostClassRequestCompletionTimeout = 5000;
#endif

#ifndef kLanguageIDEnglishUS
// USB Language Identifiers 1.0
constexpr uint16_t kLanguageIDEnglishUS = 0x0409;
#endif

#ifndef kIOHIDRegisterServiceKey
constexpr const char* kIOHIDRegisterServiceKey = "RegisterService";
#endif

#endif /* HIDConstants_h */
