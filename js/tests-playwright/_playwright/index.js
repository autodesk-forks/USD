export {
    MODELS_URL,
} from './constants';


// If an environment variable is set, it should be a boolean string, e.g., 'true' or 'false'. In case is intended
// as number, e.g., '0' or '1', it should be parsed as such. Only 'true' and finite numbers > 0 are considered as
// truthy, everything else is considered as falsy. If the environment variable is not set, the default value is
// undefined.
export function isTruthy(value) {
    if (value === undefined) {
        return undefined;
    }
    if (isFinite(parseInt(value, 10))) {
        return parseInt(value, 10) > 0;
    }
    return value.toLocaleLowerCase() === 'true' || value === '1';
}

export function isFalsy(value) {
    return !isTruthy(value);
}
