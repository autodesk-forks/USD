Module.preRun.push(function() {
    ENV.PIXAR_TF_ENV_SETTING_FILE = "/webgpu-env-settings";
    ENV.PXR_MTLX_STDLIB_SEARCH_PATHS = "/libraries";
    ENV.PXR_MTLX_PLUGIN_SEARCH_PATHS = "/libraries";
})

