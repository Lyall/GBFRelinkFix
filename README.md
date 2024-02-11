# Granblue Fantasy: Relink Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)</br>
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/GBFRelinkFix/total.svg)](https://github.com/Lyall/GBFRelinkFix/releases)

This is a fix that adds custom resolutions, ultrawide support and more to Granblue Fantasy: Relink.<br />

## Features
- Custom resolution/ultrawide support.
- Correct FOV and aspect ratio at any resolution.
- Gameplay FOV/camera distance modification.
- Scales HUD to 16:9.
- Spanned HUD backgrounds to fill screen (loading fades etc).
- Option to span gameplay HUD to edges of the screen. (Check ini to enable)
- Graphical tweaks (Shadow Quality/LOD Distance/Disable TAA).
- Experimental option to raise FPS cap to 240. (Check ini to enable)

## Installation
- Grab the latest release of GBFRelinkFix from [here.](https://github.com/Lyall/GBFRelinkFix/releases)
- Extract the contents of the release zip in to the the game folder.<br />(e.g. "**steamapps\common\Granblue Fantasy Relink**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.

## Configuration
- See **GBFRelinkFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

- Raising FPS cap can cause issues with physics. [#2](https://github.com/Lyall/GBFRelinkFix/issues/2)
  
## Screenshots

| ![ezgif-1-94aa803391](https://github.com/Lyall/GBFRelinkFix/assets/695941/43e186f0-0e4d-4ace-a0c9-300f3171e414)|
|:--:|
| Gameplay with the fix. |

## Credits
Many thanks to VagrantZero on the WSGF Discord for providing a copy of the game! 
<br/>
Also thanks to KingKrouch and nikos on the WSGF Discord for doing valuable testing.

[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
