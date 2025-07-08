// @ts-check
const { defineConfig, devices } = require('@playwright/test');

const path = require('path');

const { config: dotenv } = require('dotenv');

/**
 * Read environment variables from the `.env` file.
 * https://github.com/motdotla/dotenv
 */
dotenv({ override: true });

const TEST_DIRECTORY = path.join(__dirname, 'tests-playwright');
const DEFAULT_PORT = 8000;
const USDVIEWWEB_LOCAL_PORT = Number(process.env.USDVIEWWEB_LOCAL_PORT);
const TEST_PORT = !Number.isNaN(USDVIEWWEB_LOCAL_PORT) && USDVIEWWEB_LOCAL_PORT > 0 ? USDVIEWWEB_LOCAL_PORT : DEFAULT_PORT;

/**
 * @see https://playwright.dev/docs/test-configuration
 */
module.exports = defineConfig({
    testDir: TEST_DIRECTORY,
    snapshotDir: path.join(TEST_DIRECTORY, '.snapshots'),
    /* Configures a template controlling location of snapshots */
    snapshotPathTemplate: '{snapshotDir}/{testFileDir}/{testFileName}/{testName}-{arg}{ext}',
    /* Run tests in files in parallel */
    fullyParallel: true,
    /* Fail the build on CI if you accidentally left test.only in the source code. */
    forbidOnly: !!process.env.CI,
    /* Retry on CI only */
    retries: process.env.CI ? 2 : 0,
    /* Opt out of parallel tests on CI. */
    workers: process.env.CI ? 1 : undefined,
    // Folder for test artifacts such as screenshots, videos, traces, etc.
    outputDir: path.join(TEST_DIRECTORY, '.results'),
    /* Reporter to use. See https://playwright.dev/docs/test-reporters */
    reporter: [
        ['line'],
        ['blob', { outputDir: path.join(TEST_DIRECTORY, '.reports/blob/'), fileName: 'report.zip' }],
        ['html', { outputFolder: path.join(TEST_DIRECTORY, '.reports/html/'), open: 'never' }],
        ['junit', { outputFile: path.join(TEST_DIRECTORY, '.reports/junit/results.xml') }],
    ],
    /* Shared settings for all the projects below. See https://playwright.dev/docs/api/class-testoptions. */
    use: {
        /* Base URL to use in actions like `await page.goto('/')`. */
        baseURL: `http://localhost:${TEST_PORT}`,
        /* Collect trace when retrying the failed test. See https://playwright.dev/docs/trace-viewer */
        trace: 'on-first-retry'
    },
    testIgnore: [`${TEST_DIRECTORY}/_playwright/**`],
    expect: {
        toHaveScreenshot: {
            maxDiffPixelRatio: 0.02,
        },
    },

    /* Configure projects for major browsers */
    projects: [
        {
            name: 'benchmarks',
            testMatch: /.*\.benchmark\.js/,
            use: {
                ...devices['Desktop Chrome'],
                channel: 'chromium',
                // channel: 'chrome', // use branded Chrome for benchmarks
                launchOptions: {
                    // See https://peter.sh/experiments/chromium-command-line-switches/
                    args: [
                        '--enable-gpu',
                        // '--disable-software-rasterizer',
                        // '--use-gl=egl',
                        '--enable-unsafe-webgpu',

                        // Rendering Performance Tweaks

                        '--disable-frame-rate-limit',
                        '--disable-gpu-vsync',
                        '--max-gum-fps="9999"',
                        '--force_high_performance_gpu',
                        '--ignore-gpu-blocklist',
                        '--js-flags=--expose-gc',

                        // Disables throttling of timers in background pages to ensure consistent timer execution intervals.
                        '--disable-background-timer-throttling',
                        // Prevents Chromium from treating occluded (hidden) windows as background pages to maintain consistent performance.
                        '--disable-backgrounding-occluded-windows',
                        // Disables lowering the priority of renderer processes that are in the background to ensure consistent rendering performance.
                        '--disable-renderer-backgrounding',

                        // '--auto-open-devtools-for-tabs', // uncomment to make debugger statements work right after start
                    ],
                },
            },
        }
    ],

    /* Run your local dev server before starting the tests */
    webServer: [
        {
            command: `npm run start-dev-server`,
            url: `http://localhost:${TEST_PORT}/usdviewweb.html`,
            reuseExistingServer: true,
        }
    ],
});
