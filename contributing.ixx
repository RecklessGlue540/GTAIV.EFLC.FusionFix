module;

#include <common.hxx>

export module contributing;

import common;

class Contributing
{
public:
    Contributing()
    {
        FusionFix::onInitEvent() += []()
        {
            // Description strings fix for Reflection Resolution, Water Quality, Shadow Quality and Texture Filter Quality
            {
                static auto GRPH_VID_MODE = "GRPH_REFL_QUAL"
                static auto GRPH_VID_MODE = "GRPH_WATER_QUAL"
                static auto GRPH_VID_MODE = "GRPH_SHA_QUAL"
                static auto GRPH_TEX_QUAL = "GRPH_ANISO_QUAL"

                auto pattern = hook::pattern("68 B4 8E F8 00");
                if (!pattern.empty())
                    injector::WriteMemory(pattern.get_first(1), &aGrphVidMode_0[0], true);

                pattern = hook::pattern("68 A4 8E F8 00");
                if (!pattern.empty())
                    injector::WriteMemory(pattern.get_first(1), &aGrphVidMode_1[0], true);
                
                pattern = hook::pattern("68 94 8E F8 00");
                if (!pattern.empty())
                    injector::WriteMemory(pattern.get_first(1), &aGrphVidMode_2[0], true);

                pattern = hook::pattern("68 74 8E F8 00");
                if (!pattern.empty())
                    injector::WriteMemory(pattern.get_first(1), &aGrphTexQual_0[0], true);
            }
        };
    }
} Contributing;