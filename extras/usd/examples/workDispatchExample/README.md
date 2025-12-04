workDispatchExample
===================
`workDispatchExample` is an example demonstrating how to create and build USD
against a custom task management implementation. It is an example only and is
unsupported for general use.

`workDispatchExample` is based on the libdispatch, which is builtin on all Darwin
platforms and can be found at 
https://swiftlang.github.io/swift-corelibs-libdispatch/. Note that this was built 
and tested only on a Darwin machine.

## Building `workDispatchExample`

Run cmake to configure and build the library. With the example command line
below, its headers and shared library will be installed under
`/path/to/install`.

```
cmake --install-prefix /path/to/install -B /path/to/build -S OpenUSD/pxr/extras/usd/examples/workDispatchExample
cmake --build /path/to/build --target install --config Release
```

## Building USD with `workDispatchExample`

Specify `workDispatchExample` as the custom task management implementation
by setting the `PXR_WORK_IMPL` option to `workDispatchExample` when running
cmake. The `workDispatchExample_DIR` option must also be set to the directory
containing the package config file named `workDispatchExampleConfig.cmake`
that was installed as part of the build above.

```
cmake -DPXR_WORK_IMPL=workDispatchExample -DworkDispatchExample_DIR=/path/to/install/lib64/cmake/workDispatchExample ...
```

If you are building USD with build_usd.py instead of using cmake directly,
these arguments can be passed through using `--build-args`:

```
build_usd.py --build-args USD,"-DPXR_WORK_IMPL=workDispatchExample -DworkDispatchExample_DIR=/path/to/install/lib64/cmake/workDispatchExample" ...
```
