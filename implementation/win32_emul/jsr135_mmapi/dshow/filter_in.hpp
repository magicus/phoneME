#pragma once

#include <dshow.h>

class filter_in : public IBaseFilter
{
public:
    virtual bool data(SIZE_T size, BYTE *p) = 0;
    static bool create(wchar_t const *format, filter_in **ppfilter);
};
