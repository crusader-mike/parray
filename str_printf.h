#ifndef STR_PRINTF_H_2016_09_08_21_30_29_448_H_
#define STR_PRINTF_H_2016_09_08_21_30_29_448_H_


#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <climits>
#include "misc_macro.h"


//------------------------------------------------------------------------------
namespace common {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
using std::size_t;


////////////////////////////////////////////////////////////////////////////////////
// IMPORTANT NOTE (for MSVC)!
// According to latest investigations of _vsnprintf code it could have ONLY TWO outcomes:
// 1. successfull:
//      buffer was large enough to accomodate result string and terminating zero
//      returned value is the length of result string (not counting zero)
// 2. almost successfull:
//      buffer was not large enough to accomodate result string and terminating zero
//      buffer is used completely
//      in MBCS last character could be written "partially"
//      buffer is NOT zero terminated
//      returned value is -1

////////////////////////////////////////////////////////////////////////////////////
// str_* functions family
// These are SAFE wrappers around _vsnprintf() function. That means:
//      - result string is ALWAYS null-terminated
//      - result string is NOT always valid (i.e. could end with lead byte
//        or first byte of surrogate pair); see comments in 'str_vprintf'
//      - buffer size determined automatically whenever possible
//      - every function returns length of result string (i.e. NOT counting zero)
//
// As result you could (almost!) safely use this function for something like this:
//      char buf[1024];
//      char* pCur = buf;
//      pCur += str_printf(pCur, sizeof(buf) - (pCur - buf), ...);
//      pCur += str_printf(pCur, sizeof(buf) - (pCur - buf), ...);
//      pCur += str_printf(pCur, sizeof(buf) - (pCur - buf), ...);
//
// It has one drawback -- it is impossible to determine if buffer was big enough
// to accomodate whole string or truncation happens. Sometimes it is necessary to
// know if buffer was large enough (i.e. in CString::Format(...)) -- in this case
// you will need to use:
//      _vsnprintf
//      _snprintf


//------------------------------------------------------------------------------
// va_list + size [closure]
inline size_t str_vprintf(char* buf, size_t size, char const* pszFormat, va_list args) noexcept
{
    // no zero-sized buffers allowed (can't give zero-termination guarantee)

#if defined (__GNUC__)
    int nRes = vsnprintf(buf, size, pszFormat, args);
    if (nRes < 0)
    {
        // From the manual:
        // The snprintf() function shall fail if:
        //  EOVERFLOW - The value of n is greater than {INT_MAX} or the number
        //              of bytes needed to hold the output excluding the terminating
        //              null is greater than {INT_MAX}.
        buf[0] = '\0';
        return 0;
    }
    else if (static_cast<size_t>(nRes) > size - 1)
    {
        // buffer was too small
        return size - 1;
    }
    else // if (static_cast<size_t>(nRes) < size)
    {
        return static_cast<size_t>(nRes);
    }
#elif defined (_MSC_VER)
    assert(size != 0);

#pragma warning (suppress:4996)
    int nRes = _vsnprintf(buf, size, pszFormat, args);
    // IF size in (0, INT_MAX + 1] THEN nRes = { -1 or in [0, IN_MAX]}
    // IF size > INT_MAX + 1 THEN nRes = -1 because _vsnprintf exits immediately with:
    //      IF size = INT_MAX + 2 THEN buf[0] == 0
    //      IF size > INT_MAX + 2 THEN buf is not changed at all
    //
    // Morals:
    //        - never pass size > INT_MAX + 1
    //        - we could use (nRes < 0) instead of (nRes == -1) since it is potentially faster
    //
    assert(size - 1 <= INT_MAX);

    if (nRes < 0)
    {
#   ifdef _MBCS
        // here I wanted to implement 'consistency' guarantee, but in the end I decided
        // not to do it because:
        //      - '_mbsdec()' will not work with encoding schemes like Big5 where 
        //        lead and trail byte ranges overlap (see CRT sources) and wikipedia
        //      - '_mbsdec()' does not work if you pass a pointer into the middle
        //        of MBCS character
        //      - UTF-16 has surrogate pairs (two 16bit words that represents one symbol)
        //      - even with direct encodings such as UTF-32 it is not a good idea to split
        //        strings in random position, because of some features (like diacritic marks)
        //
        // So I left it as it is. Notes:
        //      - this code is used in places where correct representation is not 
        //        necessary (e.g. logging) and nothrow guarantee is more important
        //      - in other cases specialized 'no-loss' mechanisms (like in CString::Format) 
        //        or complex string analysis should be used to make generated string
        //        safe to use (e.g. concatenate with others)
        //      - even if you use 'unsafe' strings usually decoder can 'resync' after
        //        a few characters (immediately for UTF-16) -- for human it will look
        //        like a 'gap' in the text with (hopefully ;)) obviously wrong characters
#   endif
        buf[size - 1] = 0;
        return size - 1;
    }

    return static_cast<unsigned int>(nRes);
#else
#   error Please provide proper implementation for your compiler!
#endif
}


//------------------------------------------------------------------------------
// va_list + deduction
template<size_t size>
inline size_t str_vprintf(char (&buf)[size], char const* pszFormat, va_list args) noexcept
{
    return str_vprintf(buf, size, pszFormat, args);
}


//------------------------------------------------------------------------------
// [...] + deduction
template<size_t size>
inline size_t str_printf(char (&buf)[size], char const* pszFormat, ...) noexcept
{
    va_list args;
    va_start(args, pszFormat);

    size_t nRes = str_vprintf(buf, size, pszFormat, args);
    va_end(args);

    return nRes;
}


//------------------------------------------------------------------------------
// [...] + size
inline size_t str_printf(char* buf, size_t size, char const* pszFormat, ...) noexcept
{
    va_list args;
    va_start(args, pszFormat);

    size_t nRes = str_vprintf(buf, size, pszFormat, args);

    va_end(args);

    return nRes;
}


//------------------------------------------------------------------------------
} //namespace common {
//------------------------------------------------------------------------------


#endif //STR_PRINTF_H_2016_09_08_21_30_29_448_H_
