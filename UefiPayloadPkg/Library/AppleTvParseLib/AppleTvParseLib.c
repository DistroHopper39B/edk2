/** @file
  This library will parse the Apple TV information struct and extract those required
  information.

  Copyright (c) 2024, DistroHopper39B. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/BlParseLib.h>
#include <IndustryStandard/Acpi.h>
#include <AppleTv.h>

/**
  This function retrieves the parameter base address from boot loader.

  This function will get bootloader specific parameter address for UEFI payload.
  e.g. HobList pointer for Slim Bootloader, and coreboot table header for Coreboot.

  @retval NULL            Failed to find the GUID HOB.
  @retval others          GUIDed HOB data pointer.

**/
VOID *
EFIAPI
GetParameterBase (
  VOID
  )
{
  APPLETV_PEI_HANDOFF_DATA  *HandoffData;
  //
  // AppleTV will pass the information structure to BootloaderParameter
  //
  
  HandoffData = (APPLETV_PEI_HANDOFF_DATA *) (UINT32) GET_BOOTLOADER_PARAMETER ();
  
  return HandoffData;  
}

/**
  Acquire the memory map information.

  @param  MemInfoCallback     The callback routine
  @param  Params              Pointer to the callback routine parameter

  @retval RETURN_SUCCESS     Successfully find out the memory information.
  @retval RETURN_NOT_FOUND   Failed to find the memory information.

**/
RETURN_STATUS
EFIAPI
ParseMemoryInfo (
  IN  BL_MEM_INFO_CALLBACK  MemInfoCallback,
  IN  VOID                  *Params
  )
{
  APPLETV_PEI_HANDOFF_DATA  *HandoffData;
  UINTN                     Index;
  
  HandoffData = GetParameterBase ();
  
  for (Index = 0; Index < HandoffData->MemoryMap->Count; Index++) {
    MemInfoCallback (&HandoffData->MemoryMap->Entry[Index], Params);
  }
  
  return RETURN_SUCCESS;
}

/**
  Acquire SMBIOS table from Apple TV.

  @param  SmbiosTable               Pointer to the SMBIOS table info.

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseSmbiosTable (
  OUT UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmbiosTable
  )
{
  APPLETV_PEI_HANDOFF_DATA  *HandoffData;
  
  HandoffData = GetParameterBase ();
  
  SmbiosTable->SmBiosEntryPoint = (UINT64)(UINTN)HandoffData->SmbiosTable;
  
  return RETURN_SUCCESS;
}

/**
  Acquire ACPI table from Apple TV.

  @param  AcpiTableHob              Pointer to the ACPI table info.

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseAcpiTableInfo (
  OUT UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTableHob
  )
{
  APPLETV_PEI_HANDOFF_DATA  *HandoffData;
  
  HandoffData = GetParameterBase ();
  
  AcpiTableHob->Rsdp = (UINT64)(UINTN)HandoffData->AcpiTable;
  
  return RETURN_SUCCESS;
}

/**
  Find the serial port information

  @param  SerialPortInfo     Pointer to serial port info structure

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
EFIAPI
ParseSerialInfo (
  OUT SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  //
  // We hardcode these values to COM1 on the Apple TV for debugging purposes.
  //
  SerialPortInfo->BaseAddr    = 0x3F8;
  SerialPortInfo->RegWidth    = 1;
  SerialPortInfo->Type        = PLD_SERIAL_TYPE_IO_MAPPED;
  SerialPortInfo->Baud        = 115200;
  SerialPortInfo->InputHertz  = 0; // unused
  SerialPortInfo->UartPciAddr = 0; // unused
  
  return RETURN_SUCCESS;
}

/**
  Find the video frame buffer information

  @param  GfxInfo             Pointer to the EFI_PEI_GRAPHICS_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
ParseGfxInfo (
  OUT EFI_PEI_GRAPHICS_INFO_HOB  *GfxInfo
  )
{
  APPLETV_PEI_HANDOFF_DATA              *HandoffData;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *GfxMode;
  
  HandoffData = GetParameterBase ();
    
  DEBUG ((DEBUG_INFO, "Dumping video information now.\n"));
  DEBUG ((DEBUG_INFO, "Video base address: 0x%08X\n", HandoffData->VideoBase));
  DEBUG ((DEBUG_INFO, "Video width: %d\n", HandoffData->VideoWidth));
  DEBUG ((DEBUG_INFO, "Video height: %d\n", HandoffData->VideoHeight));
  DEBUG ((DEBUG_INFO, "Video depth: %d\n", HandoffData->VideoDepth));
  DEBUG ((DEBUG_INFO, "Video pitch: %d\n", HandoffData->VideoPitch));
  
  GfxMode                                 = &GfxInfo->GraphicsMode;
  
  GfxMode->Version                        = 0;
  GfxMode->HorizontalResolution           = HandoffData->VideoWidth;
  GfxMode->VerticalResolution             = HandoffData->VideoHeight;
  GfxMode->PixelsPerScanLine              = (HandoffData->VideoPitch << 3) / HandoffData->VideoDepth;
  
  GfxMode->PixelFormat                    = PixelBlueGreenRedReserved8BitPerColor;
  GfxMode->PixelInformation.RedMask       = 0x00FF0000;
  GfxMode->PixelInformation.GreenMask     = 0x0000FF00;
  GfxMode->PixelInformation.BlueMask      = 0x000000FF;
  GfxMode->PixelInformation.ReservedMask  = 0xFF000000;
  
  GfxInfo->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)HandoffData->VideoBase;
  GfxInfo->FrameBufferSize = HandoffData->VideoPitch * HandoffData->VideoHeight;
  
  return RETURN_SUCCESS;  
}

/**
  Find the video frame buffer device information

  @param  GfxDeviceInfo      Pointer to the EFI_PEI_GRAPHICS_DEVICE_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information.

**/
RETURN_STATUS
EFIAPI
ParseGfxDeviceInfo (
  OUT EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *GfxDeviceInfo
  )
{
  return RETURN_NOT_FOUND;
}

/**
  Parse and handle the misc info provided by bootloader

  @retval RETURN_SUCCESS           The misc information was parsed successfully.
  @retval RETURN_NOT_FOUND         Could not find required misc info.
  @retval RETURN_OUT_OF_RESOURCES  Insufficant memory space.

**/
RETURN_STATUS
EFIAPI
ParseMiscInfo (
  VOID
  )
{
  return RETURN_SUCCESS;
}
