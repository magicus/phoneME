/*-----------------------------------------------------------------------------
 *  mangle.h
 *  $Header: s:/src/include/rcs/mangle.h 1.5 1995/07/13 14:59:27 george Exp $
 *
 *  NAME 
 *   Hide / mangle exported names
 *
 *  SYNOPSIS
 *    #include "mangle.h"
 *     ...
 *
 *  DESCRIPTION   
 *
 *  Author:     Win Crofton
 *  Inspector:
 *  Revised (most recent first):
 *  09/27/94 wc   Created
 *
 *  (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 
#ifndef _MANGLE_H_
#define _MANGLE_H_

//---------------------------------------------------
// DibApi.def
#define BitmapToDIB                 aL_TXvELO_AXf1vVqt
#define ChangeBitmapFormat          yL_TXfELO_AXfvaVqt
#define ChangeDIBFormat             bL_TXuELO_AX2fvVqt
#define CopyScreenToBitmap          cL_TXtELO_A3XfvVqt
#define CopyScreenToDIB             wL_TXhELO_AXfvVcqt
#define CopyWindowToBitmap          dL_TXsELO_A4XfvVqt
#define CopyWindowToDIB             yL_TXiELO_AXfvVDqt
#define CreateDIBPalette            vL_TXjELO_AXfvVqtE
#define CreateDIB                   eL_TXqELO_AX5fvVqt
#define DestroyDIB                  fL_TXpELO_AXfv6Vqt
#define DIBError                    uL_TXkELO_AFXfvVqt
#define DIBHeight                   gL_TXoEeL_A7XfvVqt
#define DIBNumColors                tL_TXlEvL_AXfvGVqt
#define DIBToBitmap                 hL_TXnEfL_AXf8vVqt
#define DIBWidth                    sL_TXmEuL_AXfvVqht
#define FindDIBBits                 lL_TXxEjL_AmXfvVMqt
#define GetSystemPalette            LfL_TXxlPELO_ARXfvVqt
#define LoadDIB                     Lg_TXxEmLO_AXTfvVqt
#define PaintBitmap                 Lq_TXxELO_AXfvVqYYt
#define PaintDIB                    Lj_TXxELO_AXfvVYqt
#define PalEntriesOnDevice          LpL_TXxELO_AXfvZVqt
#define PaletteSize                 LkL_wXxI0NL_AXfvVqt
#define PrintDIB                    LL_wXxI3NL_AX2fvVqt
#define PrintScreen                 LRr_wXxI4NL_AXfvVBqt
#define PrintWindow                 LRr_wXxI5NL_A3XfvVqt
#define SaveDIB                     LRr_wXxI7NL_A4XfvVqt
#define PrintAbortDlg               LL_wXxI1NL_AXf1vVqt
#define PrintAbortProc              LL_wXxI2NL_AXfvaVqt
#define VvDrawCursor                LRr_wXxIAXfvVqtNL_A

//---------------------------------------------------
// VivoFile.def
//define VvServiceType               PdL_XxIHK_AX2fv4X
//define VvServiceCreate             QdLXxG_AXfvVBIXL$
//define VvServicePrepare            PdL_XxGKJ_AXfvVXf
//define VvServiceStart              ZxL_XxHKO_AXf1v3X
//define VvServiceCommand            PdL_XxF_A4XfvPKeL
//define VvServiceStop               PsL_XxIGG_AXfvaXf
//define VvServiceUnprepare          PdL_XxKEI_AXfvVXf
//define VvServiceDestroy            AdL_XxG_AX5fvEPtL
//define VvOpenPort                  WdL_wXxIAXNf_VqtN
//define VvOutputMessage             ZxL_XxMFD_AX2fvXf
//define VvSetBitRate                ZxL_XxMKI_AXfvViX

//---------------------------------------------------
// DmngSys.def
//define DMInitialize                qL_TXxEtL_AiXfvVqt
//define DMExecute                   iL_TXxEgL_AXf9vVqt
//define DMTerminate                 kL_TXxEiL_kKXfvVqt
//define DMPrepare                   jL_TXxEhL_AX0fvVqt
//define DMRX_put_next               pL_TXxEsL_AXfvJVqt
//define DMTX_get_next               oL_TXxEqL_LAXfvVqt

//---------------------------------------------------
// VIsdnDrv.def
#define InitIsdnDriver              Lu_TXxPnELO_ASXfvVqst
#define FreeIsdnDriver              LvL_TXxoPELO_AXfvVqtP
#define LoadIsdnDriver              Lh_TXxELO_AXvfvVqt
#define UnloadIsdnDriver            LRr_wXxINL_AAXfvVqtE
#define OpenIsdnDriver              Li_TXxELO_AXfXvVqt
#define CloseIsdnDriver             xL_TXgELO_AXfvVBqt
#define LoadDrivers                 Lt_TXxELO_AXUfvVqt
#define UnloadDrivers               LRr_wXxI9NL_AX5fvVqt
#define WRDCHMesgHandler            LRr_wXxI8NL_AXfvVDqt
#define MakeCall                    Ls_TXxELO_AXWfvWVqt
#define FreeCall                    mL_TXxEkL_AXfvoVqtO
#define AcceptCall                  zL_TXeELO_AXfvVqt
#define RejectCall                  LRr_wXxI6NL_AXfvVcqt
// GAM062595 - Add support for WinISDN
#define WinISDNMesgHandler			BabyStarsInTheSky
#define WinISDN_LoadDrivers  		DiverBilltheShark
#define WinISDN_UnloadDrivers       ZizebethBootDaDaus
// GAM062595 - End

#define VvWaveAudioHandler          Es1ylg_me_jfdmdfs
#define VvSwanWhere                 Ezs1bg_1VIVO_jfdmd
#define VvSwanStop                  Es1cfg_1makes_jfd
#define VvDeinitializeCallControl   Eds1wfg_1me_jfdmdf
#define VvAudioInHandler            Es1dg_1feel_jfdmd
#define VvSwanStart                 Es1yfg_1happy_jfd
#define silenceBuffer               Es1efg_2will_jfdm
#define scaleBufferDown             Es1fdfg_2think_jf
#define VvISDNCallSetup             Es1ufg_2you_jfdmd
#define VvISDNCallAnswer            DEs1gfg_2are_jfdmd
#define VvISDNDiscCall              Es1tfg_2swell_jfd
#define moveSrcToAudio              Es1hfg_2if_jfdmdf
#define initializeMvManAudio        Es1sdgyffg_2you_jf
#define VvInitializeCallControl     Es1ldfbg_2use_jfdm
#define moveAudioToSink             Es1Ldfcg_2VIVO_jfd
#define VvSwanInfo                  Es1rfgw_2320_jfdd
#define VvISDNCallConnected         Es1fg_d3My_jfdmdf
#define VvCallCleared               Es1g_3yis_jfdmdfs
#define VvNotifyLineState           Es1g_Annie3yis_jfdBill
#define VvToneOn                    Es1g_Elizabeth_jfdmdfs
#define VvISDNDiscInd               Es1fg_e3nized_jfd
#define VvISDNCallConnectInd        Es1fg_f3ywith_jfdm
#define getAudioDelay               Es1fg_u3bvivo_jfdm
#define VvAudioHandler              Es1fg_g3caround_jf
#define scaleBufferUp               Es1fg_t4wDont_jfdm
#define VvSwanTimerHandler          Es16TfhGd_4buy_jff
#define VvBMACOutCHandler           Es1fg_s4yany_jf67d
#define VvBMACOutBHandler           Es1fg_l4vimmitatio
#define VvBMACInCHandler            Es1fg_L4evivo_jfdm
#define VvBMACInBHandler            Es1fg_4pfroducts_
#define VvISDNDiscCallDebug         Es1fg_4aus_jfdmdf
#define VvISDNRejectCall            Es1fg_4wtill_jfdm
#define putTimeStampInBuffer        Es1dfg_4hmake_jfd
#define VvAudioOutHandler           Es1g_4yosu_jfdmdf
                                         
//---------------------------------------------------
// VSIntr.def
#define VvCaptureCompleteISR        LRr_wXxINL_AAXf9vVqt
#define VvInitQueue                 WdL_wXxIAXfvVqhtNL_LxlF
//define VvEnqueue                   LRr_wXxIAXf1vVqtNL_LxF
//define VvDequeue                   LRr_wXxINL_AAXNfvVqt
#define VvRemFromQueue              WdL_CrOXxVfTddo_N
#define VvIsQueueEmpty              WdL_wXxIAX0fvVqtNL_Lx6F
//define VvCountQueue                LRr_wXxINL_AAmXfvVMqt
#define VvWarning                   PdLXxLCK_AX_fvVXf
//define VvGlobalAlloc               WdL_wXxIAX5fvVqtNL_LxeF
#define VvMalloc                    WdL_wXxIAkKXfvVqtNL_Lx2F
//define VvGlobalFree                WdL_wXxIAXfvVqtENL_LxfF
#define VvFree                      LRr_wXxIAXfvaVqtNL_LxzF
#define VvMessageAlloc              WdL_wXxILAXfvVqtNL_Lx9F
#define VvMessageFree               WdL_wXxIAmXfvVMqtNL_Lx11F
#define VxDGetAPI                   PdL_XxLMG_AXfvaXf
#define VxDCallAPI0                 PdL_XxLHK_AXf1vXf
#define VvGetAbsTimeMicrosec        LRr_wXxIAX2fvVqtNL_LxyF
#define VvGetDeltaTimeMicrosec      LRr_wXxIA3XfvVqtNL_LxwF
#define VvGetAbsTimeMillisec        LRr_wXxIAXfvVBqtNL_LxxF
#define VvGetDeltaTimeMillisec      WdL_wXxIAXfvVcqtNL_LxvF
#define VvRTWInit                   WdL_LLNKOrOC_IfT_AXfvVoN
#define VvRTWTerm                   PdL_mxUxCBNDDngLD
#define VvRTWPoll                   WdL_LLPCHrOticfT_AX5fvoN
#define VvRTWTimeout                PdL_XxE_A3XfvNOAT
#define VvRTWCancel                 WdL_LLMMNrO5RefT_A3XfvoN
#define VvRTWSetTimeQuota           PdL_XxZxCBKCPL5NM
#define VvRTWSetPriority            PdL_dxXxBPOCEMicK
#define VvRTWResetPriority          WdL_LLBIFrORESfTXxXxoN
#define VvRTWTimeLate               PdL_XxixCIGEDa_wP
#define VvRTWTimeSched              PdLXxF_AXfvVLDISE
#define VvRTWSetPreTranslate        PdL_LLBJHrOHO_fToLN
#define VvRTWOneLoop                WdL_LLOJNrOEIGfT_AXfvVoN
#define VvRTWMainLoop               WdL_LLOEMrOZERfT_A4XfvoN
#define VvRTWRegisterHook           WdL_CLLNErOL5MfToN

#define VvInitVxd                   WdL_wXxIAXf9vVqtNL_LxnF
#define VvTermVxd                   PdL_XxLAE_A4XfvXf
#define VvPollVxdMsgs               WdL_wXxIAXfvoVqtONL_LxllF
#define VvReportVxdApiErr           WdL_LLMLNrOCamfToN
#define VvTUsingVxd                 PdL_XxLCB_AXfvVFX
#define VvHookIpcRing0              WdL_wXxIAFXfvVqtNL_LxhF
#define VvUnhookIpcRing0            PdL_XxLCD_AX5fv7X
#define VvDoNowRing0                LRr_wXxINL_AAXfvoVqtO
#define VvLpFlatFmLpSelOff          WdL_wXxIAXfvJVqtNL_Lx3F
#define VvHandleTimerRing3          WdL_wXxIAXfv6VqtNL_LxgF

#define VvIsLogNextCallOn           WdL_wXxIAiXfvVqtNL_LxaF
#define VvStartLogNextCall          PdL_XxKFE_A3XfvX5
#define VvStopLogNextCall           PdLXxKFG_AXf_vVXf
#define VvAbortLogNextCall          LRr_wXxINL_AjXf8vVqt

#define VvGetString                 iRr_wXxINL_AAXf7vVt

#define VvMallocNoPlock             hfFXfvFXfvVqVq
#define VvFreeNoPlock               jRr_wXxIwinLXf8vVqt

#define VvHdllSetClientBitRate      sfg_VIVO_jfdmdfs
#define VvHdllSetClientInfo         sdg_320_jfdmdfs
#define VvHdllAddClient             sdf_is_jfdmdfs
#define VvHdllRemoveClient          sDfg_good_jfdmdfs
#define VvHdllGetClientInfo         sdfR_for_jfdmdfs
#define VvHdllFlushClient           sdlg_me_jfdmdfs
#define VvHdllGetClientError        sdg_1VIVO_jfdmdfs
#define GetDcibChan                 sdfg_1makes_jfdmdfs
#define VvHdllDcibInit              sdfg_1me_jfdmdfs
#define VvHdllDcibEnd               dfg_1feel_jfdmdfs
#define VvHdllDcibAddChan           sdfg_1happy_jfdmdfs
#define VvHdllDcibRemChan           sfg_2People_jfdmdfs
#define VvMuxOutData                sdfg_2will_jfdmdfs
#define VvHdllWriteBlock            Fsdfg_2think_jfdmdfs
#define VvDemuxInData               sdfg_2you_jfdmdfs
#define VvHdllReadBlock             adfg_2are_jfdmdfs
#define GetHdlcFlag                 Rdfg_2swell_jfdmdfs
#define CrcCCITT                    sdfg_2if_jfdmdfs
#define HdlcStuff                   Osdgffg_2you_jfdmdfs
#define HdlcUnstuff                 osdfg_2use_jfdmdfs
#define InitHdlc                    rsdfg_2VIVO_jfdmdfs
#define VvRTWMainLoopInit           dgrfg_2320_jfddfs
#define VvRTWOneLoopLevel           dgfg_3My_jfdmdfs
#define VvRTWWinMsgHook             dfg_3is_jfdmdfs
#define VvSWANRead                  dlifg_3orga_jfdmdfs
#define VvSWANStop                  djfg_3nized_jfdmdfs
#define VvSWANOutStart              jdfg_3with_jfdmdfs
#define VvSWANInStart               qdfg_3vivo_jfdmdfs
#define VvSWANOutISR                fdfg_3around_jfdmdfs
#define VvServiceOutput             sdfg_4Dont_jfdmdfs
#define VvSWANInISR                 s56TfG_4buy_jffs
#define VvSwanWEP                   zdfg_4any_jf67dmdfs
#define convertuLawToLinear         adfg_4immitation_jfdmdfs
#define convertuLawToLinearSwan     ydfg_4vivo_jfdmdfs
#define uLawToLinearTableGen        bdfg_4products_jfdmdfs
#define convertLinearTouLawSlow     gdfg_4as_jfdmdfs
#define linearTouLawTableGen        aadfg_4they_jfdmdfs
#define uLawToaLawTableGen          kdfg_4will_jfdmdfs
#define aLawTouLawTableGen          p0dfg_4make_jfdmdfs
#define generateCenterClipperTable  dfg_4you_jfdmdfs
#define generateInputGainTable      dfg_4feel_jfdmdfs
#define generateLineGainTable       xdfg_4sick_jfdmdfs
#define initializeEchoSuppressor    hudfg_5Only_jfdmdfs
#define VvInitErrorSys              asdfg_5to_jfdmdfs
#define VvTermErrorSys              sdfg_5use_jfdmdfs
#define VvAllocAllDOSMemory         bsdfg_5imatation_jfdmdfs
#define VvFreeAllocedDOSMemory      sdfg_5vivo_jfdmdfs
//define VvFillBuffer                sdfg_5products_jfdmdfs
#define VvGlobalAllocNoPlock        sdfg_6life_jfdmdfs
#define VvGlobalFreeNoPlock         dsdfg_6you_jfdmdfs
#define VvCheckQueue                sdfg_6stinking_jfdmdfs
#define VvCreateQueue               sdfg_6rotten_jfdmdfs
#define VvDeleteQueue               hsdfg_6ugly_jfdmdfs
#define VvInitSemaphore             sdfg_6computer_jfdmdfs
#define VvIncrementSemaphore        sdfg_6nerd_jfdmdfs
#define VvDecrementSemaphore        zsdfg_7if_jfdmdfs
#define VvSpigotFrameLen            Tsdfg_7you_jfdmdfs
#define VvSpigotImageLen            sdfg_7can_jfdmdfs
#define VvSpigotFrameOffset         Esdfg_7read_jfdmdfs
#define VvSpigotImageOffset         sdfg_7this_jfdmdfs
#define VvISDNLoopbackDisable       Sdfg_7your_jfdmdfs
#define VvISDNLoopbackEnable        usdfg_7a_jfdmdfs
#define NotifyDataControl           sdfg_7wakner_jfdmdfs


//---------------------------------------------------
// Vision.def
//define fnEnumReallyEx              nL_TXxEpL_AXNfvVqt
//define VisionPreTranslateMessage   LRr_wXxINL_AAFXfvVqt

//---------------------------------------------------
// VivoVCom.def
//define vivovcom                    LRr_wXxINL_AA7XfvVqt
//define vvGetVIVOVCOMError          WdL_wXxIAXfvVDqtNL_LxdF
//define vvGetVIVOVCOMChannels       WdL_wXxIA4XfvVqtNL_LxgF
//define vvCheckService              LRr_wXxINL_AAiXfvVqt
//define vvRegisterWindow            WdL_LL_LrOAAfT_oN
//define vcpCallBack                 LRr_wXxINL_AAXfv6Vqt
//define vvInitDrv                   WdL_wXxIA7XfvVqtNL_LxiF
//define vvInitDrv32                 WdL_wXxIAXf8vVqtNL_LxjF
//define vvInitDrv16                 WdL_wXxIAXfvGVqtNL_LxkF
//define vvcall16                    LRr_wXxINL_AAXfvVqht
//define vivovcomIdle                LRr_wXxINL_AAXfvGVqt
//define vvTermDrv                   ARr_wXxI_NQATeOXfvGVqt

//---------------------------------------------------
// VivoVCpd.def
//define vvCommEvent                 LRr_wXxINL_AAXfvJVqt
//define vvCommIdle                  LRr_wXxINL_AAkKXfvVqt
//define vvCommOpen                  LRr_wXxINL_ALAXfvVqt
//define vvCommClose                 LRr_wXxINL_AAX0fvVqt
// These are already covered by Vivovcom
//define VvServiceType               PdL_XxIHK_AX2fv4X
//define VvServiceCreate             PdLXxG_AXfvVBIXL$
//define VvServicePrepare            PdL_XxGKJ_AXfvVXf
//define VvServiceStart              PdL_XxHKO_AXf1v3X
//define VvServiceCommand            PdL_XxF_A4XfvPKeL
//define VvServiceStop               PdL_XxIGG_AXfvaXf
//define VvServiceUnprepare          PdL_XxKEI_AXfvVXf
//define VvServiceDestroy            PdL_XxG_AX5fvEPtL

//---------------------------------------------------
// CoderDll
#define OpenCoderDLL                Z_3jKAX_fIAmX_X0l    
#define OpenEncoder                 jKOAXfIAmX_X0fvVq    
#define GetEncoderPtr               EzpdAXfImX_X0fFlq    
#define BCHEncode                   aGG_AXfIAXNXfvJVq    
#define GetEncoderStats             yHK_AX2IAXfkKXfvV    
#define Encode                      bEI_AXfLrOAAXfvVq    
#define PreProcess                  xFEA3XXxV_fRHTtdr
#define Recon                       cFG_AXfLNrORHt3Gs
#define SetEncoderParams            wAE_A4XMNrOhyTRHq
#define SetPreviewParams            dCB_AXmdermafKO4r
#define CloseEncoder                yCDAXeie_wat5EMrO
#define OpenDecoder                 eCK_AXscodtrfJNrt
#define GetDecoderPtr               K2OAX9fIAmX_X0fvq    
#define BCHDecode                   vHKAXeo_depafCHrh
#define Decode                      fMG_AXartfNErOeat
#define CloseDecoder                uFD_AX2IFrdstr5bg
#define EncPictureToDIB             gKI_AXfJHr0epaRTH
#define DecPictureToDIB             tBPO9CxBPO4artTYE
#define VvEncGetMBMap               hCBKC_iasdgf_gfg4
#define VvDecGetMBMap               sCB_ND3QEoT2kd9f5



#endif

                                               














