/** @file
  Apple TV PEI module include file.

  Copyright (c) 2024, DistroHopper39B. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _APPLETV_PEI_H_INCLUDED_
#define _APPLETV_PEI_H_INCLUDED_

typedef struct {    
  /* Memory map info. */
  MEMORY_MAP_INFO *MemoryMap;
    
  /* Video info. */
  UINT32 VideoBase;
  UINT32 VideoWidth;
  UINT32 VideoHeight;
  UINT32 VideoPitch;
  UINT32 VideoDepth;
    
  /* ACPI and SMBIOS pointers. */
  UINT32 AcpiTable;
  UINT32 SmbiosTable;
} APPLETV_PEI_HANDOFF_DATA;



#endif