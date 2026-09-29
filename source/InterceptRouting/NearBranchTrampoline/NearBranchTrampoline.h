#pragma once

#include "dobby/dobby_internal.h"

#include "InterceptRouting/RoutingPlugin.h"

class NearBranchTrampolinePlugin : public RoutingPluginInterface {};

inline bool g_enable_near_trampoline = DOBBY_NEAR_BRANCH_DEFAULT;
