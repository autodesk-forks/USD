const express = require('express');
const path = require('path');
const { config } = require('dotenv');
const fs = require('fs');

config({ override: true });

const DEFAULT_PORT = 8000;
const USDVIEWWEB_LOCAL_PORT = Number(process.env.USDVIEWWEB_LOCAL_PORT);
const port = !Number.isNaN(USDVIEWWEB_LOCAL_PORT) && USDVIEWWEB_LOCAL_PORT > 0 ? USDVIEWWEB_LOCAL_PORT : DEFAULT_PORT;
const modelsPath = process.env.BENCHMARK_MODELS_FOLDER;

const APP_FOLDER = path.join(__dirname, '../../usdviewweb');

if (!fs.existsSync(APP_FOLDER)) {
  console.error(`Error: App folder doesnt exist. Please run cmake, including install step`);
  process.exit(1);
}

if (!modelsPath || !fs.existsSync(modelsPath)) {
  console.error(`Error: Models folder doesnt exist or not set. Please set env variable BENCHMARK_MODELS_FOLDER`);
  process.exit(1);
}

const app = express();

// Middleware - Enable CORS for testing through Ninja and other localhost environments
app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", req.headers.origin ? req.headers.origin : "*");
  res.header("Access-Control-Allow-Credentials", "true");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Range");
  res.header('Cross-Origin-Opener-Policy', 'same-origin');
  res.header('Cross-Origin-Embedder-Policy', 'require-corp');
  if (req.method == "OPTIONS") {
    res.sendStatus(200);
  } else {
    next();
  }
});

app.use('/', express.static(process.cwd(), {
  setHeaders: (res, path) => {
    if (useGzip(path)) res.header('Content-Encoding', 'gzip');
  }
}));

app.use('/', express.static(APP_FOLDER));

app.listen(port, () => {
  console.log(`HTTP server listening on port: ${port}\n`);
});

if (modelsPath && fs.existsSync(modelsPath)) {
  app.use('/models', express.static(modelsPath));
  console.log(`Serving models from: ${modelsPath}`);
} else if (modelsPath) {
  console.log(`Models path does not exist: ${modelsPath}`);
}
