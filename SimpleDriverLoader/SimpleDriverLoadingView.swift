//
//  SimpleDriverLoadingView.swift
//  SimpleDriverLoader
//
// See the LICENSE.txt file for this sample’s licensing information.
//
// Abstract:
// The SwiftUI MainActor that manages driver installation
//

import SwiftUI
import OSLog
import SystemExtensions

struct SimpleDriverLoadingView: View {

    @EnvironmentObject var model: SimpleDriverLoaderModel
    
    var body: some View {
        VStack {
            Text("Demo Controller Driver Loader")
                .font(.title)
                .padding()
            Text("Not For Distribution!")
                .font(.title2)
                .foregroundColor(.red)
            HStack {
                Button(
                    action: {
                        model.activateDext()
                    }, label: {
                        Text("Install/Update Driver")
                    }
                )
                Button(
                    action: {
                        model.deactivateDext()
                    }, label: {
                        Text("Uninstall Driver")
                    }
                )
            }
        }
        .padding()
    }
}

struct SimpleDriverLoadingView_Previews: PreviewProvider {
    
    static var model: SimpleDriverLoaderModel = .init()
    
    static var previews: some View {
        SimpleDriverLoadingView().environmentObject(model)
    }
}
