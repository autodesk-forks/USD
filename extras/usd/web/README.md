# What is UsdWeb?

The **UsdWeb** example expands on the simple 
[USDViewWeb](/pxr/usdImaging/bin/usdviewweb) application
to initialize a USD Hydra Storm renderer in a web viewport while exposing USD javascript that can be controlled via the html or js files.

## Limitations
 - MaterialX enhancement for webgpu is under review and targeted for 1.39.4.  The build_usd.py script has been updated to use a fork in the meantime.
 - MaterialX changed in 1.39.0, and some of the more complex shaders (like procedural brick) is currently not rendering as expected.  We are waiting on the parent branch to be updated to the latest USD dev/ branch to see if that fixes it.  It was working with MaterialX 1.38.10.

## Plans
 - [x] Port FreeCamera from usdview python into C++ for better manipulation.
 - [x] Expand USD API commands exposed to JavaScript (variants, layers, visibility, fit camera, etc.)
 - [x] Prototype MaterialX enhancements needed to generate WGSL-compliant GLSL.
 - [ ] Integrate webgpu enhancements into MaterialX 1.39.4.
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