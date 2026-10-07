# XenonFramework
Windows Desktop Application Development Framework

Change project setting
C++20
C17

Rename app.cpp to .ixx

Clone the project:
``` git clone --recurse-submodules git@github.com:snorrikris/XenonFramework.git ```

Don't open in VS just yet, first we need to get the Scintilla source code.

Download Scintilla source code from https://www.scintilla.org/ScintillaDownload.html

Extract the Scintilla source code to the `XenonFramework/scintilla` directory.

Now you can open the solution in VS.

Note - the Scintilla project - in the XenonDemo solution - should be present (at XenonFramework\scintilla\win32\Scintilla.vcxproj).

Add this to D:\_Dev\XenonFramework\scintilla\include\Scintilla.h - approx line 1352 (before struct Sci_CharacterRange):
```
#define WMU_SCI_SETSCROLLINFO 3334
#define WMU_SCI_GETSCROLLINFO 3335
```

Download boost from https://www.boost.org/releases/latest/ and install it to C:\_Dev\boost_xxxx.
E.g.: https://archives.boost.io/release/1.92.0/source/boost_1_92_0.zip
Create hard link to C:\_Dev to the boost install folder.
Open command promt (change "boost_1_90_0" to whatever the installed version is):
```
cd C:\_Dev
mklink /J boost_latest boost_1_90_0
```
Note - the XenonDemo project settings assume boost is accessed from C:\_Dev\boost_latest.

TODO: (suggested by Google AI)

### Try module :private (to improve compile times)
E.g.:

```
export module Xe.Menu;
import std;

export class CXeMenu {
public:
    void UpdateLayout(); // Only the signature is visible to importers
};

// 🟢 EVERYTHING BELOW THIS LINE IS TREATED LIKE A .CPP FILE 
// Modifying this code will NOT trigger a rebuild of files importing Xe.Menu!
module :private; 

void CXeMenu::UpdateLayout() {
    // Massive implementation block here
}
```
The Impact: Changing a line of code inside the private fragment will only recompile XeMenu.ixx.obj. It completely stops the dependency chain from rebuilding UGCtrl.ixx, XeTabsView.ixx, etc.


Note: If you rely heavily on the Windows SDK (<Windows.h>), wrap it tightly inside the global module fragment of only the modules that absolutely require it, or build a local named module wrapper for Win32 types.


Try to use import std; instead of #include 


### Fine-Tune MSVC Project Compilation Switches
Visual Studio 2026 introduces and refines several compiler flags specifically designed to optimize module throughput.
Right-click your project, go to Properties > C/C++ > Command Line, and ensure or experiment with these settings:

• /MP (Build with Multiple Processes): Ensure this is enabled. MSVC uses a sophisticated dependency graph allocator to compile independent modules concurrently.


• /experimental:module vs. Native Modules: Ensure you are using the native implementation. In VS 2026, ensure C++ Language Standard is set to /std:c++20 or /std:c++latest, which automatically optimizes the internal AST caching engine.


• Turn off Scan Sources for Modules on Standard Files: If your project contains a mix of traditional .cpp/.h files and modules, go to Project Properties > C/C++ > General > Scan Sources for Module Dependencies and set it to Only Modules or optimize it so MSVC doesn't waste time scanning plain C++ files for module keywords.


### Break Down Mega-Modules into Implementation Partitions
If a module becomes exceptionally large (e.g., thousands of lines of implementation code), the single file compilation time can drag down your iteration speed, even when using a private fragment.
You can break a single module across multiple files using internal implementation partitions. These partitions are hidden from consumers but let you distribute the compilation workload across multiple CPU cores.

• XeTheme-Impl.ixx (Internal Partition):
```
module Xe.Theme:Impl; // Internal partition (no 'export' before module)
import Xe.ThemeIF;

// Put a chunk of your complex virtual method implementations here
```
• XeTheme.ixx (Primary Module Interface):
```
export module Xe.Theme;
import :Impl; // Merges the compiled implementation chunks seamlessly

export class XeTheme : public XeThemeIF { ... };
```
