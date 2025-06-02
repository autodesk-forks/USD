/* eslint-disable no-irregular-whitespace */

import {
    thinsp, emdash, superscriptPlus,
    prettyPrintBytes,
    prettyPrintMilliseconds,
    prettyPrintPercentage
} from './pretty.js';

import { writeFileSync, rmSync } from 'fs';
import * as os from 'os';
import { execSync } from 'child_process';

/** @import { Browser, Page, TestInfo } from '@playwright/test' */

/**
 * Generates a shortened local timestamp in the format 'YYYYMMDD-HHMMSSmmm'.
 * @param {Date} date - The date object to generate the timestamp from.
 * @returns {string} - The shortened local timestamp.
 */
export function getShortenedLocalTimestamp(date) {
    const timezoneOffset = date.getTimezoneOffset() * 60_000;
    const localDate = new Date(date.getTime() - timezoneOffset);
    // The ISO string looks like this: '2021-09-22T19:56:40.000Z'. The first replace removes the separators and the time
    // zone information to get a string like this: '20210922195640000'. The second replace captures the first 8 digits
    // (the date) and the last 9 digits (the time) and separates them with an underscore.
    return localDate
        .toISOString()
        .replace(/[-:T.Z]/g, '')
        .replace(/^(\d{8})(\d{9})$/, '$1-$2');
}

/**
 * Class for creating a benchmark log for tests. The log contains detailed information about the test, browser, and
 * measurements taken during the test. It is designed to facilitate reproducibility and provide comprehensive context
 * for interpreting benchmark results. The class provides methods to describe the test, add measurements, and
 * finalize the log with annotations and attachments.
 *
 * @example
 * test.describe('Benchmarks with Log', () => {
 *     test.describe.configure({ mode: 'serial' }); // enforce serial execution
 *     let benchmarkLog = undefined;
 *
 *     test.beforeEach(async ({ page, browser }, testInfo) => {
 *         const benchmarkLog = new BenchmarkLog(testInfo, browser);
 *     });
 *
 *     test.afterEach(async ({ page, browser }, testInfo) => {
 *         await benchmarkLog.finalize();
 *     });
 *
 *     test('Specific Benchmark', async ({ page }) => {
 *         benchmarkLog.describe('This benchmark measures the time to...');
 *         ...
 *         // Perform some actions and measure time, and create an array of measured values:
 *         const times = [100, 200, 150]; // for example, times in ms
 *         // Provide a description, unit, data, number of runs, number of warmup runs, and optional tags
 *         benchmarkLog.pushMeasurement({
 *             time: 'time', description: 'duration after which the event occurred', unit: 'ms',
 *             data: times, numRuns: 3, numWarmupRuns: 1, tags: [ 'EVENT_NAME', 'MODEL_ID' ]});
 *      });
 * });
 *
 */
export class BenchmarkLog {
    /** @type {string} - The timestamp of the benchmark log. */
    timestamp;
    /** @type {string} - The local timestamp of the benchmark log. */
    timestamp_local;
    /** @type {string} - The description of the benchmark log. */
    description;
    /** @type {Object} - Subset of test information for context of the measurements. */
    test;
    /** @type {Object} - Subset of browser information for context of the measurements. */
    browser;
    /** @type {Object} - Device information for context of the measurements. */
    device;

    measurements = new Array();

    /** @type {TestInfo} */
    #testInfo;

    /**
     * Creates a benchmark log object for a test. The log contains information about the test and the browser, as well
     * as a timestamp and an empty description. The log also contains an empty array for measurements.
     *
     * @param {TestInfo} testInfo - Used to access test information as provided by the test framework.
     * @param {Browser} browser - The browser instance being used for the test.
     * @param {Page} page - The page instance being used for the test.
     * @returns {BenchmarkLog} The created benchmark log object.
     *
     * @throws {Error} - If the benchmark log is not initialized, i.e., not attached to testInfo.
     */
    constructor(testInfo, browser, page) {
        this.#testInfo = testInfo;

        const date = new Date();
        this.timestamp_local = getShortenedLocalTimestamp(date); // example: '19850225-215640000'
        this.timestamp = date.toISOString(); // example: '1985-02-25T21:56:40.000Z'

        this.description = undefined;
        this.test = {
            id: this.#testInfo.testId,
            title: this.#testInfo.titlePath.slice(1).join(' > '), // example: 'Initialization Speed > Viewer Initialization'
            file: this.#testInfo.titlePath[0], // example: `initialization-speed.benchmark.js`
        };
        this.browser = {
            name: browser.browserType().name(), // example: 'chromium'
            version: browser.version(), // example: '94.0.4606.71'
        };
        this.device = getDeviceInfo();
        this.evaluateViewerVersion(page);
    }

    /**
     * Adds a description to the benchmark log. The description is a string that provides additional context or details
     * about the test being performed.
     * @param {string} description - The description to add to the benchmark log.
     */
    describe(description) {
        this.description = description;
    }

    static MandatoryKeys = Object.freeze([
        'name',
        'type',
        'description',
        'unit',
        'data',
        'numRuns'
    ]);

    /**
     * Adds a measurement to the benchmark log. The measurement is an object that contains the type, description, unit,
     * data, number of runs, number of warmup runs, and optional tags. The data is an array of numeric values that
     * represent the measurements taken during the test. The number of runs is the total number of runs, including warmup
     * runs, while the number of warmup runs is the number of initial runs that are not included in the summary.
     *
     * @param {Object} measurement - The measurement object containing the measurement details.
     * @param {string} measurement.name - The name of measurement, e.g., 'immediate-response-time'.
     * @param {string} measurement.type - The type of measurement, e.g., 'time', 'memory', 'scene'.
     * @param {string} measurement.description - The description of the measurement, e.g., 'time to initialize the viewer'.
     * @param {string} measurement.unit - The unit of the measurement, e.g., 'ms', 'MB', 'polygons'.
     * @param {Array<number>} measurement.data - The array of numeric values representing the measurements.
     * @param {number} measurement.numRuns - The total number of runs, including warmup runs.
     * @param {number} [measurement.numWarmupRuns = 0] - The number of initial runs that are not included in the summary.
     * @param {Array<string>} [measurement.tags = []] - An optional array of tags to identify the measurement.
     * @param {string} [tags = undefined] - Optional tags to add to the measurement.
     * @returns {number} The index of the added measurement in the measurements array.
     */
    pushMeasurement(measurement, tags = new Array()) {
        const missingKeys = BenchmarkLog.MandatoryKeys.filter((key) => !(key in measurement));
        if (missingKeys.length > 0) {
            console.error('Missing mandatory key(s) in measurement:', missingKeys.join(', '), '; given', measurement);
            return;
        }
        !(measurement.tags instanceof Array) && (measurement.tags = new Array());
        tags instanceof Array && measurement.tags.push(...tags);

        this.measurements.push(measurement);

        BenchmarkLog.summarize(this.measurements.at(-1));
        return this.measurements.length - 1;
    }

    /**
     * Generates a summary of a numeric measurement, including average, minimum, maximum, variance, and standard
     * deviation. The summary is computed for the non-warmup runs of the measurement data, added to the measurement
     * object as a property, and returned. If a summary already exists, it returns the existing summary. Non-finite
     * values are excluded from the summary.
     *
     * @param {Object} measurement - The measurement object containing data and number of (warmup) runs.
     * @returns {Object} The summary of the measurement.
     *
     * @throws {Error} - If the number of values is less than the number of runs.
     */
    static summarize(measurement) {
        if (measurement.summary) {
            return measurement.summary;
        }
        const values = measurement.data;
        if (values.length < measurement.numRuns) {
            throw new Error(`Expected number of values ${values.length} to be at least ${measurement.numRuns}`);
        }
        values.splice(0, measurement.numWarmupRuns);

        const zerosExcluded = ['download-rate', 'download-size'].includes(measurement.type) ? true : undefined;
        const values_finites = values.filter((value) => isFinite(value) && (!zerosExcluded || value > 0));

        const empty = values_finites.length === 0;

        const average = empty ? undefined : values_finites.reduce((a, b) => a + b, 0) / values_finites.length;
        const minimum = empty ? undefined : Math.min(...values_finites);
        const maximum = empty ? undefined : Math.max(...values_finites);
        const variance = empty ? undefined : values_finites.reduce((a, b) => a + (b - average) ** 2, 0) / values_finites.length;
        const sigma = empty ? undefined : Math.sqrt(variance); // standard deviation

        measurement.summary = { average, minimum, maximum, variance, sigma };
        zerosExcluded && (measurement.summary.zerosExcluded = zerosExcluded);

        return measurement.summary;
    }

    measurement(index) {
        if (index < 0 || index >= this.measurements.length) {
            throw new Error(`Measurement index out of bounds, given ${index}`);
        }
        return this.measurements[index];
    }

    summary(index) {
        if (index < 0 || index >= this.measurements.length) {
            throw new Error(`Measurement index out of bounds, given ${index}`);
        }
        const measurement = this.measurements[index];
        return BenchmarkLog.summarize(measurement); // does not summarize if already done, returns the summary
    }

    /**
     * Evaluates and adds the viewer version information to the benchmark log. The viewer information
     * includes the current git commit, the timestamp of the git commit and whether it is a wasm64 build
     *
     * @param {Page} page - The page instance being used for the test.
     * @returns {Promise<void>} A promise that resolves when the viewer version information is added to the benchmark log.
     *
     * @throws {Error} - If the benchmark log is not initialized, i.e., not attached to testInfo.
     */
    async evaluateViewerVersion(page) {
        if (this.viewer !== undefined) {
            return;
        }

        await page.waitForFunction(() => {
            return _isWasm64 !== undefined;
        });
        const commitInfo = execSync('git rev-parse --short HEAD', { cwd: process.cwd() }).toString().trim();
        const commitTimestamp = execSync('git log -1 --format=%cd --date=iso-strict', { cwd: process.cwd() }).toString().trim();
        const isWasm64 = await page.evaluate(() => window._isWasm64());

        this.viewer = {
            version: commitInfo,
            build: isWasm64 ? 'wasm64' : 'wasm',
            commitTimestamp
        };
    }

    /**
     * Finalizes the log by adding test start time, duration, and status. After finalization, the log is frozen to
     * prevent further modifications.
     *
     * @param {boolean} [annotate=true] - Whether to annotate the test results with details of this log.
     * @param {boolean} [attach=true] - Whether to attach this log as a JSON file to the test results.
     * @returns {Promise<void>} A promise that resolves when the benchmark log is finalized.
     *
     * @throws {Error} - If the benchmark log is not initialized, i.e., not attached to testInfo.
     */
    async finalize(annotate = true, attach = true) {
        Object.assign(this.test, {
            duration: this.#testInfo.duration,
            status: this.#testInfo.status,
        });
        Object.freeze(this);

        annotate && (await this.annotate());
        attach && (await this.attach());
    }

    /**
     * This attaches the benchmark log in two ways: as a JSON object and as a JSON file using test.info().attach.
     *
     * @returns {Promise<void>} A promise that resolves when the benchmark log is saved and attached.
     */
    async attach() {
        const logJSON = JSON.stringify(
            {
                timestamp_local: this.timestamp_local,
                timestamp: this.timestamp,
                description: this.description,
                test: this.test,
                browser: this.browser,
                viewer: this.viewer,
                device: this.device,
                measurements: this.measurements,
            }, undefined, 2);

        await this.#testInfo.attach('Benchmark Log (JSON)', { body: logJSON, contentType: 'application/json' });

        const logFileName = `viewer-initialization--${this.test.id}--${this.timestamp_local}.json`;
        const logFilePath = this.#testInfo.outputPath(logFileName);

        writeFileSync(logFilePath, logJSON, { encoding: 'utf-8' });
        await this.#testInfo.attach(`Benchmark Log (JSON File)`, {
            path: logFilePath,
            contentType: 'application/json',
        });
        rmSync(logFilePath, { force: true });
    }

    /**
     * This annotates the test results with details of this benchmark log. This includes Viewer information, test
     * description, and measurement summaries, for which specialized annotations are used based on the measurement
     * types.
     *
     * @returns {Promise<void>} A promise that resolves when the annotations are added.
     */
    async annotate() {
        const annotations = this.#testInfo.annotations;

        // Annotate Description
        this.description && annotations.push({ type: 'Description', description: this.description });
        this.description && annotations.push({ type: ' ' }); // add empty annotation line

        // Annotate Measurement Summaries (if available)
        if (this.measurements.length < 1) {
            return;
        }
        // Prefer a short summary if there are multiple measurements
        const total = this.measurements.length;
        const short = total > 1;
        short && annotations.push({ type: 'Summary of Measurements' });

        for (const [index, measurement] of this.measurements.entries()) {
            switch (measurement.type) {
                case 'time':
                    annotateMeasurement(annotations, measurement, index, total, short, prettyPrintMilliseconds);
                    break;
                case 'download-size':
                    annotateMeasurement(annotations, measurement, index, total, short, prettyPrintBytes);
                    break;
                case 'scene':
                    annotateSceneMeasurement(annotations, measurement, index, total, short);
                    break;
                case 'ratio':
                    annotateMeasurement(annotations, measurement, index, total, short, prettyPrintPercentage);
                    break;
                default:
                    annotateMeasurement(annotations, measurement, index, total, short);
                    break;
            }
        }
    }
}

function measurementAnnotationPrefix(index, total, name, type) {
    const fixedWidthIndex = index.toString().padStart(Math.log10(total) + 1, 0);
    // The index should correspond to the measurement index within the measurements array, starting from 0.
    return `#${fixedWidthIndex} ${name} (${type})`;
}

function measurementAnnotationLabel(tags) {
    return tags instanceof Array && tags.length > 0 ? ' ' + emdash + ' ' + tags.join(', ') : '';
}

/**
 * Annotates the test results with a summary of a scene measurement. The summary includes the number of polygons,
 * geometries, fragments, materials, and textures. The summary is added to the annotations array either as a single line
 * or as a detailed description with multiple lines. The `short` parameter determines the output format.
 *
 * @param {Array<Object>} annotations - The array to store annotations, i.e., test.info().annotations.
 * @param {Object} measurement - The measurement object containing the summary
 * @param {number} index - The index of the measurement in the list.
 * @param {number} total - The total number of measurements. Used for prettifying the index.
 * @param {boolean} short - Whether to generate a short summary.
 */
function annotateSceneMeasurement(annotations, measurement, index, total, short) {
    if (measurement.type !== 'scene') return;

    const { polygons, polygons_instanced, geometries, fragments, materials, textures, models } = measurement.summary;

    const prefix = measurementAnnotationPrefix(index, total, measurement.name, measurement.type);

    const prettyDecimal = (number) => number.toLocaleString('en-US', { maximumFractionDigits: 0 });
    const postfix = measurementAnnotationLabel(measurement.tags);
    if (short) {
        annotations.push({
            type: prefix, description:
                `${prettyDecimal(fragments)} fragments, ${prettyDecimal(geometries)} geometries, `
                + `${prettyDecimal(polygons)} polygons (${prettyDecimal(polygons_instanced)} instanced), `
                + `${prettyDecimal(materials)} materials, ${prettyDecimal(textures)} textures, `
                + `${prettyDecimal(models)} model${models > 1 ? 's' : ''}`
                + postfix
        });
        return;
    }
    annotations.push({ type: `Summary of Scene Measurement ${prefix}${postfix}` },
        { type: `${thinsp}- Models`, description: prettyDecimal(models) },
        { type: `${thinsp}- Description`, description: measurement.description },
        { type: `${thinsp}- Fragments`, description: prettyDecimal(fragments) },
        { type: `${thinsp}- Geometries`, description: prettyDecimal(geometries) },
        { type: `${thinsp}- Polygons`, description: `${prettyDecimal(polygons)} (${prettyDecimal(polygons_instanced)} instanced)` },
        { type: `${thinsp}- Materials`, description: prettyDecimal(materials) },
        { type: `${thinsp}- Textures`, description: prettyDecimal(textures) }
    );
}

/**
 * Annotates the test results with a summary of a numeric measurement. The summary includes the average, minimum, maximum,
 * variance, and standard deviation of the measurement values. The summary is added to the annotations array either as a
 * single line or as a detailed description with multiple lines. The `short` parameter determines the output format.
 *
 * @param {Array<Object>} annotations - The array to store annotations, i.e., test.info().annotations.
 * @param {Object} measurement - The measurement object containing the summary.
 * @param {number} index - The index of the measurement in the list.
 * @param {number} total - The total number of measurements. Used for prettifying the index.
 * @param {boolean} short - Whether to generate a short summary.
 * @param {Function} [prettyPrint] - An optional function to format the measurement values.
 */
function annotateMeasurement(annotations, measurement, index, total, short, prettyPrint = undefined) {
    const { average, minimum, maximum, variance, sigma } = measurement.summary;

    const prefix = measurementAnnotationPrefix(index, total, measurement.name, measurement.type);
    const zerosExcluded = measurement.summary.zerosExcluded ? superscriptPlus : '';

    const unit = thinsp + (measurement.unit ?? '');
    const prettyFun = prettyPrint || ((value) => !isFinite(value) ? 'n/a' : `${value.toFixed(2)}${unit}`);

    const postfix = measurementAnnotationLabel(measurement.tags);
    if (short) {
        annotations.push({
            type: prefix, description:
                `${prettyFun(average)} ± ${prettyFun(sigma)} in [${prettyFun(minimum)}, ${prettyFun(maximum)}]${zerosExcluded}${postfix}`
        });
        return;
    }
    annotations.push(
        { type: `Summary of Measurement${zerosExcluded} ${prefix}${postfix}` },
        { type: `${thinsp}- Type`, description: measurement.type },
        { type: `${thinsp}- Description`, description: measurement.description },
        { type: `${thinsp}- Average`, description: prettyFun(average) },
        { type: `${thinsp}- Minimum`, description: prettyFun(minimum) },
        { type: `${thinsp}- Maximum`, description: prettyFun(maximum) },
        { type: `${thinsp}- Variance`, description: `${variance.toFixed(2)}${unit}²` },
        { type: `${thinsp}- Standard Deviation`, description: `${sigma.toFixed(2)}${unit}` },
    );
}

function getDeviceInfo() {
    const cpus = os.cpus();
    const platform = os.platform().replace('win32', 'win').replace('darwin', 'mac');

    return {
        name: os.hostname(),
        user: os.userInfo().username,
        platform: platform,
        cpu: cpus.length ? cpus[0].model : '-',
    };
}
