///********************************************************************
// VvUtilsW.h - Miscellaneous Windows-specific Vivo utility functions
//
// Author:     Tom Kent, 6/20/96
// Inspector:
//
// Copyright (c) 1996-1997 Vivo Software, Inc.  All Rights Reserved.
///*************************************************************************
// MODIFICATIONS:
//	11/12/96	tkent	Rearranged functions into VvUtils.h (platform-
//						independent stuff), VvUtilsM.h (Mac) and VvUtilsW.h
//						(Windows). Include VvUtils.h regardless of the
//						platform you are compiling for.
//	06/20/96	tkent	Implemented MacUtils.cpp to contain utilities for
//						the Mac port
//
///*************************************************************************

#ifndef VVUTILSW_H
#define VVUTILSW_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FOR_WINMW
int strcmpi(const char *a, const char *b);
#endif

#ifdef __cplusplus
}
#endif

#endif // VVUTILSW_H
