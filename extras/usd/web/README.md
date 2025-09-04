# What is UsdWeb?

The **UsdWeb** example expands on the simple 
[USDViewWeb](/pxr/usdImaging/bin/usdviewweb) application
to initialize a USD Hydra Storm renderer in a web viewport while exposing USD javascript that can be controlled via the html or js files.

## Limitations
TBD

## Plans
 - [x] Enable HdStorm C++ rendering with access to USD JavaScript commands.
 - [x] Port FreeCamera from usdview python into C++ for better manipulation.
 - [x] Expand USD API commands exposed to JavaScript (variants, layers, visibility, fit camera, etc.)
 - [x] Prototype MaterialX enhancements needed to generate WGSL-compliant GLSL.
 - [x] Integrate webgpu enhancements into MaterialX 1.39.4 main branch.
 - [ ] Simplify usdweb.cpp and usdweb.html to remove the dependency on GL and blitting textures as a post process.
 - [ ] Integrate into USD dev/ branch.

# How to build and deploy?
See basic instructions under [pxr/usdImaging/bin/usdviewweb/README.md](/pxr/usdImaging/bin/usdviewweb/README.md).

**Build:** (using ./build-wasm-release as the <build_dir>)
```
cd <root_dir_of_repo>
python3 ./build_scripts/build_usd.py --build-target wasm --build-variant release ./build-wasm-release
```

**Execute:**
```
cd <build_dir>/bin
emrun --browser chrome usdweb.html 
```

**Execute (alternative):**
```
cd <build_dir>/bin
python3 ./wasm-server.py
```

# How to use?
See [usdweb.html](usdweb.html)
<br>or<br> 
from the Chrome Developer Console, type `Module` to see the commands exposed.

# Implementation Notes
## Useful functions in usdweb.html

Here are some Javascript functions in `usdweb.html` worth highlighting:
- **fetchFileToFS()**
Lower level function that will load a file into the virtual filesystem.  It works with local files as well as http and github.com.

- **openStageFromUsdfile()**
Loads the specified file into the virtual filesystem and then loads it.  Calls fetchFileToFS().

- **loadscene_with_deps_and_actions()**
Higher-level function that loads the USD stage, allows for specifying additional dependencies to fetch to the virtual filesystem.  It also has a callback hook so one can execute a script after the USD stage has loaded.

- **downloadTextFileFromFS()**
This allows one to download a resulting text file (often a logfile) from the virtual system to the main filesystem.