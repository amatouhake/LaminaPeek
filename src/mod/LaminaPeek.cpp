#include "mod/LaminaPeek.h"

#include "ll/api/mod/RegisterHelper.h"

namespace lamina_peek {

LaminaPeek& LaminaPeek::getInstance() {
    static LaminaPeek instance;
    return instance;
}

bool LaminaPeek::load() {
    getSelf().getLogger().debug("Loading...");
    return true;
}

bool LaminaPeek::enable() {
    getSelf().getLogger().debug("Enabling...");
    return true;
}

bool LaminaPeek::disable() {
    getSelf().getLogger().debug("Disabling...");
    return true;
}

} // namespace lamina_peek

LL_REGISTER_MOD(lamina_peek::LaminaPeek, lamina_peek::LaminaPeek::getInstance());
