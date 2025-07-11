# WebView2Messaging

A simple C++ Windows desktop application using WebView2 to display a web application alongside a fixed-width panel showing JSON messages received from the web content. The application includes a menu for sending JSON commands back to the webview.

## Directory Structure

```
WebView2Messaging/             # Root folder
├── WebView2Messaging.sln     # Visual Studio solution file
├── README.md                 # This file
└── WebView2Messaging/        # Source code folder
    ├── WebView2Messaging.cpp
    ├── WebView2Messaging.h
    ├── WebView2Messaging.vcxproj
    └── packages.config
```

## Features

- Embeds a WebView2 browser control occupying the left resizable panel.
- Fixed width right panel (400px) displaying a label and a read-only multi-line text box.
- Minimum main window width is 800 pixels.
- Receives JSON messages from the web content and displays the `SelectionChange` message payload in the text box.
- Provides a Windows menu to send predefined JSON command messages back to the web content.
- Uses modern C++17 features and leverages:
  - [WebView2 SDK](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)
  - [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing
  - [Windows Implementation Library (WIL)](https://github.com/microsoft/wil) for COM helpers

## Requirements

- Windows 10 or later
- Visual Studio 2022 (Professional or Community)
- C++17 language standard enabled
- NuGet packages:
  - Microsoft.Web.WebView2
  - nlohmann.json
  - wil

## Build Instructions

1. Open `WebView2Messaging.sln` in Visual Studio 2022.
2. Restore NuGet packages via Visual Studio.
3. Ensure C++17 is enabled in project properties.
4. Build and run the application.

## Usage

- The application loads the web application at `http://localhost:8080/home/`.
- The right panel displays JSON payloads when receiving messages of type `SelectionChange`.
- Use the **Actions** menu to send various JSON commands back to the webview.

## License

This project is licensed under the MIT License.

---
