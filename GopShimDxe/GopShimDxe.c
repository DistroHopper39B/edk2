/* SPDX-License-Identifier: BSD-2-Clause-Patent */

/* INCLUDES */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Protocol/UgaDraw.h>

#include "AppleVideo.h"

/* GLOBALS */

EFI_GRAPHICS_OUTPUT_PROTOCOL            NewGop;
EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE       NewGopMode;
EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    NewGopInfo;

FRAME_BUFFER_CONFIGURE                  *FbConf;

/* EFI PROTOCOL FUNCTIONS */

EFI_STATUS
EFIAPI
GopShimQueryMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
                IN UINT32 ModeNumber,
                OUT UINTN *SizeOfInfo,
                OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info)
{
    // We only have one mode, so this is easy
    if ((Info == NULL) || (SizeOfInfo == NULL) || ((UINTN)ModeNumber >= This->Mode->MaxMode))
    {
        return EFI_INVALID_PARAMETER;
    }
    
    *Info        = This->Mode->Info;
    *SizeOfInfo  = This->Mode->SizeOfInfo;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GopShimSetMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
                UINT32 ModeNumber)
{
    // We only have one mode, so this is easy
    if (((UINTN) ModeNumber > This->Mode->MaxMode))
    {
        return EFI_UNSUPPORTED;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GopShimBlt(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
            EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
            UINTN SourceX,
            UINTN SourceY,
            UINTN DestinationX,
            UINTN DestinationY, 
            UINTN Width,
            UINTN Height,
            UINTN Delta)
{
    // Inspired by OvmfPkg/Bhyve/BhyveRfbDxe/GopScreen.c
    EFI_TPL     OriginalTPL;
    EFI_STATUS  Status;
    
    if ((UINT32) BltOperation >= EfiGraphicsOutputBltOperationMax)
    {
        return EFI_INVALID_PARAMETER;
    }
    
    if (Width == 0 || Height == 0)
    {
        return EFI_INVALID_PARAMETER;
    }
    
    // Raise TPL notify to prevent other things from happening
    OriginalTPL = gBS->RaiseTPL(TPL_NOTIFY);
    
    Status = FrameBufferBlt(FbConf,
                            BltBuffer,
                            BltOperation,
                            SourceX,
                            SourceY,
                            DestinationX,
                            DestinationY, 
                            Width, 
                            Height,
                            Delta);
    
    gBS->RestoreTPL(OriginalTPL);
    
    return Status;
}

/* ENTRY POINT */

EFI_STATUS
EFIAPI
UefiMain(EFI_HANDLE ImageHandle,
        EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS                      Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop;
    APPLE_SCREEN_INFO_PROTOCOL      *AppleScreenInfo;
    UINT64                          BaseAddress, FrameBufferSize; 
    UINT32                          BytesPerRow, Width, Height, Depth;
    UINTN                           FbConfSize = 0;
    
    DEBUG((DEBUG_INIT, "GopShim Starting\n"));
    
    // Make sure we actually need this shim
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &Gop);
    if (Status == EFI_SUCCESS)
    {
        // Shim not required
        DEBUG((DEBUG_INFO, "GOP found! This shim is not required.\n"));
        return Status;
    }
    
    // Look for Apple screen info
    Status = gBS->LocateProtocol(&gAppleScreenInfoProtocolGuid, NULL, (VOID **) &AppleScreenInfo);
    if (Status != EFI_SUCCESS)
    {
        // Neither protocol was discovered
        DEBUG((DEBUG_ERROR, "Apple graphics protocol not found! Are you running this on an itanium, or a machine with no screen? Status = %d\n", Status));
        return Status;
    }
    
    DEBUG((DEBUG_INFO, "Found Apple graphics protocol.\n"));
    Status = AppleScreenInfo->GetInfo(AppleScreenInfo, &BaseAddress,
                                &FrameBufferSize, &BytesPerRow, &Width, &Height, &Depth);
    if (Status != EFI_SUCCESS)
    {
        DEBUG((DEBUG_ERROR, "Could not get Apple graphics protocol information! Status = %d\n", Status));
        return Status;
    }
    
    DEBUG((DEBUG_INFO, "Setting up GOP\n"));
    
    //
    // Set up new GOP
    // Width is not always equal to BytesPerRow / 4, thanks Apple...
    // Correctly implemented boot loaders will have no issue dealing with this, but some might have
    // corrupted text or print off the screen. If this happens, change NewGopInfo.HorizontalResolution
    // to be equal to BytesPerRow / 4.
    //
    
    NewGopInfo.Version                          = 0;
    NewGopInfo.HorizontalResolution             = Width; 
    NewGopInfo.VerticalResolution               = Height;
    NewGopInfo.PixelFormat                      = PixelBlueGreenRedReserved8BitPerColor; // Always; if it wasn't macOS wouldn't know
    NewGopInfo.PixelInformation.RedMask         = 0x00FF0000;
    NewGopInfo.PixelInformation.GreenMask       = 0x0000FF00;
    NewGopInfo.PixelInformation.BlueMask        = 0x000000FF;
    NewGopInfo.PixelInformation.ReservedMask    = 0xFF000000;
    NewGopInfo.PixelsPerScanLine                = (BytesPerRow / 4);
    
    NewGopMode.MaxMode                          = 1; // Only one mode supported
    NewGopMode.Mode                             = 0; // Only one mode supported
    NewGopMode.Info                             = &NewGopInfo;
    NewGopMode.SizeOfInfo                       = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    NewGopMode.FrameBufferBase                  = (EFI_PHYSICAL_ADDRESS) (UINTN) BaseAddress;
    NewGopMode.FrameBufferSize                  = FrameBufferSize;
    
    // Set up BLT support
    // Inspired by OvmfPkg/Bhyve/BhyveRfbDxe/GopScreen.c
    
    Status = FrameBufferBltConfigure((VOID *) (UINTN) NewGopMode.FrameBufferBase,
                                    NewGopMode.Info,
                                    FbConf,
                                    &FbConfSize);
    // Will fail the first try, thanks Intel
    if ((Status == EFI_BUFFER_TOO_SMALL) || (Status == EFI_INVALID_PARAMETER))
    {
        FbConf = AllocatePool(FbConfSize);
        Status = FrameBufferBltConfigure((VOID *) (UINTN) NewGopMode.FrameBufferBase,
                                    NewGopMode.Info,
                                    FbConf,
                                    &FbConfSize);
        if (Status != EFI_SUCCESS) {
            DEBUG((DEBUG_ERROR, "Cannot configure BLT! Status = %d, FbConfSize = %d\n", Status, FbConfSize));
            return Status;
        }
    }
    
    NewGop.QueryMode                            = GopShimQueryMode;
    NewGop.SetMode                              = GopShimSetMode;
    NewGop.Blt                                  = GopShimBlt;
    NewGop.Mode                                 = &NewGopMode;
    
    DEBUG((DEBUG_INFO, "GOP setup complete\n"));
    
    // Install new protocol into EFI
    EFI_HANDLE NewHandle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces(&NewHandle, &gEfiGraphicsOutputProtocolGuid, &NewGop, NULL);
    if (Status != EFI_SUCCESS)
    {
        DEBUG((DEBUG_INFO, "Cannot create new protocol. Status = %d\n", Status));
        return Status;
    }

    
    return EFI_SUCCESS;
}