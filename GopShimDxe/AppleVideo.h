/* SPDX-License-Identifier: BSD-2-Clause-Patent */

#ifndef _APPLE_VIDEO_H_INCLUDED_
#define _APPLE_VIDEO_H_INCLUDED_

//
// Apple's proprietary screen info protocol GUID
//

EFI_GUID gAppleScreenInfoProtocolGuid = {0xe316e100, 0x0751, 0x4c49, {0x90, 0x56, 0x48, 0x6c, 0x7e, 0x47, 0x29, 0x03}};

typedef struct _APPLE_SCREEN_INFO_PROTOCOL APPLE_SCREEN_INFO_PROTOCOL;

typedef EFI_STATUS (EFIAPI *GetAppleScreenInfo)(APPLE_SCREEN_INFO_PROTOCOL *This,
													UINT64 *BaseAddress,
													UINT64 *FrameBufferSize,
													UINT32 *BytesPerRow,
													UINT32 *Width,
													UINT32 *Height,
													UINT32 *Depth);
													
struct _APPLE_SCREEN_INFO_PROTOCOL {
	GetAppleScreenInfo	GetInfo;
};

#endif