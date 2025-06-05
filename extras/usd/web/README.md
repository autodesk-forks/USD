# What is UsdWeb?

The **UsdWeb** example expands on the simple 
[USDViewWeb](/pxr/usdImaging/bin/usdviewweb) application
to initialize a USD Hydra Storm renderer in a web viewport while exposing USD javascript that can be controlled via the html or js files.

# How to build and deploy?
See basic instructions under [pxr/usdImaging/bin/usdviewweb/README.md](/pxr/usdImaging/bin/usdviewweb/README.md).

**Build:** (using ./build-wasm-release as the <build_dir>)
cd <root_dir_of_repo>
```
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