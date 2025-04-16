import { MODELS_URL } from "../../_playwright";
import { config } from "dotenv";
import fs from 'fs';
import path from 'path';

config({ override: true });

/**
 * Function to create a subset of the ModelDataByIdentifier map for testing.
 *
 * @param {Map} map - The map containing the model data, e.g., ModelsByIdentifier.
 * @param {Array} keys - Array of keys to include in the filtered map.
 * @returns {Map} A new map containing only the model data for the specified keys.
 */
export function filterModels(map, keys) {
    const keysFiltered = [...map.keys()].filter((key) => keys === undefined || keys.includes(key));
    return new Map(keysFiltered.map((key) => [key, map.get(key)]));
}

/**
 * Convenience function to get the benchmark models based on a named subset. The subset can be 'small', 'medium',
 * 'large', 'small+medium', or 'all'. If no subset is specified, the environment variable `LMV_BENCHMARK_MODELS_SUBSET`
 * is used. If the environment variable is not set, the default subset is 'small'. The named subsets can be combined
 * with a comma-separated list of model identifiers.
 *
 * @param {ModelsSubset} [subset] - The name of the model subset to return.
 * @param {@ModelSource} [source] - The source of the models, i.e., 'colosseum' or 'acc-qa'.
 * @returns {Map} The map containing the model data for the specified subset. @see ModelsByIdentifier
 */
export function getServedModels() {
    const modelsPath = process.env.BENCHMARK_MODELS_FOLDER;
    if (!modelsPath || !fs.existsSync(modelsPath)) {
        console.warn(
            'No benchmark models folder found. Please set the environment variable BENCHMARK_MODELS_FOLDER to the path of the models.'
        );
        return [];
    }

    const models = new Map();
    const validExtensions = ['.usd', '.usdz', '.usda'];
    const files = fs.readdirSync(modelsPath);

    files.forEach(file => {
        const ext = path.extname(file);
        if (validExtensions.includes(ext)) {
            const nameWithoutExt = path.basename(file, ext);
            models.set(nameWithoutExt, MODELS_URL + file);
        }
    });
    return models;
}

export function getFilteredModels() {
    const allModels = getServedModels();
    const filters = process.env.FILTERED_MODELS ? process.env.FILTERED_MODELS.split(',') : [];

    if (filters && filters.length > 0) {
        return filterModels(allModels, filters);
    } else {
        return allModels;
    }
}
