/*
 * @(#)jvm2cvm.h	1.10 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

#ifndef _INCLUDED_JVM2CVM_H
#define _INCLUDED_JVM2CVM_H

/* functions */
#define JVM_InternString	CVMstringIntern

#define JVM_Lseek		CVMioSeek
#define JVM_SetLength		CVMioSetLength
#define JVM_Open		CVMioOpen
#define JVM_Read		CVMioRead
#define JVM_Write		CVMioWrite
#define JVM_Close		CVMioClose
#define JVM_Available		CVMioAvailable
#define JVM_Sync		CVMioSync

#define JVM_Socket		CVMnetSocket
#define JVM_Connect		CVMnetConnect
#define JVM_Listen		CVMnetListen
#define JVM_Accept		CVMnetAccept
#define JVM_SocketAvailable	CVMnetSocketAvailable
#define JVM_Timeout		CVMnetTimeout
#define JVM_Send		CVMnetSend
#define JVM_SendTo		CVMnetSendTo
#define JVM_Recv		CVMnetRecv
#define JVM_RecvFrom		CVMnetRecvFrom
#define JVM_SocketClose		CVMnetSocketClose
#define JVM_SocketShutdown	CVMnetSocketShutdown
#define JVM_GetProtoByName	CVMnetGetProtoByName
#define JVM_SetSockOpt		CVMnetSetSockOpt
#define JVM_GetSockOpt		CVMnetGetSockOpt
#define JVM_GetSockName		CVMnetGetSockName
#define JVM_GetHostName		CVMnetGetHostName
#define JVM_Bind		CVMnetBind

#define JVM_NativePath		CVMioNativePath

/* types */
#define method_size_info	CVMmethod_size_info
#define class_size_info		CVMclass_size_info

/* macros */
#undef JVM_IO_ERR
#define JVM_IO_ERR	   CVM_IO_ERR
#undef JVM_IO_INTR
#define JVM_IO_INTR	   CVM_IO_INTR

#endif /* _INCLUDED_JVM2CVM_H */
