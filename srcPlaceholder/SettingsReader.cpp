/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/SettingsReader.h"

pm::SettingsReader::SettingsReader()
{
}

pm::SettingsReader::~SettingsReader()
{
}

rgn_type pm::SettingsReader::GetImpliedRegion(const std::vector<rgn_type>& regions)
{
    rgn_type rgn{ 0, 0, 0, 0, 0, 0 };

    if (!regions.empty())
    {
        rgn_type impliedRgn = regions.at(0);
        for (size_t n = 1; n < regions.size(); n++)
        {
            const rgn_type& region = regions.at(n);

            if (impliedRgn.sbin != region.sbin || impliedRgn.pbin != region.pbin)
                return rgn;

            if (impliedRgn.s1 > region.s1)
                impliedRgn.s1 = region.s1;
            if (impliedRgn.s2 < region.s2)
                impliedRgn.s2 = region.s2;
            if (impliedRgn.p1 > region.p1)
                impliedRgn.p1 = region.p1;
            if (impliedRgn.p2 < region.p2)
                impliedRgn.p2 = region.p2;
        }

        rgn = impliedRgn;
    }

    return rgn;
}
