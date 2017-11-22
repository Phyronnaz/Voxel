// Copyright 2017 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"


/**
 * The public interface to this module
 */
class IVoxel : public IModuleInterface
{
};

DECLARE_STATS_GROUP(TEXT("Voxels"), STATGROUP_Voxel, STATCAT_Advanced);
