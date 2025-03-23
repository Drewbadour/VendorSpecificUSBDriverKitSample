//
//  SimpleDriverLoaderModel.swift
//  SimpleDriverLoader
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// The SwiftUI scene builder that sets up the driver installation UI.
//

import Foundation
import SystemExtensions

@MainActor
class SimpleDriverLoaderModel: NSObject, ObservableObject {

	/// Deactivates the DriverKit Extension defined by ``dextBundleIdentifier``..
	///
	/// This functionality is typically not needed for most applications.
	/// However, it can be useful for situations where the user might want to temporarily disable the driver functionality.
	public var dextBundleIdentifier: String? {
		get {
			// This is the standard file path that DriverKit and other System Extensions are stored at in the app bundle.
			let sysExPath = "Contents/Library/SystemExtensions"
			let sysExURL = Bundle.main.bundleURL.appendingPathComponent(sysExPath)

			let test = Bundle.main.paths(forResourcesOfType: "dext", inDirectory: sysExPath)

			guard let fsEnumerator = FileManager.default.enumerator(at: sysExURL, includingPropertiesForKeys: nil, options: .skipsHiddenFiles) else {
				return nil
			}

			for case let targetURL as URL in fsEnumerator {
				var isDirectory: ObjCBool = false
				if FileManager.default.fileExists(atPath: targetURL.path, isDirectory: &isDirectory), isDirectory.boolValue {
					if let dextBundle = Bundle(url: targetURL),
					   let dextBundleIdentifier = dextBundle.bundleIdentifier {
						return dextBundleIdentifier
					}
				}
			}

			return nil
		}
	}
}

extension SimpleDriverLoaderModel: OSSystemExtensionRequestDelegate {

	/// Activates the DriverKit Extension defined by ``dextBundleIdentifier``.
	///
	/// > Note: This will likely result in an elevated permissions prompt for the user.
	/// > So it would be wise to warn a user before activation attempts.
	public func activateDext() -> Void {

		guard let dextBundleIdentifier: String = self.dextBundleIdentifier else {
			return
		}

		let request = OSSystemExtensionRequest.activationRequest(forExtensionWithIdentifier: dextBundleIdentifier, queue: .global(qos: .userInitiated))
		request.delegate = self
		OSSystemExtensionManager.shared.submitRequest(request)
	}

	/// Deactivates the DriverKit Extension defined by ``dextBundleIdentifier``.
	///
	/// This functionality is typically not needed for most applications.
	/// However, it can be useful for situations where the user might want to temporarily disable the driver functionality.
	public func deactivateDext() -> Void {
		guard let dextBundleIdentifier: String = self.dextBundleIdentifier else {
			return
		}

		let request = OSSystemExtensionRequest.deactivationRequest(forExtensionWithIdentifier: dextBundleIdentifier, queue: .global(qos: .userInitiated))
		request.delegate = self
		OSSystemExtensionManager.shared.submitRequest(request)
	}


	// MARK: - OSSystemExtensionRequestDelegate Functions

	/// Determines whether or not a driver should be replaced by the current version
	///
	/// This code opts for an extremely simplistic "replace always" methodology. Shipping applications should not use this logic, and instead should compare the existing driver to the new extension and determine if upgrade should occur.
	nonisolated func request(_ request: OSSystemExtensionRequest, actionForReplacingExtension existing: OSSystemExtensionProperties, withExtension ext: OSSystemExtensionProperties) -> OSSystemExtensionRequest.ReplacementAction {
		return .replace
	}

	/// Informs the program that additional user approval will be required before the DriverKit extension can operate.
	nonisolated func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
		print("requestNeedsUserApproval")
	}

	/// Informs the program of the result of a DriverKit installation flow.
	nonisolated func request(_ request: OSSystemExtensionRequest, didFinishWithResult result: OSSystemExtensionRequest.Result) {
		print("didFinishWithResult: \(result)")
	}

	/// Informs the program of a failure in the DriverKit installation flow.
	nonisolated func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
		print("didFailWithError: \(error.localizedDescription)")
	}
}
