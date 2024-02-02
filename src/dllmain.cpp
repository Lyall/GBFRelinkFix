#include "stdafx.h"
#include "helper.hpp"
#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

// Logger and config setup
inipp::Ini<char> ini;
string sFixName = "GBFRelinkFix";
string sFixVer = "1.0";
string sLogFile = "GBFRelinkFix.log";
string sConfigFile = "GBFRelinkFix.ini";
string sExeName;
filesystem::path sExePath;
RECT rcDesktop;

// Ini Variables
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
bool bHUDFix;
bool bAspectFix;
bool bFOVFix;
bool bFPSCap;

// Aspect ratio + HUD stuff
float fNativeAspect = (float)16 / 9;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fDefaultHUDWidth = (float)1920;
float fDefaultHUDHeight = (float)1080;
float fHUDWidthOffset;
float fHUDHeightOffset;

void Logging()
{
    // spdlog initialisation
    {
        try
        {
            auto logger = spdlog::basic_logger_mt(sFixName.c_str(), sLogFile, true);
            spdlog::set_default_logger(logger);

        }
        catch (const spdlog::spdlog_ex& ex)
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        }
    }

    spdlog::flush_on(spdlog::level::debug);
    spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
    spdlog::info("----------");

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();

    // Log module details
    spdlog::info("Module Name: {0:s}", sExeName.c_str());
    spdlog::info("Module Path: {0:s}", sExePath.string().c_str());
    spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
    spdlog::info("Module Timesstamp: {0:d}", Memory::ModuleTimestamp(baseModule));
    spdlog::info("----------");
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(sConfigFile);
    if (!iniFile)
    {
        spdlog::critical("Failed to load config file.");
        spdlog::critical("Make sure {} is present in the game folder.", sConfigFile);

    }
    else
    {
        ini.parse(iniFile);
    }

    // Read ini file
    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomResolution);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Uncap Framerate"], "Enabled", bFPSCap);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);

    // Log config parse
    spdlog::info("Config Parse: bCustomResolution: {}", bCustomResolution);
    spdlog::info("Config Parse: iCustomResX: {}", iCustomResX);
    spdlog::info("Config Parse: iCustomResY: {}", iCustomResY);
    spdlog::info("Config Parse: bFPSCap: {}", bFPSCap);
    spdlog::info("Config Parse: bHUDFix: {}", bHUDFix);
    spdlog::info("Config Parse: bAspectFix: {}", bAspectFix);
    spdlog::info("Config Parse: bFOVFix: {}", bFOVFix);
    spdlog::info("----------");

    // Calculate aspect ratio / use desktop res instead
    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fAspectRatio = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        iCustomResX = (int)rcDesktop.right;
        iCustomResY = (int)rcDesktop.bottom;
        fAspectRatio = (float)rcDesktop.right / (float)rcDesktop.bottom;
    }
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD variables
    fHUDWidth = iCustomResY * fNativeAspect;
    fHUDHeight = (float)iCustomResY;
    fHUDWidthOffset = (float)(iCustomResX - fHUDWidth) / 2;
    fHUDHeightOffset = 0;
    if (fAspectRatio < fNativeAspect)
    {
        fHUDWidth = (float)iCustomResX;
        fHUDHeight = (float)iCustomResX / fNativeAspect;
        fHUDWidthOffset = 0;
        fHUDHeightOffset = (float)(iCustomResY - fHUDHeight) / 2;
    }

    // Log aspect ratio stuff
    spdlog::info("Custom Resolution: fAspectRatio: {}", fAspectRatio);
    spdlog::info("Custom Resolution: fAspectMultiplier: {}", fAspectMultiplier);
    spdlog::info("Custom Resolution: fHUDWidth: {}", fHUDWidth);
    spdlog::info("Custom Resolution: fHUDHeight: {}", fHUDHeight);
    spdlog::info("Custom Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
    spdlog::info("Custom Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
    spdlog::info("----------");
}

void CustomResolution()
{ 
    if (bCustomResolution)
    {
        uint8_t* ResolutionScanResult = Memory::PatternScan(baseModule, "B8 80 07 00 00 89 ?? ?? ?? 89 ?? ?? ??");
        if (ResolutionScanResult)
        {
            spdlog::info("Custom Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ResolutionScanResult - (uintptr_t)baseModule);

            static SafetyHookMid ResolutionMidHook{};
            ResolutionMidHook = safetyhook::create_mid(ResolutionScanResult + 0x5,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = iCustomResX;
                    ctx.rcx = iCustomResY;
                });
            spdlog::info("Custom Resolution: Applied custom resolution of {}x{}", iCustomResX, iCustomResY);
        }
        else if (!ResolutionScanResult)
        {
            spdlog::error("Custom Resolution: Pattern scan failed.");
        }

        float graphicsWidth = (float)iCustomResX / 32;
        if (floor(graphicsWidth) != graphicsWidth)
        {
            spdlog::info("Graphics Corruption: Detected non-whole number, injecting hooks to round up values.");
            uint8_t* GraphicsCorruptionScanResult1 = Memory::PatternScan(baseModule, "C4 ?? ?? ?? ?? 10 C4 ?? ?? ?? ?? 20 C4 ?? ?? ?? ?? 30 C5 ?? ?? ?? 48 ?? ?? ?? ?? ?? 00 00");
            uint8_t* GraphicsCorruptionScanResult2 = Memory::PatternScan(baseModule, "C4 ?? ?? ?? ?? 10 C4 ?? ?? ?? ?? 20 C4 ?? ?? ?? ?? 30 C5 ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? 48 ?? ?? ?? ?? 00 00");
            if (GraphicsCorruptionScanResult1 && GraphicsCorruptionScanResult2)
            {
                spdlog::info("Graphics Corruption: 1: Address is {:s}+{:x}", sExeName.c_str(), ((uintptr_t)GraphicsCorruptionScanResult1 - (uintptr_t)baseModule));
                spdlog::info("Graphics Corruption: 2: Address is {:s}+{:x}", sExeName.c_str(), ((uintptr_t)GraphicsCorruptionScanResult2 - (uintptr_t)baseModule));

                static SafetyHookMid GraphicsCorruption1MidHook{};
                GraphicsCorruption1MidHook = safetyhook::create_mid(GraphicsCorruptionScanResult1,
                    [](SafetyHookContext& ctx)
                    {
                        ctx.xmm3.f32[0] = ceil(ctx.xmm3.f32[0]);
                    });

                static SafetyHookMid GraphicsCorruption2MidHook{};
                GraphicsCorruption2MidHook = safetyhook::create_mid(GraphicsCorruptionScanResult2,
                    [](SafetyHookContext& ctx)
                    {
                        ctx.xmm3.f32[0] = ceil(ctx.xmm3.f32[0]);
                    });
            }
            else if (!GraphicsCorruptionScanResult1 || !GraphicsCorruptionScanResult2)
            {
                spdlog::error("Graphics Corruption: Pattern scan failed.");
            }
        }
    }
}

void AspectFOVFix()
{
    // Don't do any of this if the aspect ratio is wider than native
    if (fNativeAspect > fAspectRatio)
    {
        if (bAspectFix)
        {
            // Aspect Ratio
            uint8_t* AspectRatioScanResult = Memory::PatternScan(baseModule, "49 ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? C5 ?? ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? ??");
            if (AspectRatioScanResult)
            {
                spdlog::info("Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)AspectRatioScanResult - (uintptr_t)baseModule);

                static SafetyHookMid AspectRatioMidHook{};
                AspectRatioMidHook = safetyhook::create_mid(AspectRatioScanResult + 0x5,
                    [](SafetyHookContext& ctx)
                    {
                        *reinterpret_cast<float*>(ctx.rax + 0x9D0) = fAspectRatio;
                    });
            }
            else if (!AspectRatioScanResult)
            {
                spdlog::error("Aspect Ratio: Pattern scan failed.");
            }

        }
       
        if (bFOVFix)
        {
            // Gameplay FOV
            uint8_t* GameplayFOVScanResult = Memory::PatternScan(baseModule, "75 ?? C5 ?? ?? ?? ?? ?? ?? 00 48 ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? 00 C6 ?? ?? ?? ?? 00 01");
            if (GameplayFOVScanResult)
            {
                spdlog::info("Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayFOVScanResult - (uintptr_t)baseModule);

                static SafetyHookMid GameplayFOVMidHook{};
                GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult + 0xE,
                    [](SafetyHookContext& ctx)
                    {
                        ctx.xmm0.f32[0] /= fAspectMultiplier;
                    });
            }
            else if (!GameplayFOVScanResult)
            {
                spdlog::error("Gameplay FOV: Pattern scan failed.");
            }

            // Cutscene FOV
            uint8_t* CutsceneFOVScanResult = Memory::PatternScan(baseModule, "48 ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? 00 48 ?? ??");
            if (CutsceneFOVScanResult)
            {
                spdlog::info("Cutscene FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CutsceneFOVScanResult - (uintptr_t)baseModule);

                static SafetyHookMid CutsceneFOVMidHook{};
                CutsceneFOVMidHook = safetyhook::create_mid(CutsceneFOVScanResult + 0xC,
                    [](SafetyHookContext& ctx)
                    {
                        ctx.xmm2.f32[0] /= fAspectMultiplier;
                    });
            }
            else if (!CutsceneFOVScanResult)
            {
                spdlog::error("Cutscene FOV: Pattern scan failed.");
            }
        } 
    }
}

void HUDFix()
{
    if (bHUDFix)
    {
        if (fNativeAspect < fAspectRatio)
        {
            // Force 16:9 UI
            uint8_t* UIAspectScanResult = Memory::PatternScan(baseModule, "39 ?? ?? ?? ?? ?? 75 ?? 48 ?? ?? ?? ?? ?? ?? 39 ?? ?? ?? ?? ?? 0F ?? ?? ?? ?? ??");
            if (UIAspectScanResult)
            {
                spdlog::info("UI: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)UIAspectScanResult - (uintptr_t)baseModule);

                // Patch jnz so it goes to the part of code we want to modify
                Memory::PatchBytes((uintptr_t)UIAspectScanResult + 0x6, "\xEB", 1);

                static SafetyHookMid UIAspectMidHook{};
                UIAspectMidHook = safetyhook::create_mid(UIAspectScanResult + 0x1F,
                    [](SafetyHookContext& ctx)
                    {
                        ctx.xmm0.f32[0] = iCustomResY * fNativeAspect;
                    });
            }
            else if (!UIAspectScanResult)
            {
                spdlog::error("UI: Pattern scan failed.");
            }

            // Fix markers being off
            uint8_t* UIMarkersScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? C5 ?? ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? ?? 41 ?? ?? ?? ?? ?? 00 01");
            if (UIMarkersScanResult)
            {
                spdlog::info("UI Markers: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)UIMarkersScanResult - (uintptr_t)baseModule);

                static SafetyHookMid UIMarkersMidHook{};
                UIMarkersMidHook = safetyhook::create_mid(UIMarkersScanResult + 0x1F,
                    [](SafetyHookContext& ctx)
                    {
                        *reinterpret_cast<float*>(ctx.rax + 0x1F4) = (float)2160 * fAspectRatio;
                    });
            }
            else if (!UIMarkersScanResult)
            {
                spdlog::error("UI Markers: Pattern scan failed.");
            }
        }
        }
}

void FPSCap()
{
    if (bFPSCap)
    {
        // Set FPS cap
        uint8_t* FPSCapScanResult = Memory::PatternScan(baseModule, "4C ?? ?? FF ?? 48 ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? ?? ?? ?? ?? C4 ?? ?? ?? ??");
        if (FPSCapScanResult)
        {
            spdlog::info("FPS Cap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FPSCapScanResult - (uintptr_t)baseModule);

            static SafetyHookMid FPSCapMidHook{};
            FPSCapMidHook = safetyhook::create_mid(FPSCapScanResult + 0x5,
                [](SafetyHookContext& ctx)
                {
                    // Menus seem to speed up beyond 240fps.
                    ctx.xmm6.f64[0] = (double)1 / 240;
                });
        }
        else if (!FPSCapScanResult)
        {
            spdlog::error("FPS Cap: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    CustomResolution();
    AspectFOVFix();
    HUDFix();
    FPSCap();
    return true;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST); // set our Main thread priority higher than the games thread
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

