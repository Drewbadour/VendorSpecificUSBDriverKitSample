# Converting a Vendor-Specific USB Device to HID with DriverKit

Convert a vendor-specific USB device to HID using USBDriverKit and HIDDriverKit.

[link_news_DemystifyCodeSigningForDriverKit]:https://developer.apple.com/news/?id=c63qcok4
[link_article_DisablingEnablingSystemIntegrityProtection]:https://developer.apple.com/documentation/security/disabling_and_enabling_system_integrity_protection
[link_article_TestingSystemExtensions]:https://developer.apple.com/documentation/driverkit/debugging_and_testing_system_extensions?language=objc
[link_framework_SystemExtensions]:https://developer.apple.com/documentation/systemextensions
[link_article_InstallingSystemExtensionsAndDrivers]:https://developer.apple.com/documentation/systemextensions/installing_system_extensions_and_drivers
[link_news_MatchYourDriverKitDrivers]:https://developer.apple.com/news/?id=zk5xdwbn
[link_article_DriverMatchingTable]:https://developer.apple.com/library/archive/qa/qa1076/_index.html
[link_article_RequestingEntitlementsForDriverKitDevelopment]:https://developer.apple.com/documentation/driverkit/requesting_entitlements_for_driverkit_development

## Overview

This sample code project shows how a DriverKit extension (dext) can be used to convert a Vendor-Specific USB device to HID. This is not a typical use case for DriverKit, but covers many advanced topics that can help improve DriverKit concept comprehension.

Please note that this sample is not configured to work with iPadOS.

The sample project contains three targets:
* `SimpleDriverLoader` - A SwiftUI app for macOS. Use this app to install or update the driver.
* `XboxControllerDriver` - The dext itself, which contains drivers for a USB device, USB interface, and user client.
* `UserClientTester` - A simple C++ application to communicate with the driver's user client.

## Configure the Sample Code Project

To run the sample code project, you first need to build and run `SimpleDriverLoader`, which installs the driver.

Since this sample requires the use of a restricted entitlement (`com.apple.developer.driverkit.transport.usb`), it uses `xcconfig` files to manage signing.

In order to configure debug and release signing, please create a `Signing_Debug.xcconfig` and `Signing_Release.xcconfig` in the `BuildConfig/` directory. They will require these keys to be set:

* `DEVELOPMENT_TEAM` - Found in the "quick look" preview for your provisioning profile, under the "Entitlements" header, with the key `com.apple.developer.team-identifier`. Typically this is a 10 character string of uppercase letters and numbers.
* `PROVISIONING_PROFILE_APP` - The UUID for the provisioning profile for the `SimpleDriverLoader` app. Found in the "quick look" preview for your provisioning profile, under "UUID".
* `PROVISIONING_PROFILE_APP` - The UUID for the provisioning profile for the `XboxControllerDriver` dext. Found in the "quick look" preview for your provisioning profile, under "UUID".

If you need additional help with DriverKit codesigning, consider reading [Demystify code signing for DriverKit][link_news_DemystifyCodeSigningForDriverKit].

## Install and run the driver extension on macOS

Before running the app, consider enabling System Extension developer mode to simplify your development experience. This can be achieved by the following:

1. Temporarily turn off SIP, as described in the article [Disabling and Enabling System Integrity Protection][link_article_DisablingEnablingSystemIntegrityProtection].
1. Confirm that SIP is disabled with the Terminal command `csrutil status`.
1. Enter dext development mode with `systemextensionsctl developer`, as described in the article [Debugging and Testing System Extensions][link_article_TestingSystemExtensions].

To run the sample app in macOS, use the scheme selector to select the `SimpleDriverLaoder` scheme and the "My Mac" destination. If you have not enabled System Extension developer mode, then you will need to copy the app to the Applications folder and launch the app after building. However, if System Extension developer mode is enabled, you can run the app directly from Xcode.

The `SimpleDriverLoader` target declares the `XboxControllerDriver` as a dependency, so building the app target builds the dext and its installer together. When run, the `SimpleDriverLoader` shows a single window with a text label that says "Demo Controller Driver Loader". Below this, it shows an "Install/Update Driver" button, and a "Uninstall Driver" button. Click "Install/Update Driver" to perform the installation, or upgrade the driver to the latest version.

To install the dext on macOS, the app uses the [System Extensions][link_framework_SystemExtensions] framework to install and activate the dext, as described in [Installing System Extensions and Drivers][link_article_InstallingSystemExtensionsAndDrivers].

- Note: This call may prompt a "System Extension Blocked" dialog, which explains that `SimpleDriverLoader` tried to install a new system extension. To complete the installation, open System Preferences and go to the Security & Privacy pane. Unlock the pane if necessary, and click "Allow" to complete the installation. To confirm installation of the `XboxControllerDriver` extension, run `systemextensionsctl list` in Terminal.

## How this driver works

Darwin will not initialize vendor-specific USB devices. Vendor-specific USB devices are any device with a USB `bDeviceClass` of 255 (0xFF). This means that devices like the Xbox One controller will not enumerate any of their interfaces to a device running Darwin without a device-level driver. In order to resolve this, a device-matching driver must be written to attach to the device and set a configuration that will enumerate the interfaces. In the case of this driver, this is the `Microsoft - Xbox One - Device` in the driver's `IOKitPersonalities`. In the `Start_Impl` method of the device driver, setting the configuration of the device with `device->SetConfiguration(TARGET_CONFIGURATION, true)` will achieve this goal.

Once the interface is available, the `Microsoft - Xbox One - Interface` entry in the `Info.plist` will be matched, initializing a driver for the controller interface. The driver interface class for the Xbox One controller interface extends the `IOUserHIDDevice` class, which provides most of the affordances needed to communicate with a HID device. If this was actually a HID device, then the driver would work with the device as-expected. However, for devices that aren't actually HID-compatible, a few changes will need to be made.

1. A HID-compliant device description must be provided by overriding `newDeviceDescription`
1. A HID report descriptor must be provided by overriding `newReportDescriptor`
1. The driver must initialize the pipes and manage communication to the device, sending packets to the OS with `handleReport`.

By providing all of these things, it is possible to make an Xbox One controller appear as if it were a HID-compliant device.

## Matching a Vendor-Specific USB Device

Referring to the driver score matching table from [this technical Q&A][link_article_DriverMatchingTable]:

| Keys | Comments | Probe Score |
|---|---|---|
| idVendor + idProduct + bcdDevice | | 100000 |
| idVendor + idProduct | | 90000 |
| idVendor + bDeviceSubClass + bDeviceProtocol | Only if bDeviceClass is 0xFF (vendor specific). | 80000 |
| idVendor + bDeviceSubClass | Only if bDeviceClass is 0xFF (vendor specific). | 70000 |
| bDeviceClass + bDeviceSubClass + bDeviceProtocol | Only if bDeviceClass is not 0xFF. | 60000 |
| bDeviceClass + bDeviceSubClass | Only if bDeviceClass is not 0xFF. | 50000 |

Notice that the only matching options available for a device with a `bDeviceClass` of 0xFF, aka a vendor specific device, are the first four. All of which involve matching on `idVendor`. In DriverKit, matching on `idVendor` is controlled via restricted entitlements. This means that proceeding with development of a vendor-specific device will require a restricted entitlement.

If you need to request restricted entitlements, consider reading the article [Requesting Entitlements for DriverKit Development][link_article_RequestingEntitlementsForDriverKitDevelopment].


### `Info.plist`

As for the `Info.plist` matching, consider this example from the source code:

```xml
<key>Microsoft - Xbox One - Device</key>
<dict>
    <key>CFBundleIdentifier</key>
    <string>$(PRODUCT_BUNDLE_IDENTIFIER)</string>
    <key>CFBundleIdentifierKernel</key>
    <string>com.apple.kpi.iokit</string>
    <key>IOClass</key>
    <string>IOUserService</string>
    <key>IOMatchCategory</key>
    <string>com.apple.null.driver</string>
    <key>IOProviderClass</key>
    <string>IOUSBHostDevice</string>
    <key>IOResourceMatch</key>
    <string>IOKit</string>
    <key>IOUserClass</key>
    <string>XboxOneDevice</string>
    <key>IOUserServerName</key>
    <string>com.apple.null.driver</string>
    <key>UserClientProperties</key>
    <dict>
        <key>IOClass</key>
        <string>IOUserUserClient</string>
        <key>IOUserClass</key>
        <string>XboxOneDeviceUserClient</string>
    </dict>
    <key>idVendor</key>
    <integer>1118</integer>
    <key>idProduct</key>
    <integer>746</integer>
</dict>
```

The key elements are `IOProviderClass` set to `IOUSBHostDevice`, which means that this entry matches a USB device, and an `IOUserClass` set to the name of your DriverKit class. The `UserClientProperties` section is included for the purposes of this driver, but is unnecessary for matching a USB device. Finally, notice that the driver uses the `idVendor` and `idProduct` matching criteria, which correspond to the second entry in the USB device matching table.

## Matching a USB Interface

Matching USB interfaces follows a similar rule set:

| Keys | Comments | Probe Score |
|---|---|---|
| idVendor + idProduct + bInterfaceNumber + bConfigurationValue + bcdDevice | | 100000 |
| idVendor + idProduct + bInterfaceNumber + bConfigurationValue | | 90000 |
| idVendor + bInterfaceSubClass + bInterfaceProtocol | Only if bInterfaceClass is 0xFF (vendor specific). | 80000 |
| idVendor + bInterfaceSubClass | Only if bInterfaceClass is 0xFF (vendor specific). | 70000 |
| bInterfaceClass + bInterfaceSubClass + bInterfaceProtocol | Only if bInterfaceClass is not 0xFF. | 60000 |
| bInterfaceClass + bInterfaceSubClass | Only if bInterfaceClass is not 0xFF. | 50000 |

Similar rules apply for vendor-specific interfaces, which will also require restricted entitlements for matching.

### `Info.plist`

```xml
<key>Microsoft - Xbox One - Interface</key>
<dict>
    <key>CFBundleIdentifier</key>
    <string>$(PRODUCT_BUNDLE_IDENTIFIER)</string>
    <key>CFBundleIdentifierKernel</key>
    <string>com.apple.kpi.iokit</string>
    <key>IOClass</key>
    <string>AppleUserHIDDevice</string>
    <key>IOMatchCategory</key>
    <string>com.apple.null.driver</string>
    <key>IOProviderClass</key>
    <string>IOUSBHostInterface</string>
    <key>IOResourceMatch</key>
    <string>IOKit</string>
    <key>IOUserClass</key>
    <string>XboxOneInputInterface</string>
    <key>IOUserServerName</key>
    <string>com.apple.null.driver</string>
    <key>idVendor</key>
    <integer>1118</integer>
    <key>idProduct</key>
    <integer>746</integer>
    <key>bInterfaceNumber</key>
    <integer>0</integer>
    <key>bConfigurationValue</key>
    <integer>1</integer>
    <key>UserClientProperties</key>
    <dict>
        <key>IOClass</key>
        <string>IOUserUserClient</string>
        <key>IOUserClass</key>
        <string>XboxOneUserClient</string>
    </dict>
</dict>
```

Once again, the key elements are `IOProviderClass` set to `IOUSBHostInterface` for matching USB interfaces, and an `IOUserClass` set to the name of your DriverKit class. The `UserClientProperties` section is included for the purposes of this driver, but is unnecessary for matching a USB interface. For this interface, matching is done using the second table item once again using the `idVendor`, `idProduct`, `bInterfaceNumber`, and `bConfigurationValue`.

For more information on matching drivers, check out the [Match your DriverKit drivers with the right USB device][link_news_MatchYourDriverKitDrivers] article and the articles it links.
