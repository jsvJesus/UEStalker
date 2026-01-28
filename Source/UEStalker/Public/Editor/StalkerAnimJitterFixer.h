#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
class FStalkerAnimJitterFixer
{
public:
	static void Startup();
	static void Shutdown();
};
#endif