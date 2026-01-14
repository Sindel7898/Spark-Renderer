#include "NvdiaDLSS_Intergration.h"

#ifdef constant
#undef constant
#endif

#include <sl.h>
#include <sl_dlss.h>
#include <sl_dlss_d.h>
#include <iostream>

void NvdiaDLSS_Intergration::InitDLSS()
{
	HMODULE mod = LoadLibraryA("sl.interposer.dll");

	if (!mod) {
		std::cerr << "Failed to load sl.interposer.dll. Ensure it is in the executable directory.\n";
		return;
	}

	sl_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(mod, "vkGetInstanceProcAddr");

	sl::Preferences pref{};
	sl::Feature features[] = { sl::kFeatureDLSS, sl::kFeatureDLSS_RR }; 
	pref.featuresToLoad = features;
	pref.numFeaturesToLoad = std::size(features);
	pref.renderAPI = sl::RenderAPI::eVulkan;

	sl::Result res;
	if (SL_FAILED(res, slInit(pref))) {
		std::cerr << "Streamline Init failed with error code: " << (int)res << "\n";
	}
}

void NvdiaDLSS_Intergration::CleanUp()
{

}

NvdiaDLSS_Intergration::~NvdiaDLSS_Intergration()
{
	CleanUp();
}