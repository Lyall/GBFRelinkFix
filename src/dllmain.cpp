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
string sFixVer = "1.0.3";
string sLogFile = "GBFRelinkFix.log";
string sConfigFile = "GBFRelinkFix.ini";
string sExeName;
filesystem::path sExePath;
RECT rcDesktop;

// Ini Variables
int iInjectionDelay;
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
float fFOVMulti;
float fCamDistMulti;
bool bHUDFix;
bool bSpanHUD;
bool bAspectFix;
bool bFOVFix;
bool bShadowQuality;
int iShadowQuality;
float fLODMulti;
bool bDisableTAA;
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
    inipp::get_value(ini.sections["GBFRelinkFix Parameters"], "InjectionDelay", iInjectionDelay);
    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomResolution);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Gameplay FOV"], "Multiplier", fFOVMulti);
    inipp::get_value(ini.sections["Gameplay Camera Distance"], "Multiplier", fCamDistMulti);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bHUDFix);
    inipp::get_value(ini.sections["Span HUD"], "Enabled", bSpanHUD);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    inipp::get_value(ini.sections["Shadow Quality"], "Enabled", bShadowQuality);
    inipp::get_value(ini.sections["Shadow Quality"], "Value", iShadowQuality);
    inipp::get_value(ini.sections["Level of Detail"], "Multiplier", fLODMulti);
    inipp::get_value(ini.sections["Disable TAA"], "Enabled", bDisableTAA);
    inipp::get_value(ini.sections["Raise Framerate Cap"], "Enabled", bFPSCap);

    // Log config parse
    spdlog::info("Config Parse: iInjectionDelay: {}ms", iInjectionDelay);
    spdlog::info("Config Parse: bCustomResolution: {}", bCustomResolution);
    spdlog::info("Config Parse: iCustomResX: {}", iCustomResX);
    spdlog::info("Config Parse: iCustomResY: {}", iCustomResY);
    spdlog::info("Config Parse: fFOVMulti: {}", fFOVMulti);
    if (fFOVMulti < (float)0.1 || fFOVMulti > (float)2.5)
    {
        fFOVMulti = std::clamp(fFOVMulti, (float)0.1, (float)2.5);
        spdlog::info("Config Parse: fFOVMulti value invalid, clamped to {}", fFOVMulti);
    }
    spdlog::info("Config Parse: fCamDistMulti: {}", fCamDistMulti);
    if (fCamDistMulti < (float)0.1 || fCamDistMulti >(float)2.5)
    {
        fCamDistMulti = std::clamp(fCamDistMulti, (float)0.1, (float)2.5);
        spdlog::info("Config Parse: fCamDistMulti value invalid, clamped to {}", fCamDistMulti);
    }
    spdlog::info("Config Parse: bHUDFix: {}", bHUDFix);
    spdlog::info("Config Parse: bSpanHUD: {}", bSpanHUD);
    spdlog::info("Config Parse: bAspectFix: {}", bAspectFix);
    spdlog::info("Config Parse: bFOVFix: {}", bFOVFix);
    spdlog::info("Config Parse: bShadowQuality: {}", bShadowQuality);
    spdlog::info("Config Parse: iShadowQuality: {}", iShadowQuality);
    if (iShadowQuality < 256 || iShadowQuality > 8192)
    {
        iShadowQuality = std::clamp(iShadowQuality, 128, 16384);
        spdlog::info("Config Parse: iShadowQuality value invalid, clamped to {}", iShadowQuality);
    }
    spdlog::info("Config Parse: fLODMulti: {}", fLODMulti);
    if (fLODMulti < (float)0.1 || fLODMulti >(float)10)
    {
        fLODMulti = std::clamp(fLODMulti, (float)0.1, (float)10);
        spdlog::info("Config Parse: fLODMulti value invalid, clamped to {}", fLODMulti);
    }
    spdlog::info("Config Parse: bDisableTAA: {}", bDisableTAA);
    spdlog::info("Config Parse: bFPSCap: {}", bFPSCap);
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

void ApplyResolution()
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
    }
}

void GraphicalFixes()
{
    if (bCustomResolution)
    {
        // Fix graphical corruption when resolution width is not divisible by 32
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

        // Screen Effects
        uint8_t* ScreenEffectsScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? 48 ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? 00 C3") + 0xB;
        if (ScreenEffectsScanResult)
        {
            spdlog::info("Screen Effects: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ScreenEffectsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid ScreenEffectsMidHook{};
            ScreenEffectsMidHook = safetyhook::create_mid(ScreenEffectsScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio > fNativeAspect)
                    {
                        ctx.xmm0.f32[0] = fAspectMultiplier;
                    }
                    else if (fAspectRatio < fNativeAspect)
                    {

                    }
                });

        }
        else if (!ScreenEffectsScanResult)
        {
            spdlog::error("Screen Effects: Pattern scan failed.");
        }
    }
}

void AspectFOVFix()
{
    if (bAspectFix && (fNativeAspect > fAspectRatio))
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

    // Gameplay FOV
    uint8_t* GameplayFOVScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? ?? 00 80 ?? ?? 00 74 ??");
    if (GameplayFOVScanResult)
    {
        spdlog::info("Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayFOVScanResult - (uintptr_t)baseModule);

        static SafetyHookMid GameplayFOVMidHook{};
        GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult,
            [](SafetyHookContext& ctx)
            {
                // Fix gameplay FOV at <16:9
                if (bFOVFix && (fNativeAspect > fAspectRatio))
                {
                    ctx.xmm10.f32[0] /= fAspectMultiplier;
                }

                // Run FOV multiplier
                if (fFOVMulti != (float)1)
                {
                    ctx.xmm10.f32[0] *= fFOVMulti;
                }
            });
    }
    else if (!GameplayFOVScanResult)
    {
        spdlog::error("Gameplay FOV: Pattern scan failed.");
    }

    if (fCamDistMulti != (float)1)
    {
        // Gameplay Camera Distance
        uint8_t* GameplayCameraDistScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? ?? 48 ?? ?? 74 ??");
        if (GameplayCameraDistScanResult)
        {
            spdlog::info("Gameplay Camera Distance: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayCameraDistScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GameplayCameraDistMidHook{};
            GameplayCameraDistMidHook = safetyhook::create_mid(GameplayCameraDistScanResult + 0x8,
                [](SafetyHookContext& ctx)
                {
                    // Run camera distance multiplier
                    ctx.xmm9.f32[0] *= fCamDistMulti;
                });
        }   
        else if (!GameplayCameraDistScanResult)
        {
            spdlog::error("Gameplay Camera Distance: Pattern scan failed.");
        }
    }

    if (bFOVFix && (fNativeAspect > fAspectRatio))
    {
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

void HUDFix()
{
    if (bHUDFix)
    {
        // Don't do this if the aspect ratio is wider than native
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
        }

        // Fix markers being off
        uint8_t* UIMarkersScanResult = Memory::PatternScan(baseModule, "48 ?? ?? 20 48 ?? ?? 49 ?? ?? C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? 00") + 0xA;
        if (UIMarkersScanResult)
        {
            spdlog::info("UI Markers: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)UIMarkersScanResult - (uintptr_t)baseModule);

            static SafetyHookMid UIMarkersMidHook{};
            UIMarkersMidHook = safetyhook::create_mid(UIMarkersScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (fAspectRatio < fNativeAspect)
                    {
                        *reinterpret_cast<float*>(ctx.rcx + 0x1F8) = (float)2160 + fHUDHeightOffset;
                    }
                    else if (fAspectRatio > fNativeAspect)
                    {
                        *reinterpret_cast<float*>(ctx.rcx + 0x1F4) = (float)2160 * fAspectRatio;
                    }
                });            
        }
        else if (!UIMarkersScanResult)
        {
            spdlog::error("UI Markers: Pattern scan failed.");
        }

        // Span backgrounds
        uint8_t* UIBackgroundsScanResult = Memory::PatternScan(baseModule, "41 ?? ?? ?? ?? 00 E8 ?? ?? ?? ?? 80 ?? ?? ?? 00 0F ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? 48 ?? ?? E8 ?? ?? ?? ?? 48 ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? 00") + 0x2F;
        if (UIBackgroundsScanResult)
        {
            spdlog::info("UI Backgrounds: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)UIBackgroundsScanResult - (uintptr_t)baseModule);
            
            if (fAspectRatio > fNativeAspect)
            {
                static SafetyHookMid UIBackgroundsWidthMidHook{};
                UIBackgroundsWidthMidHook = safetyhook::create_mid(UIBackgroundsScanResult,
                    [](SafetyHookContext& ctx)
                    {

                        // If it is 3840px wide then it must span the entire screen
                        if (*reinterpret_cast<float*>(ctx.rax + 0x1F4) == (float)3840)
                        {
                            // Fade to black = 1932007245 | Pause screen bg = 1611295806 | Dialogue bg = 2454207042  | Title menu bg = 4291119775
                            // Main menu bg = 2384707215  | Lyria's journal = 3818795736 | Load save bg = 3969399384 | Title fade white = 1646463024
                            // Main menu transition bg = 2056445562 | Title menu fade black = 3970768321 | Title options bg 1 = 603087221 | Title options bg 2 = 61148732
                            if (*reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1932007245
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1611295806
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)2454207042
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)4291119775
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)2384707215
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3818795736
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3969399384
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1646463024
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)2056445562
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3970768321
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)61148732
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)603087221)
                            {
                                ctx.xmm0.f32[0] = (float)2160 * fAspectRatio;
                            }
                        }
                    });

            }
            else if (fAspectRatio < fNativeAspect)
            {
                static SafetyHookMid UIBackgroundsHeightMidHook{};
                UIBackgroundsHeightMidHook = safetyhook::create_mid(UIBackgroundsScanResult + 0x28,
                    [](SafetyHookContext& ctx)
                    {
                        // If it is 3840px wide then it must span the entire screen
                        if (*reinterpret_cast<float*>(ctx.rax + 0x1F4) == (float)3840)
                        {
                            // Fade to black = 1932007245 | Pause screen bg = 1611295806 | Dialogue bg = 2454207042  | Title menu bg = 4291119775
                            // Main menu bg = 2384707215  | Lyria's journal = 3818795736 | Load save bg = 3969399384 | Title fade white = 1646463024
                            // Main menu transition bg = 2056445562 | Title menu fade black = 3970768321 | Title options bg 1 = 603087221 | Title options bg 2 = 61148732
                            if (*reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1932007245
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1611295806
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)2454207042
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)4291119775
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)2384707215
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3818795736
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3969399384
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1646463024
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)2056445562
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3970768321
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)61148732
                                || *reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)603087221)
                            {
                                ctx.xmm4.f32[0] = (float)3840 / fAspectRatio;
                            }
                        }
                    });
            }
        }
        else if (!UIBackgroundsScanResult)
        {
            spdlog::error("UI Backgrounds: Pattern scan failed.");
        }
        
    }

    if (bSpanHUD)
    {
        // Spanned HUD
        uint8_t* HUDConstraintsScanResult = Memory::PatternScan(baseModule, "48 ?? ?? ?? ?? ?? 00 48 ?? ?? 74 ?? C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? 00 C5 ?? ?? ?? ?? ?? ?? 00 EB ??") + 0x1C;
        if (HUDConstraintsScanResult)
        {
            spdlog::info("UI Constraints: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDConstraintsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HUDConstraintsMidHook{};
            HUDConstraintsMidHook = safetyhook::create_mid(HUDConstraintsScanResult,
                [](SafetyHookContext& ctx)
                {
                    // Gameplay HUD = 1719602056
                    if (*reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)1719602056)
                    {
                        // Span
                        if (fAspectRatio > fNativeAspect)
                        {
                            ctx.xmm2.f32[0] = (float)2160 * fAspectRatio;
                        }
                        else if (fAspectRatio < fNativeAspect)
                        {
                            ctx.xmm0.f32[0] = (float)3840 / fAspectRatio;
                        }
                    }
                    // Guard & Lock-On
                    if (*reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)605904162)
                    {
                        // Offset
                        if (fAspectRatio > fNativeAspect)
                        {
                            *reinterpret_cast<float*>(ctx.rax + 0x1CC) = (float)-(((2160 * fAspectRatio) - 3840) / 2);
                        }
                        else if (fAspectRatio < fNativeAspect)
                        {
                            *reinterpret_cast<float*>(ctx.rax + 0x1D0) = (float)-(((3840 / fAspectRatio) - 2160) / 2);
                        }     
                    }
                    // Dodge
                    if (*reinterpret_cast<int*>(ctx.rax + 0x1FC) == (int)3550204025)
                    {
                        // Offset
                        if (fAspectRatio > fNativeAspect)
                        {
                            *reinterpret_cast<float*>(ctx.rax + 0x1CC) = (float)(((2160 * fAspectRatio) - 3840) / 2);
                        }
                        else if (fAspectRatio < fNativeAspect)
                        {
                            *reinterpret_cast<float*>(ctx.rax + 0x1D0) = (float)-(((3840 / fAspectRatio) - 2160) / 2);
                        }
                    }
                });
        }
        else if (!HUDConstraintsScanResult)
        {
            spdlog::error("UI Constraints: Pattern scan failed.");
        }
    }
}

void GraphicalTweaks()
{
    if (bShadowQuality)
    {
        // Set FPS cap
        uint8_t* ShadowQualityScanResult = Memory::PatternScan(baseModule, "8B ?? ?? ?? C4 ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? 00");
        if (ShadowQualityScanResult)
        {
            spdlog::info("Shadow Quality: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ShadowQualityScanResult - (uintptr_t)baseModule);

            static SafetyHookMid ShadowQualityMidHook{};
            ShadowQualityMidHook = safetyhook::create_mid(ShadowQualityScanResult,
                [](SafetyHookContext& ctx)
                {
                    *reinterpret_cast<int*>(ctx.rcx + (ctx.rdx + 0x4)) = iShadowQuality;
                    *reinterpret_cast<int*>(ctx.rcx + (ctx.rdx + 0x8)) = iShadowQuality;
                });
        }
        else if (!ShadowQualityScanResult)
        {
            spdlog::error("Shadow Quality: Pattern scan failed.");
        }
    }

    if (fLODMulti != (float)1)
    {
        uint8_t* LODDistanceScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? 00 C6 ?? ?? ?? ?? 00 01 83 ?? ?? 00 0F ?? ?? ?? ?? 00 31 ??");
        if (LODDistanceScanResult)
        {
            spdlog::info("Level of Detail: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)LODDistanceScanResult - (uintptr_t)baseModule);

            static SafetyHookMid LODDistanceMidHook{};
            LODDistanceMidHook = safetyhook::create_mid(LODDistanceScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm1.f32[0] *= fLODMulti;
                });
        }
        else if (!LODDistanceScanResult)
        {
            spdlog::error("Level of Detail: Pattern scan failed.");
        }
    }

    if (bDisableTAA)
    {
        // Disable TAA
        uint8_t* TemporalAAScanResult = Memory::PatternScan(baseModule, "0F ?? ?? ?? 88 ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? 48 ?? ?? ?? 5E");
        if (TemporalAAScanResult)
        {
            spdlog::info("Temporal AA: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)TemporalAAScanResult - (uintptr_t)baseModule);
            // xor ecx, ecx
            Memory::PatchBytes((uintptr_t)TemporalAAScanResult, "\x31\xC9\x90\x90", 4);
            spdlog::info("Temporal AA: Patched instruction to disable TAA.");
        }
        else if (!TemporalAAScanResult)
        {
            spdlog::error("Temporal AA: Pattern scan failed.");
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
    ApplyResolution();
    Sleep(iInjectionDelay);
    GraphicalFixes();
    AspectFOVFix();
    HUDFix();
    GraphicalTweaks();
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

