Additional Requirements
-----------------------

Node version 18.0.0^
Emscripten version 4.0.8

Setup
-----

Build USD with Emscripten for NodeJS (in the root of this repository):

locally
```sh
python build_scripts/build_usd.py --build-target node <build_folder>
```
or in a Docker container
```sh
docker build --build-arg BUILD_TARGET="--build-target node" -t <CONTAINER_TAG> .
docker run -it -w //src <CONTAINER_TAG>
```

and run

```sh
cd js
npm install
```

in this `js` folder.

Tests
------

Run

```sh
npm run test
```

or in watch mode

```sh
npm run test --  --watch
```

Benchmarking
------------

You can launch the benchmark script by running:

```sh
npm run test:playwright:benchmarks
```

Ensure that USD has been built using the `build_usd.py` Python script or with CMake, including the install step. This process will create a folder under `js` called `usdviewweb/`, which contains the application used for benchmarking.

To configure parameters, copy the `default_env` file to `.env` and modify it with the appropriate values.

```sh
cp default_env .env
```

To view the results, run:

```sh
npx playwright show-report tests-playwright/.reports/html
```

NPM package consumption
------------------------

Currently, we only support consumption of the bindings via Script tags in the browser or via Node.js

If you are using Webpack you can add the bindings to your application with

```
  externals: {
    "usd": 'usd', // indicates global variable
  },
  plugins: [
    new CopyPlugin({
      patterns: [
        { from: "<PATH TO BINDINGS>" },
      ],
    }),
  ],
```

and after adding `<script src="jsBindings.js"></script>` to your HTML page use it in your code with

```
    <script src="jsBindings.js"></script>
    <script type="module">
      const Usd = await usdModule();
      const UsdStage = Usd.UsdStage;
      let stage = UsdStage.CreateNew('HelloWorld.usda');
    </script>
```

In Node.Js you can load it via 
```
const usdModule = require("usd");
const Usd = await usdModule();
const UsdStage = Usd.UsdStage;

let stage = UsdStage.CreateNew('HelloWorld.usda');
...
```
