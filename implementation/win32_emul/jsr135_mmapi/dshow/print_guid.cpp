#include <control.h>
#include <stdio.h>
#include <strmif.h>
#include <uuids.h>
#include <wmsdkidl.h>

#include <initguid.h>

#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#include <dxva.h>
#pragma warning(pop)

#include <wmcodecdsp.h>

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
	va_list        args;

	va_start(args, fmt);
    vsprintf( str8, fmt, args );
	va_end(args);

    OutputDebugString( str8 );
}

struct guid_item
{
    GUID guid;
    CHAR const *name;
};

#define define_guid_item(name) name, #name

guid_item const static guid_items[]=
{
    define_guid_item(DXVA_ModeWMV9_A                ),
    define_guid_item(DXVA_ModeWMV9_B                ),
    define_guid_item(DXVA_ModeWMV9_C                ),
    define_guid_item(FORMAT_MPEGVideo               ),
    define_guid_item(FORMAT_None                    ),
    define_guid_item(FORMAT_VideoInfo               ),
    define_guid_item(FORMAT_VideoInfo2              ),
    define_guid_item(FORMAT_WaveFormatEx            ),
    define_guid_item(GUID_NULL                      ),
    define_guid_item(IID_IAMDeviceRemoval           ),
    define_guid_item(IID_IAMFilterMiscFlags         ),
    define_guid_item(IID_IAMOpenProgress            ),
    define_guid_item(IID_IAMPushSource              ),
    define_guid_item(IID_IAsyncReader               ),
    define_guid_item(IID_IBasicAudio                ),
    define_guid_item(IID_IBasicVideo                ),
    define_guid_item(IID_IFileSourceFilter          ),
    define_guid_item(IID_IKsPropertySet             ),
    define_guid_item(IID_IMediaPosition             ),
    define_guid_item(IID_IMediaSeeking              ),
    define_guid_item(IID_IPersist                   ),
    define_guid_item(IID_IStream                    ),
    define_guid_item(IID_IStreamBuilder             ),
    define_guid_item(IID_IVideoWindow               ),
    define_guid_item(MEDIASUBTYPE_AIFF              ),
    define_guid_item(MEDIASUBTYPE_AU                ),
    define_guid_item(MEDIASUBTYPE_Asf               ),
    define_guid_item(MEDIASUBTYPE_Avi               ),
    define_guid_item(MEDIASUBTYPE_DOLBY_AC3         ),
    define_guid_item(MEDIASUBTYPE_DOLBY_AC3_SPDIF   ),
    define_guid_item(MEDIASUBTYPE_DRM_Audio         ),
    define_guid_item(MEDIASUBTYPE_DVD_LPCM_AUDIO    ),
    define_guid_item(MEDIASUBTYPE_DssAudio          ),
    define_guid_item(MEDIASUBTYPE_DssVideo          ),
    define_guid_item(MEDIASUBTYPE_IEEE_FLOAT        ),
    define_guid_item(MEDIASUBTYPE_MPEG1Audio        ),
    define_guid_item(MEDIASUBTYPE_MPEG1AudioPayload ),
    define_guid_item(MEDIASUBTYPE_MPEG1Packet       ),
    define_guid_item(MEDIASUBTYPE_MPEG1Payload      ),
    define_guid_item(MEDIASUBTYPE_MPEG1System       ),
    define_guid_item(MEDIASUBTYPE_MPEG1Video        ),
    define_guid_item(MEDIASUBTYPE_MPEG1VideoCD      ),
    define_guid_item(MEDIASUBTYPE_MPEG2_AUDIO       ),
    define_guid_item(MEDIASUBTYPE_MSAUDIO1          ),
    define_guid_item(MEDIASUBTYPE_NV11              ),
    define_guid_item(MEDIASUBTYPE_NV12              ),
    define_guid_item(MEDIASUBTYPE_None              ),
    define_guid_item(MEDIASUBTYPE_PCM               ),
    define_guid_item(MEDIASUBTYPE_RAW_SPORT         ),
    define_guid_item(MEDIASUBTYPE_RGB24             ),
    define_guid_item(MEDIASUBTYPE_RGB32             ),
    define_guid_item(MEDIASUBTYPE_RGB555            ),
    define_guid_item(MEDIASUBTYPE_RGB565            ),
    define_guid_item(MEDIASUBTYPE_RGB8              ),
    define_guid_item(MEDIASUBTYPE_SPDIF_TAG_241h    ),
    define_guid_item(MEDIASUBTYPE_UYVY              ),
    define_guid_item(MEDIASUBTYPE_WAVE              ),
    define_guid_item(MEDIASUBTYPE_WMVR              ),
    define_guid_item(MEDIASUBTYPE_Y41P              ),
    define_guid_item(MEDIASUBTYPE_YUY2              ),
    define_guid_item(MEDIASUBTYPE_YV12              ),
    define_guid_item(MEDIASUBTYPE_YVYU              ),
    define_guid_item(MEDIATYPE_AUXLine21Data        ),
    define_guid_item(MEDIATYPE_AnalogAudio          ),
    define_guid_item(MEDIATYPE_AnalogVideo          ),
    define_guid_item(MEDIATYPE_Audio                ),
    define_guid_item(MEDIATYPE_File                 ),
    define_guid_item(MEDIATYPE_Interleaved          ),
    define_guid_item(MEDIATYPE_LMRT                 ),
    define_guid_item(MEDIATYPE_MPEG2_PES            ),
    define_guid_item(MEDIATYPE_MPEG2_SECTIONS       ),
    define_guid_item(MEDIATYPE_Midi                 ),
    define_guid_item(MEDIATYPE_ScriptCommand        ),
    define_guid_item(MEDIATYPE_Stream               ),
    define_guid_item(MEDIATYPE_Text                 ),
    define_guid_item(MEDIATYPE_Timecode             ),
    define_guid_item(MEDIATYPE_URL_STREAM           ),
    define_guid_item(MEDIATYPE_Video                ),
    define_guid_item(WMMEDIASUBTYPE_ACELPnet        ),
    define_guid_item(WMMEDIASUBTYPE_MP3             ),
    define_guid_item(WMMEDIASUBTYPE_WMAudioV8       ),
    define_guid_item(WMMEDIASUBTYPE_WMAudioV9       ),
    define_guid_item(WMMEDIASUBTYPE_WMAudio_Lossless),
    define_guid_item(WMMEDIASUBTYPE_WMSP1           ),
    define_guid_item(WMMEDIASUBTYPE_WMV1            ),
    define_guid_item(WMMEDIASUBTYPE_WMV2            ),
    define_guid_item(WMMEDIASUBTYPE_WMV3            ),
    define_guid_item(WMMEDIASUBTYPE_WMVA            ),
    define_guid_item(WMMEDIASUBTYPE_WMVP            ),
    define_guid_item(WMMEDIASUBTYPE_WVC1            ),
    define_guid_item(WMMEDIASUBTYPE_WVP2            )
};

void print(GUID guid)
{
    for(SIZE_T i = 0; i < sizeof(guid_items) / sizeof(guid_items[0]); i++)
    {
        if(guid_items[i].guid == guid)
        {
            PRINTF("%s", guid_items[i].name);
            return;
        }
    }
    PRINTF("unknown (%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}
