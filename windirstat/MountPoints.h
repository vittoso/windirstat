// MountPoints.h - Declaratio of CMountPoins
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
// Copyright (C) 2004-2024 WinDirStat Team (windirstat.net)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#pragma once

class CReparsePoints final
{
    struct SPointVolume
    {
        CStringW point;  // Path like "mount\backup\"
        CStringW volume; // Volume identifier
        DWORD flags;     // File system flags (see GetVolumeInformation documentation)

        SPointVolume()
            : flags(0)
        {
        }
    };

    using PointVolumeArray = CArray<SPointVolume, SPointVolume&>;

public:
    ~CReparsePoints();
    void Initialize();
    bool IsVolumeMountPoint(CStringW path);
    bool IsFolderJunction(const CStringW& path);
    bool IsFolderJunction(DWORD attr);

private:
    void Clear();
    void GetDriveVolumes();
    void GetAllMountPoints();

    bool IsVolumeMountPoint(CStringW volume, CStringW path) const;

    // m_drive contains the volume identifiers of the Drives A:, B: etc.
    // mdrive[0] = Volume identifier of A:\.
    CArray<CStringW, LPCWSTR> m_drive;

    // m_volume maps all volume identifiers to PointVolumeArrays
    CMap<CStringW, LPCWSTR, PointVolumeArray*, PointVolumeArray*> m_volume;
};
