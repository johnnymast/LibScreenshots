# Windows Build Tutorial (CLion + Visual Studio + vcpkg) — LibScreenshots

Deze handleiding legt uit hoe je LibScreenshots correct bouwt op Windows met:

- CLion
- Visual Studio 2022 Community
- MSVC v143
- Windows SDK (automatisch geïnstalleerd)
- vcpkg (x64-windows)
- Visual Studio generator (GEEN Ninja)

Dit is de enige configuratie die 100% stabiel werkt voor Windows-native screenshot backends.

------------------------------------------------------------
1. Installeer Visual Studio 2022 Community
------------------------------------------------------------

Open de Visual Studio Installer en vink slechts één workload aan:

    ✔ Desktop development with C++

Dit installeert automatisch:

- MSVC v143 compiler
- Windows SDK (10 of 11)
- CMake tools
- NMake Makefiles
- Visual Studio generator
- Alle benodigde headers en libs

Je hoeft geen individuele SDK’s meer te selecteren.

------------------------------------------------------------
2. Installeer vcpkg (optioneel)
------------------------------------------------------------

LibScreenshots heeft geen externe dependencies nodig, maar vcpkg is handig als je later uitbreidt.

    git clone https://github.com/microsoft/vcpkg
    cd vcpkg
    bootstrap-vcpkg.bat

Je hoeft geen libraries te installeren voor LibScreenshots.

------------------------------------------------------------
3. Configureer CLion Toolchain
------------------------------------------------------------

Ga naar:

    File → Settings → Build, Execution, Deployment → Toolchains

Selecteer:

- Toolchain: Visual Studio
- C Compiler: cl.exe
- C++ Compiler: cl.exe
- Debugger: bundled
- Environment: leeg laten

CLion detecteert automatisch:

- Windows SDK
- MSVC toolchain

⚠ BELANGRIJK:
CLion gebruikt standaard Ninja, maar Ninja veroorzaakt op Windows timestamp-bugs.
Daarom gebruiken we de Visual Studio generator.

------------------------------------------------------------
4. Configureer CMake in CLion
------------------------------------------------------------

Ga naar:

    Settings → Build, Execution, Deployment → CMake

Stel in:

- Generator: Visual Studio 17 2022
- Toolchain: Visual Studio
- CMake options (alleen nodig als je vcpkg gebruikt):

      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

------------------------------------------------------------
5. Verwijder oude build-mappen
------------------------------------------------------------

Verwijder:

    cmake-build-debug/
    cmake-build-release/
    cmake-build-debug-visual-studio/
    build/   (als je die handmatig hebt gemaakt)

Dit voorkomt conflicten tussen Ninja en Visual Studio.

------------------------------------------------------------
6. Builden
------------------------------------------------------------

Klik:

- Reload CMake Project
- Build

Je krijgt:

- ✔ Windows backend geselecteerd
- ✔ MSVC toolchain gevonden
- ✔ Windows SDK gevonden
- ✔ LibScreenshots.lib gebouwd
- ✔ LibScreenshots.dll gebouwd (bij SHARED build)

------------------------------------------------------------
7. Install (Als Administrator)
------------------------------------------------------------

Ga naar de CLion build-map:

    cd LibScreenshots/cmake-build-debug-visual-studio

Voer uit:

    cmake --install . --prefix "C:/Program Files/LibScreenshots"

Dit installeert:

- C:/Program Files/LibScreenshots/bin/LibScreenshots.dll
- C:/Program Files/LibScreenshots/lib/LibScreenshots.lib
- C:/Program Files/LibScreenshots/include/LibScreenshots/...
- C:/Program Files/LibScreenshots/lib/cmake/LibScreenshots/LibScreenshotsConfig.cmake

------------------------------------------------------------
Klaar
------------------------------------------------------------

Je hoeft dus geen Windows SDK handmatig te kiezen.
Je hoeft geen Desktop SDK meer te zoeken.
Je hoeft geen Ninja te gebruiken (sterker nog: niet doen).
Je hoeft geen individuele componenten meer aan te vinken.

Alleen “Desktop development with C++” is genoeg.
