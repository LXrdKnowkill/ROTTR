#pragma once
#include "../Kiero/kiero.h"

namespace KieroHooks
{
	inline bool Imgui_Init = false;
	void Init();
	void Shutdown();
	void InitImgui();
}