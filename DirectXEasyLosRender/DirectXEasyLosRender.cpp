// DirectXEasyLosRender.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "DirectXEasyLosRender.h"

#define MAX_LOADSTRING 100

#include "mainHeaders.h"


#define ENABLE_EXPERIMENTAL_DXIL_SUPPORT 1

#define MY_IID_PPV_ARGS IID_PPV_ARGS



UINT m_frameIndex;
UINT m_rtvDescriptorSize;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> los_commandAllocator;
Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[2];
ID3D12Device* g_Device = nullptr;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChainLos;
Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChainMain;

Microsoft::WRL::ComPtr<ID3D12RootSignature> losRootSignature;

Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_losrtvHeap;
Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;


Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> los_command_List; // maybe other 1,2,3,4,5




// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL InitApplication(HINSTANCE hInstance);
void InitDirectX(HWND* hWnd);
static HRESULT EnableExperimentalShaderModels();




#ifdef ENABLE_EXPERIMENTAL_DXIL_SUPPORT
// A more recent Windows SDK than currently required is needed for these.
typedef HRESULT(WINAPI* D3D12EnableExperimentalFeaturesFn)(
    UINT                                    NumFeatures,
    __in_ecount(NumFeatures) const IID* pIIDs,
    __in_ecount_opt(NumFeatures) void* pConfigurationStructs,
    __in_ecount_opt(NumFeatures) UINT* pConfigurationStructSizes);

static const GUID D3D12ExperimentalShaderModelsID = // 76f5573e-f13a-40f5-b297-81ce9e18933f
{
    0x76f5573e, 0xf13a, 0x40f5, { 0xb2, 0x97, 0x81, 0xce, 0x9e, 0x18, 0x93, 0x3f }
};
#endif


static HRESULT EnableExperimentalShaderModels()
{
    HMODULE hRuntime = LoadLibraryW(L"d3d12.dll");
    if (!hRuntime)
        return HRESULT_FROM_WIN32(GetLastError());

    D3D12EnableExperimentalFeaturesFn pD3D12EnableExperimentalFeatures =
        (D3D12EnableExperimentalFeaturesFn)GetProcAddress(hRuntime, "D3D12EnableExperimentalFeatures");

    if (pD3D12EnableExperimentalFeatures == nullptr)
    {
        FreeLibrary(hRuntime);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    OutputDebugString(L" Experimental Shader's model is active ! \n");
    return pD3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModelsID, nullptr, nullptr);
}



BOOL InitApplication(HINSTANCE hInstance) {

    WNDCLASSEX wcx;

    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW; // redraw if size will change
    wcx.lpfnWndProc = WndProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // was without HBRUSH 
    wcx.lpszMenuName = L"LosMain";
    wcx.lpszClassName = L"MainWClass";
    wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    //LoadImage(hInstance, MAKEINTRESOURCE(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
   /// GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    return RegisterClassEx(&wcx);

}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

    OutputDebugStringW(L"loading simple engine \n");

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef LOS_DIRECT_ULTRA
    OutputDebugStringW(" is DirectX ULTRA ");
#endif
    
    MSG msg;

    if (!InitApplication(hInstance)) {
         
        return FALSE;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DIRECTXEASYLOSRENDER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        OutputDebugStringW(L"not init Windows \n");
        return FALSE;
    }

    BOOL fGotMessage;
    while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 && fGotMessage != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
     }



    //HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTXEASYLOSRENDER));

    //MSG msg;

    //// Main message loop:
    //while (GetMessage(&msg, nullptr, 0, 0))
    //{
    //    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }
    //}

    return (int) msg.wParam;
    UNREFERENCED_PARAMETER(lpCmdLine);
}






ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTXEASYLOSRENDER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DIRECTXEASYLOSRENDER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_CREATE: 
    {
        OutputDebugStringW(L" Create windo ");
         
        PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SWAP_LAYER_BUFFERS | PFD_DOUBLEBUFFER,    //Flags
            PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
            32,                   // Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,                   // Number of bits for the depthbuffer
            8,                    // Number of bits for the stencilbuffer
            0,                    // Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };


        InitDirectX(&hWnd);

    }


    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}




void InitDirectX(HWND* hWnd) {

    if (SUCCEEDED(D3D12GetDebugInterface(MY_IID_PPV_ARGS(&debugInterface)))) {
        debugInterface->EnableDebugLayer();
        OutputDebugString(L" Enable debug is okk ");
    }
    else {
        OutputDebugStringW(L" Not enable debug layers ");
    }


    EnableExperimentalShaderModels();

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory));


    Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;
    static const bool bUseWarpDriver = false;

    if (!bUseWarpDriver) {
        SIZE_T MaxSize = 0;

        for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx) {

         }
    }

}