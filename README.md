# BrainBread

BrainBread is a cooperative zombie survival mod for Valve's
[Half-Life](https://store.steampowered.com/app/70/HalfLife/) (GoldSrc engine)
where players fight together against hordes of zombies. Features include an
arsenal of over 20 weapons, a gore system, AI-controlled military allies, and
the risk of turning into a zombie yourself when bitten.

The codebase is derived from **Public-Enemy**, our earlier Half-Life mod.
Files using the `pe_` prefix originate from that project.

For more information visit the BrainBread website:
**<https://ironoak.ch/BB/>**

## Building

The project includes Visual Studio project files (`.vcproj` / `.vcxproj`) for
building on Windows and `Makefile`s for building on Linux (i386).

- **Server DLL:** Open `dlls/mp.sln` or use `dlls/Makefile` to build the game
  server library (`bb.dll` / `bb.so`).
- **Client DLL:** Open `cl_dll/cl_dll.vcproj` / `cl_dll/cl_dll.vcxproj` to
  build the client library.

A working installation of the
[Half-Life SDK](https://github.com/ValveSoftware/halflife) is required for the
engine headers and libraries.

## Project Structure

```
cl_dll/         Client-side DLL (HUD, rendering, input, VGUI menus)
common/         Shared engine/game headers
dedicated/      Dedicated server (hlds) front-end
dlls/           Server-side game DLL
  sqlite/       Embedded SQLite database (public domain)
engine/         GoldSrc engine interface headers
game_shared/    Code shared between client and server
pm_shared/      Player movement prediction code (shared client/server)
utils/          Half-Life SDK map compile tools and utilities
```

Custom mod code uses the `pe_` (Public-Enemy) and `bb_` (BrainBread) prefixes.

## License

This project contains code from multiple sources with different license terms:

- **Half-Life SDK code** -- The majority of files are based on the Half-Life SDK
  by Valve and are Copyright (c) 1996-2002, Valve LLC. This code is subject to
  the Half-Life SDK License, which restricts use to non-commercial enhancements
  to products from Valve LLC. See the individual file headers for details.

- **Custom mod code** (`pe_*`, `bb_*` files) -- Created by IronOak Studios.
  This code is subject to the same Half-Life SDK License terms as the base SDK
  code it builds upon.

- **SQLite** (`dlls/sqlite/`) -- Public domain. No restrictions.

- **OpenGL extension headers** (`cl_dll/glext/`) -- Copyright (c) 2013 The
  Khronos Group Inc. Licensed under a permissive MIT-style license. See the
  file headers for the full license text.

This source code is provided for educational and modding purposes. Commercial
use is prohibited under the terms of the Half-Life SDK License.

## Trademarks

**BrainBread** is a trademark of IronOak Studios.
**Half-Life** and **Valve** are trademarks of Valve Corporation.
