#pragma once

#include <strmif.h>

void print(char const *fmt, ...);
void print(WCHAR const *fmt, ...);
void error(HRESULT hr);
void error(char const *str, HRESULT hr);
void print(GUID guid);
void dump_media_type(AM_MEDIA_TYPE const *pamt);
BOOL dump_media_types(IPin *pp, UINT32 indent);
BOOL dump_pin(IPin *pp, UINT32 indent);
BOOL dump_pins(IBaseFilter *pbf, UINT32 indent);
BOOL dump_filter(IBaseFilter *pbf, UINT32 indent);
BOOL dump_filters(IFilterGraph *pfg, UINT32 indent);
BOOL dump_filter_graph(IFilterGraph *pfg, UINT32 indent);
