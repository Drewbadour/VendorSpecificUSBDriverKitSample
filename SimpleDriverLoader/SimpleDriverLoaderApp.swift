//
//  SimpleDriverLoaderApp.swift
//  SimpleDriverLoader
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// The SwiftUI scene builder that sets up the driver installation UI.
//

import SwiftUI

@main
struct SimpleDriverLoaderApp: App {

	@StateObject private var model: SimpleDriverLoaderModel = SimpleDriverLoaderModel()

	var body: some Scene {
		WindowGroup {
			SimpleDriverLoadingView().environmentObject(model);
		}
	}
}
