#ifndef MISC_MACRO_H_11_55_40_07_11_2007_HDR_
#define MISC_MACRO_H_11_55_40_07_11_2007_HDR_


//------------------------------------------------------------------------------
#if defined (__GNUC__)
#	define PRINTF_ARGS(string_index, first_to_check) \
	 __attribute__ ((format (printf, string_index, first_to_check)))
#elif defined(_MSC_VER)
#	define PRINTF_ARGS(string_index, first_to_check)
#else
#   error Please adjust misc_macro.h for your compiler!
#endif


#endif //MISC_MACRO_H_11_55_40_07_11_2007_HDR_

