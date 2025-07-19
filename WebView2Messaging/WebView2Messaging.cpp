// WebView2Messaging.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WebView2Messaging.h"

#define NOMINMAX

#include <windows.h>
#include <wrl.h>
#include <WebView2.h>
#include <string>
#include <nlohmann/json.hpp>
#include <wil/com.h>
#include <algorithm>  // for std::max


using json = nlohmann::json;
using namespace Microsoft::WRL;

HWND g_mainWindow = nullptr;
HWND g_rightPanel = nullptr;
HWND g_textLabel = nullptr;
HWND g_textBox = nullptr;
ComPtr<ICoreWebView2Controller> g_webViewController;
ComPtr<ICoreWebView2> g_webViewWindow;

constexpr int FIXED_PANEL_WIDTH = 400;
constexpr int MIN_WINDOW_WIDTH = 800;

// Menu command IDs
constexpr int IDM_ACTIONS_SEND_BROWSE_CATALOGS = 101;
constexpr int IDM_ACTIONS_SEND_ADVANCED_SEARCH = 102;
constexpr int IDM_ACTIONS_SEND_SEARCH_IN_MAP = 103;
constexpr int IDM_ACTIONS_SEND_TABLE_WINDOW = 104;

// Helper: Convert std::wstring to UTF-8 std::string safely
std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), strTo.data(), sizeNeeded, nullptr, nullptr);
    return strTo;
}

// Helper: Convert UTF-8 std::string to std::wstring
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return {};
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), wstrTo.data(), sizeNeeded);
    return wstrTo;
}

// Send a JSON message string back to the WebView2 as a string message
void SendMessageToWebView(const json& messageJson)
{
    if (g_webViewWindow)
    {
        std::string jsonStr = messageJson.dump();
        //  g_webViewWindow->PostWebMessageAsString(std::wstring(Utf8ToWide(jsonStr)).c_str());
        g_webViewWindow->PostWebMessageAsJson(std::wstring(Utf8ToWide(jsonStr)).c_str());
    }
}

void InitializeWebView2(HWND hwnd)
{
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (controller) {
                                g_webViewController = controller;
                                g_webViewController->get_CoreWebView2(&g_webViewWindow);
                            }

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            bounds.right -= FIXED_PANEL_WIDTH;
                            g_webViewController->put_Bounds(bounds);

                            g_webViewWindow->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        wil::unique_cotaskmem_string messageRaw;
                                        if (FAILED(args->get_WebMessageAsJson(&messageRaw)))
                                            return S_OK;

                                        std::wstring wmessage(messageRaw.get());
                                        std::string utf8Message = WideToUtf8(wmessage);

                                        try {
                                            json parsed = json::parse(utf8Message);
                                            if (parsed.is_string()) {
                                                parsed = json::parse(parsed.get<std::string>());
                                            }

                                            if (parsed.is_object() && parsed.contains("header") && parsed.contains("body")) {
                                                const auto& header = parsed["header"];
                                                if (header["source"]=="CATEX_BRIDGE" && header["type"] == "SelectionChange") {
                                                    std::string bodyJson = parsed["body"].dump(2);
                                                    std::wstring displayText = Utf8ToWide(bodyJson);
                                                    SetWindowTextW(g_textBox, displayText.c_str());
                                                }
                                            }
                                        }
                                        catch (const std::exception& e) {
                                            OutputDebugStringA(("JSON parse error: " + std::string(e.what()) + "\n").c_str());
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            g_webViewWindow->Navigate(L"http://localhost:8080/home/");
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create menu bar
        HMENU hMenubar = CreateMenu();
        HMENU hMenu = CreatePopupMenu();

        AppendMenu(hMenu, MF_STRING, IDM_ACTIONS_SEND_BROWSE_CATALOGS, L"Browse Catalogs");
        AppendMenu(hMenu, MF_STRING, IDM_ACTIONS_SEND_TABLE_WINDOW, L"Table Window");
        AppendMenu(hMenu, MF_STRING, IDM_ACTIONS_SEND_ADVANCED_SEARCH, L"Advanced Search");
        AppendMenu(hMenu, MF_STRING, IDM_ACTIONS_SEND_SEARCH_IN_MAP, L"Search in currrent location");
        AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, L"Actions");

        SetMenu(hwnd, hMenubar);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        switch (wmId)
        {
        case IDM_ACTIONS_SEND_BROWSE_CATALOGS:
        {
            // Compose a JSON message to send back to WebView
            json msg;
            msg["header"] = {
                {"source", "WinApp"},
                {"type", "Command"}
            };
            msg["body"] = {
                {"action", 304}   // Browse Catalogs
            };

            SendMessageToWebView(msg);
            break;
        }
        case IDM_ACTIONS_SEND_TABLE_WINDOW:
        {
            // Compose a JSON message to send back to WebView
            json msg;
            msg["header"] = {
                {"source", "WinApp"},
                {"type", "Command"}
            };
            msg["body"] = {
                {"action", 212}   // Table Window
            };
            SendMessageToWebView(msg);
            break;
        }
        case IDM_ACTIONS_SEND_ADVANCED_SEARCH:
        {
            json msg;
            msg["header"] = {
                {"source", "WinApp"},
                {"type", "Command"}
            };
            msg["body"] = {
                {"action", 301}   //  Advanced Search
            };

            SendMessageToWebView(msg);
            break;
        }
        case IDM_ACTIONS_SEND_SEARCH_IN_MAP:
        {
            json msg;
            msg["header"] = {
                {"source", "WinApp"},
                {"type", "Command"}
            };
            msg["body"] = {
                {"action", 2000},
                {"parameters", {
                    {"fitToBounds", true},
                    {"fitBounds", nullptr},
                    {"locationName", "current map bounds"}
                }}
            };
            SendMessageToWebView(msg);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        auto mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
        return 0;
    }
    case WM_SIZE:
    {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        int rightWidth = FIXED_PANEL_WIDTH;
        int leftWidth = std::max<LONG>(0, clientRect.right - rightWidth);

        // Resize WebView bounds
        RECT webBounds = { 0, 0, leftWidth, clientRect.bottom };
        if (g_webViewController) {
            g_webViewController->put_Bounds(webBounds);
        }

        // Resize and reposition container panel
        SetWindowPos(g_rightPanel, nullptr, leftWidth, 0, rightWidth, clientRect.bottom, SWP_NOZORDER);

        // Resize children relative to container
        RECT panelRect;
        GetClientRect(g_rightPanel, &panelRect);

        // Label position inside container
        SetWindowPos(g_textLabel, nullptr, 10, 10, panelRect.right - 20, 20, SWP_NOZORDER);

        // Textbox fills rest of container below label
        SetWindowPos(g_textBox, nullptr, 10, 35, panelRect.right - 20, panelRect.bottom - 45, SWP_NOZORDER);

        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"WebView2SampleWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    g_mainWindow = CreateWindowEx(
        0, CLASS_NAME, L"WebView2 Split Panel Example with Menu",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_mainWindow)
        return 0;

    // Create container panel for right column
    g_rightPanel = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0, 0, 0, 0,
        g_mainWindow, nullptr, hInstance, nullptr
    );

    // Create label as child of container
    g_textLabel = CreateWindowEx(
        0, L"STATIC", L"SelectionChange Payload:",
        WS_CHILD | WS_VISIBLE,
        10, 10, FIXED_PANEL_WIDTH - 20, 20,
        g_rightPanel, nullptr, hInstance, nullptr
    );

    // Create text box as child of container
    g_textBox = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_BORDER,
        10, 35, FIXED_PANEL_WIDTH - 20, 700,
        g_rightPanel, nullptr, hInstance, nullptr
    );

    ShowWindow(g_mainWindow, nCmdShow);
    UpdateWindow(g_mainWindow);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"COM initialization failed", L"Error", MB_OK);
        return 0;
    }

    InitializeWebView2(g_mainWindow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return 0;
}
