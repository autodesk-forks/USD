if (Module?.preRun) {
    Module.preRun.push(function() {
        ENV.PIXAR_TF_ENV_SETTING_FILE = "/webgpu-env-settings";
        ENV.PXR_AR_DEFAULT_SEARCH_PATH = "/";
        ENV.PXR_MTLX_STDLIB_SEARCH_PATHS = "/libraries";
        ENV.PXR_MTLX_PLUGIN_SEARCH_PATHS = "/libraries";
        // Enable this to see all errors in the browser console
        // ENV.TF_DEBUG = "TF_PRINT_ALL_POSTED_ERRORS_TO_STDERR"; 
        FS.mkdir("/usdviewweb");
        FS.mount(IDBFS, {autoPersist: true}, "/usdviewweb");
        return FS.syncfs(true, function (err) {
            if (err) {
                console.error("Error syncing filesystem:", err);
            } else {
                console.log("Filesystem synced.");
            }
        });
    });
}

