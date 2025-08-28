#include "persistence/BasePersistence.h"
#include "core/Configuration.h"

BasePersistence::BasePersistence(Configuration* config)
    : m_config(config), m_installed(false) {}