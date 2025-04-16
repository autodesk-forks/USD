/* eslint-disable playwright/no-skipped-test */
/* eslint-disable playwright/expect-expect */
/* eslint-disable playwright/no-conditional-in-test */

import { expect, test } from '@playwright/test';

import {
    BenchmarkLog,
    getResolutionsSubset,
    prettyPrintTestRun,
    getFilteredModels
} from './helpers';

const NUM_SAMPLES = 100;
const NUM_WARMUP_RUNS = 0;
const NUM_RUNS = 4;
/**
 * Speed benchmarks evaluate the viewer's ability to display models with highly interactive frame rates, while
 * considering different rendering modes, such as progressive and full frame rendering. The rendering speed depends on
 * resolution, backend, optimizations and their configurations, post-processing setup, and complexity of the scene.
 */
test.describe('WASM Rendering Speed', () => {
    test.describe.configure({ mode: 'serial' }); // enforce serial execution

    /** @type {BenchmarkLog} */
    let benchmarkLog = undefined;

    const resolutions = getResolutionsSubset('720p'); // other: '720p,1440p', '1440p-ramp', or based on process.env.BENCHMARK_RESOLUTIONS_SUBSET
    test.beforeEach(async ({ page, browser }, testInfo) => {
        benchmarkLog = new BenchmarkLog(testInfo, browser);
    });

    test.afterEach(async ({ page }, testInfo) => {
        await benchmarkLog.finalize(true, true);
        benchmarkLog = undefined;
    });

    const test_frameRendering = async (page, progressive, immediate, hooks) => {
        const { preInitializationCallback, postInitializationCallback, postLoadCallback, preRunCallback, postRunCallback } = hooks;

        const [numWarmupRuns, numRuns] = [NUM_WARMUP_RUNS, NUM_RUNS];
        const runs = numWarmupRuns + numRuns;

        // Note: Just loading and rendering during that ramps up the GPU and CPU, so warm-up runs might be unnecessary.
        // lets at least have two runs to have some meaningful statistics
        expect(numRuns).toBeGreaterThanOrEqual(2);

        /** @todo: this is rather arbitrary, should be aligned with number of samples */
        // Accumulate the total timeout for all models and runs.
        test.setTimeout(3_600 * 2 * 1e+3); // two hours

        await test.step(`Initialize viewing`, async () => {
            await preInitializationCallback?.(page);
            await postInitializationCallback?.(page);
        });

        /** @todo: could also be done as pre-run callback configuration */
        await test.step(`${progressive ? 'Enable' : 'Disable'} progressive rendering (pre-run)`, async () => {
            await page.evaluate(({ progressive, immediate = true }) => {
                window.progressive = progressive;
                window.immediate = progressive && immediate;
            }, { progressive, immediate });
        });

        const models = getFilteredModels();
        for (const [identifier, filename] of models) {

            await page.goto('/usdviewweb.html?perf');
            await page.waitForFunction(() => {
                return _ems_main !== undefined && document.getElementById("webgpuCanvas");
            });
            expect(await page.evaluate(() => _ems_main !== undefined)).toBeTruthy();

            await postLoadCallback?.(page);

            const numSamples = NUM_SAMPLES;
            await page.evaluate((numSamples) => {
                window.measurements = new Array();

                window.addEventListener('onplay', (event) => {
                    window.measurements_running = true;
                    window.measurements.length = numSamples;
                    window.measurements.fill(0);
                    window.measurements_average = NaN;
                });
                window.addEventListener('onframechanged', (event) => {
                    if (!window.measurements_running) return;
                    const index = event.detail.frameIndex;
                    window.measurements[index] = performance.now(); // start time
                });
                window.addEventListener('onframepresented', (event) => {
                    if (!window.measurements_running) return;
                    const index = event.detail.frameIndex;
                    window.measurements[index] = performance.now() - window.measurements[index]; // end time - start time

                    if (!event.detail.lastFrame) return;

                    window.measurements_average = window.measurements.reduce((sum, value) => sum + value, 0) / window.measurements.length;
                    window.measurements_first_frame = window.measurements.shift();
                    window.measurements_frame_average_exclude_first_frame = window.measurements.reduce((sum, value) => sum + value, 0) / window.measurements.length;
                    
                    window.measurements_running = false;
                });
            }, numSamples);

            const iterations = [...resolutions].map(([resolution, { width, height }]) =>
                [resolution, width, height]);
            for (const [resolution, width, height] of iterations) {

                await page.evaluate(({width, height}) => {
                    document.getElementById("webgpuCanvas").height = height;
                    document.getElementById("webgpuCanvas").width = width;
                }, {width, height});

                const finalFrameTimeAverages = new Array();
                const firstFrameTime = new Array();
                const finalFrameExcludeFirstTimeAverages = new Array();

                for (let run = 0; run < runs; ++run) {
                    const stepTitle = prettyPrintTestRun(
                        run, runs, numWarmupRuns, [identifier, resolution].join(', '));
                    await test.step(stepTitle, async () => {
                        await preRunCallback?.(page);

                        await page.evaluate(async ({numSamples, filename}) => {
                            try {
                                let usdFilename = await loadBinaryFile(filename);
                                var lengthBytes = lengthBytesUTF8(filename) + 1;
                                var fileNameOnWasmHeap = _malloc(lengthBytes);
                                stringToUTF8(usdFilename, fileNameOnWasmHeap, lengthBytes);
                                _ems_main(
                                    document.getElementById("webgpuCanvas").width,
                                    document.getElementById("webgpuCanvas").height,
                                    numSamples,
                                    fileNameOnWasmHeap,
                                    true
                                );
                                _free(fileNameOnWasmHeap);
                            } catch(err) {
                                console.log(err);
                            }
                        }, {numSamples, filename});

                        await page.waitForFunction(() => !window.measurements_running && isFinite(window.measurements_average));
                        const finalFrameTimes_average = await page.evaluate(() => window.measurements_average);
                        const firstFrame = await page.evaluate(() => window.measurements_first_frame);
                        const finalFrameTimesExcludeFirst_average = await page.evaluate(() => window.measurements_frame_average_exclude_first_frame);
                        finalFrameTimeAverages.push(finalFrameTimes_average);
                        firstFrameTime.push(firstFrame);
                        finalFrameExcludeFirstTimeAverages.push(finalFrameTimesExcludeFirst_average);

                        await postRunCallback?.(page);
                    });
                }

                benchmarkLog.pushMeasurement({
                    name: 'final-response-time', type: 'time', unit: 'ms',
                    description: 'average time to render a final frame',
                    data: finalFrameTimeAverages, numRuns, numWarmupRuns, tags: [identifier, resolution]
                });

                benchmarkLog.pushMeasurement({
                    name: 'first-frame', type: 'time', unit: 'ms',
                    description: 'time to render the first frame',
                    data: firstFrameTime, numRuns, numWarmupRuns, tags: [identifier, resolution]
                });

                benchmarkLog.pushMeasurement({
                    name: 'final-response-time-exclude-first', type: 'time', unit: 'ms',
                    description: 'average time to render a final frame excluding first frame',
                    data: finalFrameExcludeFirstTimeAverages, numRuns, numWarmupRuns, tags: [identifier, resolution]
                });

            }
        }
    };

    test('Full Frame Rendering (WASM)', async ({ page }) => {
        benchmarkLog.describe(
            `This benchmark measures the time it takes to render a full frame of a model. The measurements are averaged
            over multiple consecutive runs. It also displays how long the first frame took and the average excluding it.`);

        await test_frameRendering(page, false, undefined, {
            preInitializationCallback: undefined,
            postLoadCallback: undefined
        });
    });

});