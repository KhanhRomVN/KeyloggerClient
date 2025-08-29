#include "persistence/BasePersistence.h"

BasePersistence::BasePersistence(Configuration* config)
    : m_config(config), m_installed(false) {}