/*/////////////////////////////////////////////////////////////////////////////
    ADV library

  Author:
    Michael Kilburn

/////////////////////////////////////////////////////////////////////////////*/


#ifndef PARRAY_TOOLS_H_2016_09_05_23_21_06_660_H_
#define PARRAY_TOOLS_H_2016_09_05_23_21_06_660_H_


#include "parray.h"
#include <type_traits>
#include <cctype>
#include <cwctype>
#include <string>
#include <cstring>
#include <algorithm>
#include <deque>
#include <iterator>
#include <limits>


//------------------------------------------------------------------------------
// parray<> tools
//
//  parray trim[_left|_right](parray v)
//      trim whitespaces
//
//  bools starts_with(parray v1, parray v2)
//      true if v1 starts with v2
//  bools ends_with(parray v1, parray v2)
//      true if v1 ends with v2
//  T* contains(parray v1, parray v2)
//      returns pointer to v2's occurence inside of v1 (not necessarily the first one) or nullptr if v2 is not in v1
//
// split/join functions
//
// Notes:
//  - [r] stands for 'reverse' -- i.e. rsplit will process array in reverse:
//      rsplit("1,2,3", ',') --> ["3","2","1"]
//    similarly for rjoin:
//      rjoin(["3","2","1"], ',') --> "1,2,3"
//  - [_se] stands for 'skip empty':
//      split_se("1,,2,3", ',') --> ["1","2","3"]
//      join_se(["1","","3"], ',') --> "1,3"
//  - split delimiter can be:
//      single value
//      parray of values
//      functor with certain interface (see bitset_delim for example)
//  - join delimiter can be:
//      single value
//      parray of values
//  - join result type should have following member functions (fine for vector/basic_string/etc):
//      reserve(size_t)
//      I end()
//      insert(I pos, I it1, I it2)
//
//  T* [r]split[_se](parray v, D delim, F f)
//      split v into subarrays using delimiter and pass them into functor f until it returns false
//      return pointer to an element where we stopped or nullptr (if all data was processed)
//  deque<parray> [r]split[_se](parray v, D delim)
//      split v using delimiter and return deque of subarrays
//  size_t [r]split[_se](parray v, D delim, parray* buf, size_t buf_sz)
//  size_t [r]split[_se](parray v, D delim, parray (&buf)[n])
//      split v using delimiter and place resulting subarrays into provided buffer; if buffer is not big enough -- last element
//      will contain unprocessed remainder of v; return number of buffer elements used
//      pre-condition: buffer size > 1
//
//  R [r]join[_se](I it, I it_end, D delim)
//      join arrays specified by [it, it_end) range into R using delimiter
//  void [r]join[_se](I it, I it_end, D delim, F f)
//      the same but instead of building value of type R -- call f(v) for every value we would otherwise add to R
//
//
// Example 1:
//  remove second substring from the back with only one memory allocation
//
//    rcstring data = ...;          //  something like "AAA_..._BBB_CCCC"
//
//    rcstring parts[3];
//    size_t count = rsplit(data, '_', parts);
//          // parts[0] == "CCCC"
//          // parts[1] == "BBB"
//          // parts[2] == "AAA_..."
//    if (count == 3 && parts[0] == ntba("CCCC") && parts[1] == ntba("BBB"))
//    {
//        parts[1].len = 0;                                      // make parts[1] empty
//        return rjoin_se<string>(parts, parts + count, '_');    // AAA_..._CCCC
//    }
//    else
//        ...;  // unexpected data
//
//
// Example 2:
//  lets say we are supposed to receive exactly three values separated by \r\n\t,:; or space
//
//    bitset_delim<> delim{ ntba("\r\n\t,:; ") };   // constructing bitset_delim is not trivial, if possible do it beforehand
//
//    rcstring data = ...;              // " A\r:B;,\t C\n"
//    rcstring parts[4];
//    if (split_se(data, delim, parts) == 3)
//    {
//          // parts[0] == "A"
//          // parts[1] == "B"
//          // parts[2] == "C"
//    }
//    else
//        ...;      // bad data
//
//
// Example 3:
//  split and process string of any length without any memory allocations
//
//    rcstring data = ntba("A_B_C_...._X_Y_Z");
//    for(;;)
//    {
//        rcstring parts[3];
//        size_t count = split(data, '_', parts);
//
//        for(size_t i = 0, t = (count != 3 ? count : 2); i < t; ++i)
//          cout << parts[i] << ntba("\n");
//
//        if (count != 3) break;    // no remainder --> we processed everything
//
//        data = parts[2];          // process the rest
//    }
//


//------------------------------------------------------------------------------
namespace adv { namespace parray_tools_pvt_ {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
using std::size_t;
using adv::parray;
using std::deque;
using std::basic_string;
using std::find;
using std::find_if;
using std::find_first_of;
using std::make_reverse_iterator;
using std::numeric_limits;

template<class T> using remove_cv = std::remove_cv_t<T>;
template<bool B, class T = void> using enable_if = std::enable_if_t<B, T>;
template<class T, class U> constexpr bool is_same = std::is_same<T, U>::value;
template<class F, class T> constexpr bool is_convertible = std::is_convertible<F, T>::value;
template<class T> constexpr bool is_scalar = std::is_scalar<T>::value;

// relaxed version of 'is_same<remove_cv<E>, remove_cv<T>> && is_convertible<E*, T*>'
template<class E, class T> constexpr bool is_almost_same = (sizeof(E) == sizeof(T)) && is_convertible<E*, T*>;


//------------------------------------------------------------------------------
template<class T, bool = is_scalar<T>> struct ArgType { using type = remove_cv<T> const&; };
template<class T> struct ArgType<T, true> { using type = remove_cv<T> const; };
template<class T> using argtype = typename ArgType<T>::type;


//------------------------------------------------------------------------------
template<class T, enable_if<is_same<remove_cv<T>, char   >>...> inline bool isspace(T v) { return std::isspace (v) != 0; }
template<class T, enable_if<is_same<remove_cv<T>, wchar_t>>...> inline bool isspace(T v) { return std::iswspace(v) != 0; }


//------------------------------------------------------------------------------
template<class T, class Tr, enable_if<is_same<remove_cv<T>, char> || is_same<remove_cv<T>, wchar_t>>...>
inline parray<T, Tr> trim(parray<T, Tr> v)
{
    T* p_end = v.p + v.len;
    while(v.p != p_end && isspace(*v.p)) ++v.p;     // trim left

    if (v.p != p_end)                               // trim right
        while( isspace(p_end[-1]) ) --p_end;        // v.p points to non-whitespace char (no need for v.p != p_end check)

    v.len = p_end - v.p;
    return v;
}


//------------------------------------------------------------------------------
template<class T, class Tr, enable_if<is_same<remove_cv<T>, char> || is_same<remove_cv<T>, wchar_t>>...>
inline parray<T, Tr> trim_left(parray<T, Tr> v)
{
    T* p_end = v.p + v.len;
    while(v.p != p_end && isspace(*v.p)) ++v.p;

    v.len = p_end - v.p;
    return v;
}


//------------------------------------------------------------------------------
template<class T, class Tr, enable_if<is_same<remove_cv<T>, char> || is_same<remove_cv<T>, wchar_t>>...>
inline parray<T, Tr> trim_right(parray<T, Tr> v)
{
    T* p_end = v.p + v.len;
    while(p_end != v.p && isspace(p_end[-1])) --p_end;

    v.len = p_end - v.p;
    return v;
}


//------------------------------------------------------------------------------
// Notes:
//  - in comparison below array lengths are guaranteed to be equal, maybe we should allow different 
//    traits? But if we do -- which one to use?
//
template<class T, class E, class Tr>
inline bool starts_with(parray<T, Tr> v1, parray<E, Tr> v2)
{
    return v1.len >= v2.len && parray<T, Tr>(v2.len, v1.p) == v2;
}


//------------------------------------------------------------------------------
template<class T, class E, class Tr>
inline bool ends_with(parray<T, Tr> v1, parray<E, Tr> v2)
{
    return v1.len >= v2.len && parray<T, Tr>(v2.len, v1.p + v1.len - v2.len) == v2;
}


//------------------------------------------------------------------------------
// return nullptr if v1 does not contain v2
// otherwise -- T* that points to some occurence of v2 within v1 (not necessarily first one)
template<class T, class E, class Tr, enable_if<is_almost_same<T, E> || is_almost_same<E, T>>...>
inline T* contains(parray<T, Tr> v1, parray<E, Tr> v2)
{
    if (v1.len < v2.len) return nullptr;
    if (v2.len == 0) return v1.p;                                   // any array contains an empty one

    // check if v2 is literally inside of v1
    if (v1.p <= v2.p && (v2.p + v2.len) <= (v1.p + v1.len))         // assuming memory model is linear :-)
        return v1.p + (v2.p - v1.p);

    T* p = std::search(v1.p, v1.p + v1.len, v2.p, v2.p + v2.len);   // ok, do it hard way

    return (p != v1.p + v1.len) ? p : nullptr;
}


//------------------------------------------------------------------------------
// little helper (to tell apart between D delimiter and single-value T delimiter)
//
template<class T> struct IsDelimiter
{
	typedef char Yes[1];
	typedef char No [2];

	template <int> struct Check;

    template <class C> static Yes& Test(Check<C::is_delimiter>*);
	template <class>   static No & Test(...);

    enum { value = (sizeof(Test<T>(nullptr)) == sizeof(Yes)) };
};

template<class T> constexpr bool is_delimiter = IsDelimiter<T>::value;


//------------------------------------------------------------------------------
// Common delimiter classes
//------------------------------------------------------------------------------

template<class T>
struct single_delim
{
    enum { is_delimiter };      // mark this type for 'delim is: D' case

    argtype<T> delim;           // single value

    template<class I> inline I find_first(I p, I p_end) const { return find(p, p_end, delim); }
    template<class I> inline I skip_all  (I p, I p_end) const { return find_if(p, p_end, [this](auto const& v) { return v != delim; }); }

    // pre-condition: p was produced by 'find_first()'
    template<class I> inline I skip_one  (I p) const { return ++p; }
};

template<class T>
struct multi_delim
{
    enum { is_delimiter };      // mark this type for 'delim is: D' case

    parray<T> delim;            // multiple values (any of them)

    template<class I> inline I find_first(I p, I p_end) const { return find_first_of(p, p_end, delim.p, delim.p + delim.len); }
    template<class I> inline I skip_all  (I p, I p_end) const { return find_if(p, p_end, [this, d_end = delim.p + delim.len](auto const& v) { return find(delim.p, d_end, v) == d_end; }); }

    // pre-condition: p was produced by 'find_first()' and != p_end
    template<class I> inline I skip_one  (I p) const { return ++p; }
};

// bitmap lookup delimiter, ignores values outside of [minV, maxV] range
template<class IntT = unsigned char, IntT minV = numeric_limits<IntT>::min(), IntT maxV = numeric_limits<IntT>::max()>
class bitset_delim
{
    static_assert(maxV - minV > 0, "");     // max idx value must be positive IntT

    enum {  char_bits   = numeric_limits<unsigned char>::digits,    // aka CHAR_BIT
            chunk_bits  = sizeof(unsigned long)*char_bits,
            chunk_count = ((maxV - minV + 1) + chunk_bits - 1)/chunk_bits,
    };

    unsigned long data[chunk_count];

    unsigned idx_(unsigned i) const { return i / chunk_bits; }
    unsigned long mask_(unsigned i) const { return 1ul << (i % chunk_bits); }

    void set_(unsigned i) { data[idx_(i)] |=  mask_(i); }
    void clr_(unsigned i) { data[idx_(i)] &= ~mask_(i); }
    bool get_(unsigned i) const { return ( data[idx_(i)] & mask_(i) ) ? true : false; }

public:
    bitset_delim() { clear_all(); }

    template<class T, class Tr>
    bitset_delim(parray<T, Tr> v) : bitset_delim()
    {
        for(size_t i = 0; i < v.len; ++i)
            set_bit(v[i]);
    }

    void clear_all() { memset(data,  0, sizeof(data)); }
    void set_all  () { memset(data, ~0, sizeof(data)); }

    template<class T>
    void set_bit(T const& v)
    {
        IntT tmp = v;
        if (minV <= tmp && tmp <= maxV) set_(tmp - minV);
    }

    template<class T>
    void clear_bit(T const& v)
    {
        IntT tmp = v;
        if (minV <= tmp && tmp <= maxV) clr_(tmp - minV);
    }

    template<class T>
    bool is_set(T const& v) const
    {
        IntT tmp = v;
        return (minV <= tmp && tmp <= maxV) ? get_(tmp - minV) : false;
    }

    // delimiter implementation

    enum { is_delimiter };      // mark this type for 'delim is: D' case

    template<class I> inline I find_first(I p, I p_end) const { return find_if(p, p_end, [this](auto const& v) { return  this->is_set(v); }); }
    template<class I> inline I skip_all  (I p, I p_end) const { return find_if(p, p_end, [this](auto const& v) { return !this->is_set(v); }); }

    // pre-condition: p was produced by 'find_first()' and != p_end
    template<class I> inline I skip_one  (I p) const { return ++p; }
};


//------------------------------------------------------------------------------
// split[_se]_() -- (internal) generic functions to split range
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// split range using delimiter and call f() for each subrange until f returns true
// returns pointer to element where it stopped or I() if nothing was left
template<class I, class D, class F>
inline I split_(I it, I it_end, D const& delim, F f)
{
    for(;;)
    {
        I p = delim.find_first(it, it_end);

        bool stop = f(it, p);

        if (p == it_end) return I();            // last chunk was processed

        p = delim.skip_one(p);                  // skip delimiter

        if (stop) return p;

        it = p;
    }
}


//------------------------------------------------------------------------------
// split parray using delimiter and call f() for each non-empty subarray until f returns true
// returns pointer to element where it stopped or I() if nothing was left
template<class I, class D, class F>
inline I split_se_(I it, I it_end, D const& delim, F f)
{
    for(bool stop = false; ; )
    {
        it = delim.skip_all(it, it_end);                // skip leading delimiters
        if (it == it_end) return I();

        if (stop) return it;

        I p = delim.find_first(it + 1, it_end);         // we know *it is not delim

        stop = f(it, p);

        it = p;
    }
}


//------------------------------------------------------------------------------
// split() functions family
//
// Possible aspects:
//  result ->: F, deque, (parray<T>*, sz), parray<T>[n]
//  delim is: D, T, parray<T>
//  form: [r]split[_se]
//


//------------------------------------------------------------------------------
//  delim is: D
//  form: split
template<class T, class Tr, class D, class F, enable_if<is_delimiter<D>>...>
inline T* split(parray<T, Tr> v, D const& delim, F f)
{
    return split_(v.p, v.p + v.len, delim, [&f](auto it, auto it_end) { return f(parray<T, Tr>(it_end - it, it)); });
}

template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
deque<parray<T, Tr>> split(parray<T, Tr> v, D const& delim)
{
    deque<parray<T, Tr>> res;
    split(v, delim, [&res](auto v){ res.push_back(v); return false; });
    return res;
}

// pre-condition: buf_sz > 1
template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
size_t split(parray<T, Tr> v, D const& delim, parray<T, Tr>* buf, size_t buf_sz)
{
    size_t i = 0, count = buf_sz - 1;
    T* p = split(v, delim, [buf, count, &i](auto v) { buf[i++] = v; return i == count; });

    if (p)
        buf[i++] = parray<T, Tr>(v.len - (p - v.p), p);     // remainder

    return i;
}

template<class T, class Tr, class D, size_t n, enable_if<is_delimiter<D>>...>
inline size_t split(parray<T, Tr> v, D const& delim, parray<T, Tr> (&buf)[n])
{
    static_assert(n > 1, "buf size should be > 1");
    return split(v, delim, &buf[0], n);
}


//------------------------------------------------------------------------------
//  delim is: D
//  form: split_se
template<class T, class Tr, class D, class F, enable_if<is_delimiter<D>>...>
inline T* split_se(parray<T, Tr> v, D const& delim, F f)
{
    return split_se_(v.p, v.p + v.len, delim, [&f](auto it, auto it_end) { return f(parray<T, Tr>(it_end - it, it)); });
}

template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
deque<parray<T, Tr>> split_se(parray<T, Tr> v, D const& delim)
{
    deque<parray<T, Tr>> res;
    split_se(v, delim, [&res](auto v){ res.push_back(v); return false; });
    return res;
}

// pre-condition: buf_sz > 1
template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
size_t split_se(parray<T, Tr> v, D const& delim, parray<T, Tr>* buf, size_t buf_sz)
{
    size_t i = 0, count = buf_sz - 1;
    T* p = split_se(v, delim, [buf, count, &i](auto v) { buf[i++] = v; return i == count; });

    if (p)
        buf[i++] = parray<T, Tr>(v.len - (p - v.p), p);     // remainder

    return i;
}

template<class T, class Tr, class D, size_t n, enable_if<is_delimiter<D>>...>
inline size_t split_se(parray<T, Tr> v, D const& delim, parray<T, Tr> (&buf)[n])
{
    static_assert(n > 1, "buf size should be > 1");
    return split_se(v, delim, &buf[0], n);
}


//------------------------------------------------------------------------------
//  delim is: D
//  form: rsplit
template<class T, class Tr, class D, class F, enable_if<is_delimiter<D>>...>
T* rsplit(parray<T, Tr> v, D const& delim, F f)
{
    return split_(make_reverse_iterator(v.p + v.len), make_reverse_iterator(v.p), delim, [&f](auto it, auto it_end) { return f(parray<T, Tr>(it_end - it, it_end.base())); }).base();
}

template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
deque<parray<T, Tr>> rsplit(parray<T, Tr> v, D const& delim)
{
    deque<parray<T, Tr>> res;
    rsplit(v, delim, [&res](auto v){ res.push_back(v); return false; });
    return res;
}

// pre-condition: buf_sz > 1
template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
size_t rsplit(parray<T, Tr> v, D const& delim, parray<T, Tr>* buf, size_t buf_sz)
{
    size_t i = 0, count = buf_sz - 1;
    T* p = rsplit(v, delim, [buf, count, &i](auto v) { buf[i++] = v; return i == count; });

    if (p)
        buf[i++] = parray<T, Tr>(p - v.p, v.p);     // remainder

    return i;
}

template<class T, class Tr, class D, size_t n, enable_if<is_delimiter<D>>...>
inline size_t rsplit(parray<T, Tr> v, D const& delim, parray<T, Tr> (&buf)[n])
{
    static_assert(n > 1, "buf size should be > 1");
    return rsplit(v, delim, &buf[0], n);
}


//------------------------------------------------------------------------------
//  delim is: D
//  form: rsplit_se
template<class T, class Tr, class D, class F, enable_if<is_delimiter<D>>...>
T* rsplit_se(parray<T, Tr> v, D const& delim, F f)
{
    return split_se_(make_reverse_iterator(v.p + v.len), make_reverse_iterator(v.p), delim, [&f](auto it, auto it_end) { return f(parray<T, Tr>(it_end - it, it_end.base())); }).base();
}

template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
deque<parray<T, Tr>> rsplit_se(parray<T, Tr> v, D const& delim)
{
    deque<parray<T, Tr>> res;
    rsplit_se(v, delim, [&res](auto v){ res.push_back(v); return false; });
    return res;
}

// pre-condition: buf_sz > 1
template<class T, class Tr, class D, enable_if<is_delimiter<D>>...>
size_t rsplit_se(parray<T, Tr> v, D const& delim, parray<T, Tr>* buf, size_t buf_sz)
{
    size_t i = 0, count = buf_sz - 1;
    T* p = rsplit_se(v, delim, [buf, count, &i](auto v) { buf[i++] = v; return i == count; });

    if (p)
        buf[i++] = parray<T, Tr>(p - v.p, v.p);     // remainder

    return i;
}

template<class T, class Tr, class D, size_t n, enable_if<is_delimiter<D>>...>
inline size_t rsplit_se(parray<T, Tr> v, D const& delim, parray<T, Tr> (&buf)[n])
{
    static_assert(n > 1, "buf size should be > 1");
    return rsplit_se(v, delim, &buf[0], n);
}


//------------------------------------------------------------------------------
//  delim is: T
//  form: split
template<class T, class Tr, class F, enable_if<!is_delimiter<T>>...>
T* split(parray<T, Tr> v, argtype<T> delim, F f) { return split(v, single_delim<T>{delim}, f); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
deque<parray<T, Tr>> split(parray<T, Tr> v, argtype<T> delim) { return split(v, single_delim<T>{delim}); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
size_t split(parray<T, Tr> v, argtype<T> delim, parray<T, Tr>* buf, size_t buf_sz) { return split(v, single_delim<T>{delim}, buf, buf_sz); }

template<class T, class Tr, size_t n, enable_if<!is_delimiter<T>>...>
inline size_t split(parray<T, Tr> v, argtype<T> delim, parray<T, Tr> (&buf)[n]) { return split(v, single_delim<T>{delim}, buf); }


//------------------------------------------------------------------------------
//  delim is: parray<T>
//  form: split
template<class T, class Tr, class E, class Tr2, class F>
T* split(parray<T, Tr> v, parray<E, Tr2> delim, F f) { return split(v, multi_delim<E>{ parray<E>(delim) }, f); }

template<class T, class Tr, class E, class Tr2>
deque<parray<T, Tr>> split(parray<T, Tr> v, parray<E, Tr2> delim) { return split(v, multi_delim<E>{ parray<E>(delim) }); }

template<class T, class Tr, class E, class Tr2>
size_t split(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr>* buf, size_t buf_sz) { return split(v, multi_delim<E>{ parray<E>(delim) }, buf, buf_sz); }

template<class T, class Tr, class E, class Tr2, size_t n>
inline size_t split(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr> (&buf)[n]) { return split(v, multi_delim<E>{ parray<E>(delim) }, buf); }


//------------------------------------------------------------------------------
//  delim is: T
//  form: split_se
template<class T, class Tr, class F, enable_if<!is_delimiter<T>>...>
T* split_se(parray<T, Tr> v, argtype<T> delim, F f) { return split_se(v, single_delim<T>{delim}, f); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
deque<parray<T, Tr>> split_se(parray<T, Tr> v, argtype<T> delim) { return split_se(v, single_delim<T>{delim}); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
size_t split_se(parray<T, Tr> v, argtype<T> delim, parray<T, Tr>* buf, size_t buf_sz) { return split_se(v, single_delim<T>{delim}, buf, buf_sz); }

template<class T, class Tr, size_t n, enable_if<!is_delimiter<T>>...>
inline size_t split_se(parray<T, Tr> v, argtype<T> delim, parray<T, Tr> (&buf)[n]) { return split_se(v, single_delim<T>{delim}, buf); }


//------------------------------------------------------------------------------
//  delim is: parray<T>
//  form: split_se
template<class T, class Tr, class E, class Tr2, class F>
T* split_se(parray<T, Tr> v, parray<E, Tr2> delim, F f) { return split_se(v, multi_delim<E>{ parray<E>(delim) }, f); }

template<class T, class Tr, class E, class Tr2>
deque<parray<T, Tr>> split_se(parray<T, Tr> v, parray<E, Tr2> delim) { return split_se(v, multi_delim<E>{ parray<E>(delim) }); }

template<class T, class Tr, class E, class Tr2>
size_t split_se(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr>* buf, size_t buf_sz) { return split_se(v, multi_delim<E>{ parray<E>(delim) }, buf, buf_sz); }

template<class T, class Tr, class E, class Tr2, size_t n>
inline size_t split_se(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr> (&buf)[n]) { return split_se(v, multi_delim<E>{ parray<E>(delim) }, buf); }


//------------------------------------------------------------------------------
//  delim is: T
//  form: rsplit
template<class T, class Tr, class F, enable_if<!is_delimiter<T>>...>
T* rsplit(parray<T, Tr> v, argtype<T> delim, F f) { return rsplit(v, single_delim<T>{delim}, f); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
deque<parray<T, Tr>> rsplit(parray<T, Tr> v, argtype<T> delim) { return rsplit(v, single_delim<T>{delim}); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
size_t rsplit(parray<T, Tr> v, argtype<T> delim, parray<T, Tr>* buf, size_t buf_sz) { return rsplit(v, single_delim<T>{delim}, buf, buf_sz); }

template<class T, class Tr, size_t n, enable_if<!is_delimiter<T>>...>
inline size_t rsplit(parray<T, Tr> v, argtype<T> delim, parray<T, Tr> (&buf)[n]) { return rsplit(v, single_delim<T>{delim}, buf); }


//------------------------------------------------------------------------------
//  delim is: parray<T>
//  form: rsplit
template<class T, class Tr, class E, class Tr2, class F>
T* rsplit(parray<T, Tr> v, parray<E, Tr2> delim, F f) { return rsplit(v, multi_delim<E>{ parray<E>(delim) }, f); }

template<class T, class Tr, class E, class Tr2>
deque<parray<T, Tr>> rsplit(parray<T, Tr> v, parray<E, Tr2> delim) { return rsplit(v, multi_delim<E>{ parray<E>(delim) }); }

template<class T, class Tr, class E, class Tr2>
size_t rsplit(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr>* buf, size_t buf_sz) { return rsplit(v, multi_delim<E>{ parray<E>(delim) }, buf, buf_sz); }

template<class T, class Tr, class E, class Tr2, size_t n>
inline size_t rsplit(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr> (&buf)[n]) { return rsplit(v, multi_delim<E>{ parray<E>(delim) }, buf); }


//------------------------------------------------------------------------------
//  delim is: T
//  form: rsplit_se
template<class T, class Tr, class F, enable_if<!is_delimiter<T>>...>
T* rsplit_se(parray<T, Tr> v, argtype<T> delim, F f) { return rsplit_se(v, single_delim<T>{delim}, f); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
deque<parray<T, Tr>> rsplit_se(parray<T, Tr> v, argtype<T> delim) { return rsplit_se(v, single_delim<T>{delim}); }

template<class T, class Tr, enable_if<!is_delimiter<T>>...>
size_t rsplit_se(parray<T, Tr> v, argtype<T> delim, parray<T, Tr>* buf, size_t buf_sz) { return rsplit_se(v, single_delim<T>{delim}, buf, buf_sz); }

template<class T, class Tr, size_t n, enable_if<!is_delimiter<T>>...>
inline size_t rsplit_se(parray<T, Tr> v, argtype<T> delim, parray<T, Tr> (&buf)[n]) { return rsplit_se(v, single_delim<T>{delim}, buf); }


//------------------------------------------------------------------------------
//  delim is: parray<T>
//  form: rsplit_se
template<class T, class Tr, class E, class Tr2, class F>
T* rsplit_se(parray<T, Tr> v, parray<E, Tr2> delim, F f) { return rsplit_se(v, multi_delim<E>{ parray<E>(delim) }, f); }

template<class T, class Tr, class E, class Tr2>
deque<parray<T, Tr>> rsplit_se(parray<T, Tr> v, parray<E, Tr2> delim) { return rsplit_se(v, multi_delim<E>{ parray<E>(delim) }); }

template<class T, class Tr, class E, class Tr2>
size_t rsplit_se(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr>* buf, size_t buf_sz) { return rsplit_se(v, multi_delim<E>{ parray<E>(delim) }, buf, buf_sz); }

template<class T, class Tr, class E, class Tr2, size_t n>
inline size_t rsplit_se(parray<T, Tr> v, parray<E, Tr2> delim, parray<T, Tr> (&buf)[n]) { return rsplit_se(v, multi_delim<E>{ parray<E>(delim) }, buf); }


//------------------------------------------------------------------------------
// join[_se]_() -- (internal) generic join functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template<class I, class T, class Tr, class F>
inline void join_(I it, I it_end, parray<T, Tr> delim, F f)
{
    if (it != it_end)
    {
        f(*it);
        for(++it; it != it_end; ++it)
        {
            f(delim);
            f(*it);
        }
    }
}

template<class I>
inline size_t total_len_(I it, I it_end, size_t delim)
{
    size_t len = 0;
    if (it != it_end)
    {
        len = it->len;
        for(++it; it != it_end; ++it) len += delim + it->len;
    }
    return len;
}

template<class R, class I, class T, class Tr>
inline R join_(I it, I it_end, parray<T, Tr> delim)
{
    R r;
    r.reserve(total_len_(it, it_end, delim.len));
    join_(it, it_end, delim, [&r](auto v){ r.insert(end(r), v.p, v.p + v.len); } );
    return r;
}


//------------------------------------------------------------------------------
template<class I, class T, class Tr, class F>
inline void join_se_(I it, I it_end, parray<T, Tr> delim, F f)
{
    for(; it != it_end; ++it)
    {
        if (!it->empty())        // first non-empty value
        {
            f(*it);
            for(++it; it != it_end; ++it)
            {
                if(!it->empty())
                {
                    f(delim);
                    f(*it);
                }
            }
            break;  // no need for one more 'it != it_end' check
        }
    }
}

template<class I>
inline size_t total_len_se_(I it, I it_end, size_t delim)
{
    size_t len = 0;
    for(; it != it_end; ++it)
    {
        if (!it->empty())        // first non-empty value
        {
            len = it->len;
            for(++it; it != it_end; ++it)
            {
                if(!it->empty()) len += delim + it->len;
            }
            break;  // no need for one more 'it != it_end' check
        }
    }
    return len;
}

template<class R, class I, class T, class Tr>
inline R join_se_(I it, I it_end, parray<T, Tr> delim)
{
    R r;
    r.reserve(total_len_se_(it, it_end, delim.len));
    join_se_(it, it_end, delim, [&r](auto v){ r.insert(end(r), v.p, v.p + v.len); } );
    return r;
}


//------------------------------------------------------------------------------
// join() functions family
//
// Possible aspects:
//  input data: deque, (parray<T>*, sz)
//  delim is: T, parray<T>
//  form: [r]join[_se]
//
// Notes:
//  - return type should support 'insert(end, it1, it2)' and 'reserve(size_t)'
//


//------------------------------------------------------------------------------
//  input data: range
//  delim is: T, parray<T>
//  form: [r]join[_se]
template<class R, class I, class T>
R join(I it, I it_end, T const& delim) { return join_<R>(it, it_end, parray<T const>(1, &delim)); }

template<class R, class I, class T>
R join_se(I it, I it_end, T const& delim) { return join_se_<R>(it, it_end, parray<T const>(1, &delim)); }

template<class R, class I, class T>
R rjoin(I it, I it_end, T const& delim) { return join_<R>(make_reverse_iterator(it_end), make_reverse_iterator(it), parray<T const>(1, &delim)); }

template<class R, class I, class T>
R rjoin_se(I it, I it_end, T const& delim) { return join_se_<R>(make_reverse_iterator(it_end), make_reverse_iterator(it), parray<T const>(1, &delim)); }

template<class R, class I, class T, class Tr>
R join(I it, I it_end, parray<T, Tr> delim) { return join_<R>(it, it_end, delim); }

template<class R, class I, class T, class Tr>
R join_se(I it, I it_end, parray<T, Tr> delim) { return join_se_<R>(it, it_end, delim); }

template<class R, class I, class T, class Tr>
R rjoin(I it, I it_end, parray<T, Tr> delim) { return join_<R>(make_reverse_iterator(it_end), make_reverse_iterator(it), delim); }

template<class R, class I, class T, class Tr>
R rjoin_se(I it, I it_end, parray<T, Tr> delim) { return join_se_<R>(make_reverse_iterator(it_end), make_reverse_iterator(it), delim); }


//------------------------------------------------------------------------------
// expose generic form too -- it is too useful
//
template<class I, class T, class F>
void join(I it, I it_end, T const& delim, F f) { join_(it, it_end, parray<T const>(1, &delim), f); }

template<class I, class T, class F>
void join_se(I it, I it_end, T const& delim, F f) { join_se_(it, it_end, parray<T const>(1, &delim), f); }

template<class I, class T, class F>
void rjoin(I it, I it_end, T const& delim, F f) { join_(make_reverse_iterator(it_end), make_reverse_iterator(it), parray<T const>(1, &delim), f); }

template<class I, class T, class F>
void rjoin_se(I it, I it_end, T const& delim, F f) { join_se_(make_reverse_iterator(it_end), make_reverse_iterator(it), parray<T const>(1, &delim), f); }

template<class I, class T, class Tr, class F>
void join(I it, I it_end, parray<T, Tr> delim, F f) { join_(it, it_end, delim, f); }

template<class I, class T, class Tr, class F>
void join_se(I it, I it_end, parray<T, Tr> delim, F f) { join_se_(it, it_end, delim, f); }

template<class I, class T, class Tr, class F>
void rjoin(I it, I it_end, parray<T, Tr> delim, F f) { join_(make_reverse_iterator(it_end), make_reverse_iterator(it), delim, f); }

template<class I, class T, class Tr, class F>
void rjoin_se(I it, I it_end, parray<T, Tr> delim, F f) { join_se_(make_reverse_iterator(it_end), make_reverse_iterator(it), delim, f); }


//------------------------------------------------------------------------------
} // namespace parray_tools_pvt_
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
using parray_tools_pvt_::trim;
using parray_tools_pvt_::trim_left;
using parray_tools_pvt_::trim_right;
using parray_tools_pvt_::starts_with;
using parray_tools_pvt_::ends_with;
using parray_tools_pvt_::contains;
using parray_tools_pvt_::split;
using parray_tools_pvt_::split_se;
using parray_tools_pvt_::rsplit;
using parray_tools_pvt_::rsplit_se;
using parray_tools_pvt_::join;
using parray_tools_pvt_::join_se;
using parray_tools_pvt_::rjoin;
using parray_tools_pvt_::rjoin_se;
using parray_tools_pvt_::bitset_delim;


//------------------------------------------------------------------------------
} // namespace adv
//------------------------------------------------------------------------------


#endif //PARRAY_TOOLS_H_2016_09_05_23_21_06_660_H_

