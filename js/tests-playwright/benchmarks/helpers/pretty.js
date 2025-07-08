export const thinsp = ' '; // \u2009'; // thin space
export const emdash = '—'; // \u2014'; // em dash
export const superscriptPlus = '⁺'; // \u207A'; // superscript plus

/**
 * Suffixes used for pretty printing of time values in milliseconds.
 */
const msSuffixes = ['ms', 'ns', 'μs', 'ms', 's'];
/**
 * Scales used for pretty printing of time values in milliseconds.
 */
const msScales = [0, 1e6, 1e3, 1, 1e-3];

/**
 * Prints given milliseconds in an appropriate seconds-based time unit and fixed number of decimal places.
 * ```
 * prettyPrintMilliseconds(0.03277); // returns '32.770μs'
 * prettyPrintMilliseconds(47738102); // returns '07:42:18:900'
 * ```
 * @param {number} milliseconds - Number of milliseconds as floating point number.
 */
export function prettyPrintMilliseconds(milliseconds) {
    if (milliseconds < 120_000) {
        let prefix = milliseconds > 0 ? Math.max(1, Math.floor(Math.log(milliseconds * 10) / Math.log(1e3)) + 3) : 0;
        prefix = Math.max(0, Math.min(4, prefix));

        const value = milliseconds * msScales[prefix];
        return `${value.toFixed(prefix < 4 ? 2 : 3)}${thinsp + msSuffixes[prefix]}`;
    }

    const date = new Date(milliseconds);
    const hh = Math.floor(milliseconds / 3_600_000).toString().padStart(2, 0);
    const mm = date.getMinutes().toString().padStart(2, 0);
    const ss = date.getSeconds().toString().padStart(2, 0);
    const ms = date.getMilliseconds().toString().padStart(3, 0);
    return `${hh > 0 ? hh + ':' : ''}${mm}:${ss}:${ms}`;
}

export function prettyPrintPercentage(ratio, fractionDigits = 2) {
    return isFinite(ratio) ? `${(ratio * 100).toFixed(fractionDigits)}${thinsp}%` : 'n/a';
}

const byteSuffixes = ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi', 'Yi'];
const oneOverLog1024 = 1.0 / Math.log(1024);

/**
 * Converts a number of bytes to a human-readable string with an appropriate byte suffix
 * based on the  International System of Quantities (ISQ, ISO/IEC 80000).
 *
 * @param {number} bytes - The number of bytes to format.
 * @param {number} [prefix] - The optional exponent of the 1024 base to use for the byte suffix. If not
 *      provided, a suitable prefix is chosen automatically based on the size of the value.
 * @returns {string} A formatted string representing the number of bytes with an appropriate byte suffix,
 *      e.g., "1.23MiB".
 */
export function prettyPrintBytes(bytes, prefix = undefined) {
    if (isNaN(bytes)) {
        return 'n/a';
    }
    if (prefix === undefined) {
        prefix = bytes > 0 ? Math.floor(Math.log(bytes) * oneOverLog1024) : 0;
    } else {
        prefix = Math.min(byteSuffixes.length - 1, Math.max(0, prefix));
    }
    const value = bytes / 1024 ** prefix;

    if (prefix === 0) {
        // If it is an integer, don't show decimal places, also use 'Bytes' instead of 'B'
        return `${Number.isInteger(value) ? value : value.toFixed(4)}${thinsp}Bytes`;
    }
    return `${value.toFixed(3)}${thinsp + byteSuffixes[prefix]}B`;
}

/**
 * Intended to create a test step title with the run number, total number of runs, and an optional label.
 * Examples: `Run 01 of 16, Initialization Speed` or `Run 02 of 02 (warmup), Urban House`
 *
 * @param {number} run - The current run number (zero-based).
 * @param {number} runs - The total number of runs.
 * @param {number} numWarmupRuns - The number of warmup runs.
 * @param {string} label - An optional label for the test run.
 * @returns {string} The pretty printed title of the test run.
 */
export function prettyPrintTestRun(run, runs, numWarmupRuns = undefined, label = undefined) {
    let title = '';
    if (numWarmupRuns && run < numWarmupRuns) {
        title += `Warmup ${(run + 1).toString().padStart(2, 0)}/${numWarmupRuns.toString().padStart(2, 0)}`;
    } else {
        title += `Run ${(run + 1 - numWarmupRuns).toString().padStart(2, 0)}/${(runs - numWarmupRuns).toString().padStart(2, 0)}`;
    }
    return label ? `${title}, ${label}` : title;
}
