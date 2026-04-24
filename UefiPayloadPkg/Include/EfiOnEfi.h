#pragma once

#define EFI_ON_EFI_SIGNATURE 0xEFEF

#pragma pack(1)
typedef struct
{
    UINT16  Signature;

    UINT64  FbBase;
    UINT32  FbWidth;
    UINT32  FbHeight;
    UINT32  FbBytesPerRow;

    UINTN   AcpiTable;
    UINTN   SmbiosTable;

    MEMORY_MAP_INFO *MemoryMap;
} EFI_ON_EFI_INFO;
#pragma pack()