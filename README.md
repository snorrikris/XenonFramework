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

