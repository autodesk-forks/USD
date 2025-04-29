import Mixpanel from 'mixpanel';

import { isTruthy } from '../../_playwright';
import { BenchmarkLog } from './BenchmarkLog';
import { config } from 'dotenv';

config({ override: true });

const MIXPANEL_TOKEN = process.env.MIXPANEL_TOKEN || 'a653c59f032f2f7a8854156013f0b4f9';
const MIXPANEL_EVENT = 'hdSt.benchmark'; // event name for Mixpanel tracking

/**
 * Mixpanel configuration defaults look quite good, no need for adjusting them (based on
 * https://github.com/mixpanel/mixpanel-js/blob/master/doc/readme.io/javascript-full-api-reference.md#mixpanelset_config
 */
const MIXPANEL_CONFIG = { geolocate: false, batch_requests: false, autotrack: false };

/**
 * This can be set to, e.g., 'user', 'nightly, or 'ci' to distinguish between manual and automated runs. It can be
 * overridden by the environment variable BENCHMARK_TRACKING_ORIGIN. For debugging, 'debug' should be used.
 * @type {string}
 * @private
 */
let origin = undefined;

/**
 * Mixpanel instance for tracking benchmark data. It is initialized lazily when the first tracking request is made.
 * @type {Mixpanel}
 * @private
 */
let mixpanel = undefined;
let options = { track: false, debug: true };

function initialize() {
    if (mixpanel) return;

    mixpanel = Mixpanel.init(MIXPANEL_TOKEN, { ...MIXPANEL_CONFIG, verbose: options.debug, debug: options.debug });

    options.track = isTruthy(process.env.BENCHMARK_TRACKING);
    origin = process.env.BENCHMARK_TRACKING_ORIGIN ?? 'user'; // intended to be used in automated runs, e.g., 'nightly' or 'ci'

    console.log('Benchmark Tracking (Mixpanel):', options.track ? `enabled (origin = '${origin}')` : 'disabled');
}

function flatten(object, exclude = undefined, parentKey = '', result = {}) {
    for (let key in object) {
        if (!Object.prototype.hasOwnProperty.call(object, key)) {
            continue;
        }
        if (exclude instanceof Array && exclude.includes(key)) {
            continue;
        }
        let propertyKey = parentKey ? `${parentKey}_${key}` : key;
        if (typeof object[key] === 'object' && !Array.isArray(object[key])) {
            flatten(object[key], exclude, propertyKey, result);
        } else {
            result[propertyKey] = object[key];
        }
    }
    return result;
}

/**
 * Flattens the given `benchmarkLog` object and sends the data to Mixpanel. The data is sent as a single event per
 * measurement to simplify the analysis of the data. The event name is `viewer.benchmark` (@see MIXPANEL_EVENT).
 * For simplicity, all time measurement summaries that are in milliseconds are converted to seconds before sending, to
 * avoid conversion overhead in the Mixpanel dashboard. The benchmark log's measurements remain unchanged.
 *
 * @param {BenchmarkLog} log
 * @returns {Promise<void>} Resolves when all Mixpanel tracking requests have been sent.
 */
export async function track(log) {
    if (!(log instanceof BenchmarkLog)) return;
    initialize();

    if (log.measurements.length === 0) return;

    if (!options.track) return;

    const benchmark_payload = new Object({
        timestamp: log.timestamp,
        origin: origin,
        ...flatten(log.test, ['startTime'], 'test'),
        ...flatten(log.browser, undefined, 'browser'),
        ...flatten(log.viewer, undefined, 'viewer'),
        ...flatten(log.device, undefined, 'device'),
    });

    const promises = new Array();

    for (const measurement of log.measurements) {
        const payload = new Object();
        Object.assign(payload, flatten(measurement, ['data', 'description', 'numRuns', 'numWarmupRuns'], 'measurement'));
        Object.assign(payload, benchmark_payload);

        switch (measurement.type) {
            case 'time':
                millisecondsSummaryToSeconds(payload);
                promises.push(new Promise((resolve) => mixpanel.track(MIXPANEL_EVENT, payload, () => resolve())));
                break;
            default:
                break;
        }
    }
    return Promise.all(promises);
}

const MILLISECONDS_TO_SECONDS = 1e-3;

function millisecondsSummaryToSeconds(payload) {
    if (payload.measurement_unit !== 'ms' || payload.measurement_type !== 'time') return;

    // rounding to a fixed number of digits is not necessary, as Mixpanel aggregates the data anyway

    payload.measurement_summary_average  *= MILLISECONDS_TO_SECONDS;
    payload.measurement_summary_minimum  *= MILLISECONDS_TO_SECONDS;
    payload.measurement_summary_maximum  *= MILLISECONDS_TO_SECONDS;

    payload.measurement_summary_sigma    *= MILLISECONDS_TO_SECONDS; // same unit as average (or input data)

    // variance is in ms² (milliseconds squared); convert to s² (seconds squared)
    payload.measurement_summary_variance *= MILLISECONDS_TO_SECONDS ** 2;

    payload.measurement_unit = 's';
}
