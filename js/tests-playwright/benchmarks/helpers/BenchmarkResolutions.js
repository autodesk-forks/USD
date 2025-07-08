

export const ResolutionIdentifiers = Object.freeze({
    R0144p_10_09: '144p',   // for 4-shades rendering and swapping cartridges
    R0200p_16_10: '200p',   // can be ray-cast, should always be fast
    // Full-HD multiples (0.25, 0.5, 1.0, and 2.0) with 16:9 ratio
    R0270p_16_09: '270p',   //    129,600 pixels,  0.1 megapixels, 1/16 HD
    R0540p_16_09: '540p',   //    518,400 pixels,  0.5 megapixels, 1/04 HD
    R1080p_16_09: '1080p',  //  2,073,600 pixels,  2.0 megapixels, Full HD
    R2160p_16_09: '2160p',  //  8,294,400 pixels,  8.3 megapixels, 4K
    // Wide-Quad-HD multiples (0.125, 0.25, 0.5, 1.0, and 2.0) with 16:9 ratio
    R0180p_16_09: '180p',   //     57,600 pixels,  0.1 megapixels, 1/16 HD
    R0360p_16_09: '360p',   //    230,400 pixels,  0.6 megapixels, 1/04 HD
    R0720p_16_09: '720p',   //    921,600 pixels,  0.9 megapixels, HD
    R1440p_16_09: '1440p',  //  3,686,400 pixels,  3.7 megapixels, WQHD (4x HD)
    R2880p_16_09: '2880p',  // 14,745,600 pixels, 14.7 megapixels, 5K
    // Square aspect ratios
    R0001p_01_01: '1p',     //          1 pixel,  1e-6 megapixel
    R0064p_01_01: '64p',    //      4,096 pixels   4.1 kilopixels :)
    R0256p_01_01: '256p',   //     65,536 pixels  65.0 kilopixels
    R1024p_01_01: '1024p',  //  1,048,576 pixels,  1.0 megapixels
    R4096p_01_01: '4096p',  //  4,194,304 pixels,  4.2 megapixels
});

const IDs = ResolutionIdentifiers;
export const ResolutionsByIdentifier = Object.freeze(new Map([
    [IDs.R0144p_10_09, { width: 160, height: 144 }],
    [IDs.R0200p_16_10, { width: 320, height: 200 }],
    // Full-HD multiples (0.25, 0.5, 1.0, and 2.0) with 16:9 ratio
    [IDs.R0270p_16_09, { width: 480, height: 270 }],
    [IDs.R0540p_16_09, { width: 960, height: 540 }],
    [IDs.R1080p_16_09, { width: 1920, height: 1080 }],
    [IDs.R2160p_16_09, { width: 3840, height: 2160 }],
    // Wide-Quad-HD multiples (0.125, 0.25, 0.5, 1.0, and 2.0) with 16:9 ratio
    [IDs.R0180p_16_09, { width: 320, height: 180 }],
    [IDs.R0360p_16_09, { width: 640, height: 360 }],
    [IDs.R0720p_16_09, { width: 1280, height: 720 }],
    [IDs.R1440p_16_09, { width: 2560, height: 1440 }],
    [IDs.R2880p_16_09, { width: 5120, height: 2880 }],
    // Square aspect ratios
    [IDs.R0001p_01_01, { width: 1, height: 1 }],
    [IDs.R0064p_01_01, { width: 2 ** 6, height: 2 ** 6 }],
    [IDs.R0256p_01_01, { width: 2 ** 8, height: 2 ** 8 }],
    [IDs.R1024p_01_01, { width: 2 ** 10, height: 2 ** 10 }],
    [IDs.R4096p_01_01, { width: 2 ** 12, height: 2 ** 12 }],
]));

/**
 * Function to create a subset of the ResolutionsByIdentifier map for testing.
 *
 * @param {Array} keys - Array of keys to include in the filtered map.
 * @returns {Map} A new map containing only the resolution data for the specified keys.
 */
export function filterResolutions(keys) {
    const keysFiltered = [...ResolutionsByIdentifier.keys()].filter((key) => keys === undefined || keys.includes(key));
    return new Map(keysFiltered.map((key) => [key, ResolutionsByIdentifier.get(key)]));
}

/**
 * Convenience function to get the benchmark resolutions based on a named subset. Subset '1080p-ramp' for example, will
 * return a map containing the resolutions 270p, 540p, and 1080p. The subset can be specified as a comma-separated list
 * of resolution identifiers or as a named subset. The named subsets are '1080p-ramp', '1440p-ramp', 'pixel',
 * 'square-ramp', and 'all'. The named subsets can be combined with a comma-separated list of resolution identifiers,
 * e.g., '720p,1440p'. Finally, subset can be set using the environment variable `BENCHMARK_RESOLUTIONS_SUBSET`.
 *
 * @param {ResolutionsSubset} [subset='720p'] - The name of the resolution subset to return.
 * @returns {Map} The map containing the resolution data for the specified subset. @see ResolutionsByIdentifier
 */
export function getResolutionsSubset(subset = '720p') {
    if (process.env.BENCHMARK_RESOLUTIONS_SUBSET) {
        subset = process.env.BENCHMARK_RESOLUTIONS_SUBSET;
    }

    const fhdRamp = filterResolutions([
        IDs.R0270p_16_09, IDs.R0540p_16_09, IDs.R1080p_16_09]);
    const wqhdRamp = filterResolutions([
        IDs.R0180p_16_09, IDs.R0360p_16_09, IDs.R0720p_16_09, IDs.R1440p_16_09]);
    const squareRamp = filterResolutions([
        IDs.R0001p_01_01, IDs.R0064p_01_01, IDs.R0256p_01_01, IDs.R1024p_01_01]);

    switch (subset) {
        case '1080p-ramp':
            return new Map([...fhdRamp]);
        case '2160p-ramp':
            return new Map([...fhdRamp, ...filterResolutions([IDs.R2160p_16_09])]);
        case '1440p-ramp':
            return new Map([...wqhdRamp]);
        case '2880p-ramp':
            return new Map([...wqhdRamp, ...filterResolutions([IDs.R2880p_16_09])]);
        case 'pixel':
            return new Map([...filterResolutions([IDs.R0001p_01_01])]);
        case 'square-ramp':
            return new Map([...squareRamp]);
        case 'square-ramp-4k':
            return new Map([...squareRamp, ...filterResolutions([IDs.R4096p_01_01])]);
        case 'all':
            return new Map(filterResolutions()); // all resolutions
        default:
            return filterResolutions(subset.split(','));
    }
}
