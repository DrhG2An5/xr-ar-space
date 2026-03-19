#include "app/App.h"
#include "app/Config.h"
#include "util/Log.h"
#include "util/ConfigFile.h"

int main() {
    xr::Log::setLevel(xr::Log::Level::Debug);
    xr::Log::info("XREAL AR Multiscreen starting...");

    xr::Config config = xr::ConfigFile::load();
    xr::App app;

    if (!app.init(config)) {
        xr::Log::error("Failed to initialize application");
        return 1;
    }

    app.run();

    // Save current config on exit so settings persist
    xr::ConfigFile::save(app.config());

    app.shutdown();

    xr::Log::info("Clean exit");
    return 0;
}
