Module.preRun.push(function() {
    ENV.PIXAR_TF_ENV_SETTING_FILE = "/webgpu-env-settings";
    FS.mkdir("/usdviewweb");
    FS.mount(IDBFS, {autoPersist: true}, "/usdviewweb");
    return FS.syncfs(true, function (err) {
        if (err) {
            console.error("Error syncing filesystem:", err);
        } else {
            console.log("Filesystem synced.");
        }
    });
})

