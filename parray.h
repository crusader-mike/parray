/*/////////////////////////////////////////////////////////////////////////////
    ADV library

  Author:
    Michael Kilburn

/////////////////////////////////////////////////////////////////////////////*/


#ifndef PARRAY_H_2016_08_30_04_03_32_135_H_
#define PARRAY_H_2016_08_30_04_03_32_135_H_


#include <cstddef>
#include <type_traits>
#include <cstring>
#include <string>
#include <vector>
#include <ostream>


//------------------------------------------------------------------------------
// parray<T, Traits>
//
//  Zero-weight class designed to represent a raw pointer to an array (i.e. it doesn't assume ownership), 
// it is essentially a (len, T*) pair with associated comparison semantics (implemented in Traits). Default
// comparisons are defined in such way that length takes precedence over elements -- i.e. shorter array is
// always 'less' than longer one. This allows for very efficient comparisons -- in general most comparisons
// don't even need to dereference T*.
//  For char types there are tools to deal with trailing nul character (see ntba() & ntbs()). And if T can 
// be compared with E then parray<T, Traits> can be compared with parray<E, Traits>.
// 
// Examples:
//
//      typedef adv::parray<char const> rcstring;
//      rcstring r1{ ntba("123") };         // len = 3
//
//      char const* p = "123";
//      rcstring r2{ ntbs(p) };             // len = strlen(p) == 3
//
//      assert( r1 == r2 );
//      assert( r1 == ntba("123") );        // will use length of "123" known at compile time
//      assert( r1 == ntbs(p) );            // will scan 'p' at most once
//      r2 == p;                            // won't compile, can't get array length from raw pointer
//      r2 == "123";                        // won't compile, for char array it is impossible to tell if it is nul-terminated or not
//
//      int n[] = {1,2,3};
//      parray<int> r3 = n;                 // len = 3
//      assert( r3 == n );                  // int is not char type -- no problems with nul-termination
//      assert( r3 < r1 );                  // r3.len == r1.len && 1 < '1'
//
//      rcstring r4 = rcstring::zero();     // default parray ctor leaves data undefined
//      assert( r4 < r1 );
//
//      std::string s("123");
//      assert( r1 <= s );
//      assert( s == r2 );
//
//      std::vector v{1,2,3};
//      assert( r3 == v );
//
// Notes:
//  - parray<T, Traits> and parray<E, Traits> are assumed comparable (they share same traits class); if T and E aren't comparable
//    compilation will fail
//  - default trait checks array length first (i.e. ntba("abc") < ntba("aaaaaaa"))
//  - passing nullptr to ntbs() is an undefined behavior (NULL was never a valid NTBS)
//  - certain conversions are explicit:
//      - string/vector -> parray -- we turn object that owns underlying resource into one that doesn't, this prevents problems like:
//          string foo() { ... }
//          rcstring r = foo();
//      - ntbs -> parray -- to find ntbs length we have to scan entire string, it is expensive
//      - parray<T, Trait1> -> parray<E, Trat2> -- changing trait means changing comparison behavior
//  - 'not_eq' is an 'alternative token' for ^ in C++, using yodaism 'eq_not' instead
//  - default trait assumes that comparisons are always commutative, i.e. l == r --> r == l, l < r --> r > l, etc
//  - if len == 0 'p' is never used. I guess you could (probably!) squeeze few extra CPU cycles by not populating 'p' if len == 0, but
//    compiler sometimes complains about potential use of uninitialized variable... so I left it as it is
//  - some ntbs comparisons rely on assumption that for any pair of char types (L and R) following is correct:
//      L l = ...;
//      R r = ...;
//      remove_cv<L> const l_nul{};
//      remove_cv<R> const r_nul{};
//      l == r && l == l_nul -> r == r_nul;     // i.e. if l == r and l is NUL then r is NUL too
//  - should probably sprinkle constexpr everywhere...
//


//------------------------------------------------------------------------------
namespace adv { namespace parray_pvt_ {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Helpers
//
using std::size_t;
using std::basic_string;
using std::char_traits;
using std::vector;
using std::allocator;
using std::memcmp;
using std::basic_ostream;

template<class T> using remove_cv = std::remove_cv_t<T>;
template<bool B, class T = void> using enable_if = std::enable_if_t<B, T>;
template<class T, class U> constexpr bool is_same = std::is_same<T, U>::value;
template<class T> constexpr bool is_volatile = std::is_volatile<T>::value;
template<class T> constexpr bool is_scalar = std::is_scalar<T>::value;
template<class F, class T> constexpr bool is_convertible = std::is_convertible<F, T>::value;
template<class T> constexpr bool is_char = is_same<remove_cv<T>, char> || is_same<remove_cv<T>, wchar_t> || is_same<remove_cv<T>, char16_t> || is_same<remove_cv<T>, char32_t>;
template<class T> constexpr bool is_byte = is_same<remove_cv<T>, unsigned char>;

// relaxed version of 'is_same<remove_cv<E>, remove_cv<T>> && is_convertible<E*, T*>'
template<class E, class T> constexpr bool is_almost_same = (sizeof(E) == sizeof(T)) && is_convertible<E*, T*>;


//------------------------------------------------------------------------------
struct parray_traits
{
    //
    // arrays
    //

protected:
    static bool same_ptr(void volatile const* l, void volatile const* r) { return l == r; }

    // element comparisons
    // using char_traits to compare chars of same base type
    template<class L, class R, enable_if< is_same<remove_cv<L>, remove_cv<R>> &&  is_char<L> >...> constexpr static bool elem_eq(L l, R r) { return char_traits<remove_cv<L>>::eq(l, r); }
    template<class L, class R, enable_if< is_same<remove_cv<L>, remove_cv<R>> &&  is_char<L> >...> constexpr static bool elem_lt(L l, R r) { return char_traits<remove_cv<L>>::lt(l, r); }
    // using operators for everything else
    template<class L, class R, enable_if<!(is_same<remove_cv<L>, remove_cv<R>> && is_char<L>)>...> constexpr static bool elem_eq(L& l, R& r) { return l == r; }
    template<class L, class R, enable_if<!(is_same<remove_cv<L>, remove_cv<R>> && is_char<L>)>...> constexpr static bool elem_lt(L& l, R& r) { return l <  r; }

    // generic comparisons -- used instead of specialized version (when related conditions aren't met)
    template<class L, class R>
    static bool generic_eq(size_t len, L* l, R* r)
    {
        for(; len > 0; --len, ++l, ++r) { if (!elem_eq(*l, *r)) return false; }
        return true;
    }

    template<class L, class R>
    static bool generic_lt(size_t len, L* l, R* r)
    {
        for(; len > 0; --len, ++l, ++r) { if (!elem_eq(*l, *r)) return elem_lt(*l, *r); }
        return false;
    }
    
    // non-volatile chars -> char_traits, non-volatile unsigned chars -> memcmp, rest -> generic algorithm
    template<class L, class R> constexpr static bool is_case1 = !is_volatile<L> && !is_volatile<R> && is_char<L> && is_char<R> && is_same<remove_cv<L>, remove_cv<R>>;
    template<class L, class R> constexpr static bool is_case2 = !is_volatile<L> && !is_volatile<R> && is_byte<L> && is_byte<R>;
    template<class L, class R> constexpr static bool is_case3 = !(is_case1<L, R> || is_case2<L, R>);

    template<class L, class R, enable_if<is_case1<L, R>>...> static bool ar_eq(size_t len, L* l, R* r) { return char_traits<remove_cv<L>>::compare(l, r, len) == 0; }
    template<class L, class R, enable_if<is_case1<L, R>>...> static bool ar_lt(size_t len, L* l, R* r) { return char_traits<remove_cv<L>>::compare(l, r, len) <  0; }

    template<class L, class R, enable_if<is_case2<L, R>>...> static bool ar_eq(size_t len, L* l, R* r) { return memcmp(l, r, len) == 0; }
    template<class L, class R, enable_if<is_case2<L, R>>...> static bool ar_lt(size_t len, L* l, R* r) { return memcmp(l, r, len) <  0; }

    template<class L, class R, enable_if<is_case3<L, R>>...> static bool ar_eq(size_t len, L* l, R* r) { return generic_eq(len, l, r); }
    template<class L, class R, enable_if<is_case3<L, R>>...> static bool ar_lt(size_t len, L* l, R* r) { return generic_lt(len, l, r); }

    // user can't overload operators for these types -- we can use a bit of optimization
    template<class L, class R, enable_if< is_scalar<L> &&  is_scalar<R>>...> static bool ar_eq_(size_t len, L* l, R* r) { return same_ptr (l, r) || ar_eq(len, l, r); }
    template<class L, class R, enable_if< is_scalar<L> &&  is_scalar<R>>...> static bool ar_lt_(size_t len, L* l, R* r) { return !same_ptr(l, r) && ar_lt(len, l, r); }

    // user can overload operators for these types -- stay safe in case he did smth very unusual (like making two objects at the same address not equal)
    template<class L, class R, enable_if<!is_scalar<L> || !is_scalar<R>>...> static bool ar_eq_(size_t len, L* l, R* r) { return ar_eq(len, l, r); }
    template<class L, class R, enable_if<!is_scalar<L> || !is_scalar<R>>...> static bool ar_lt_(size_t len, L* l, R* r) { return ar_lt(len, l, r); }

public:
    // comparisons
    template<class L, class R> static bool eq    (size_t l_len, L* l, size_t r_len, R* r) { return l_len == r_len && ar_eq_(l_len, l, r); }
    template<class L, class R> static bool eq_not(size_t l_len, L* l, size_t r_len, R* r) { return !eq(l_len, l, r_len, r); }			// l != r -> !(l == r)
    template<class L, class R> static bool lt    (size_t l_len, L* l, size_t r_len, R* r) { return l_len < r_len || (l_len == r_len && ar_lt_(l_len, l, r)); }
    template<class L, class R> static bool gt    (size_t l_len, L* l, size_t r_len, R* r) { return lt (r_len, r, l_len, l); }           // l >  r -> r <  l
    template<class L, class R> static bool lt_eq (size_t l_len, L* l, size_t r_len, R* r) { return !lt(r_len, r, l_len, l); }           // l <= r -> r >= l -> !(r < l)
    template<class L, class R> static bool gt_eq (size_t l_len, L* l, size_t r_len, R* r) { return !lt(l_len, l, r_len, r); }           // l >= r -> !(l < r)

    //
    // ntbs
    //

protected:
    // using generic algorithm for all cases (in our comparisons length trumps elements -- there are no library functions with same behavior)
    template<class L, class R>
    static bool ntbs_ar_eq(L* l, R* r)
    {
        static_assert(remove_cv<L>{} == remove_cv<R>{}, "");    // this should hold for any T that is is_char<T>

        for(remove_cv<L> const l_nul{}; ; ++l, ++r)
        {
            if (elem_eq(*l, *r))
                if (!elem_eq(*l, l_nul))
                    ;
                else
                    return true;  // l_nul == r_nul && *l == *r && *l == l_nul -> *r == r_nul -> arrays are equal
            else
                return false;
        }
    }

    template<class L, class R>
    static bool ntbs_ar_eq(size_t l_len, L* l, R* r)
    {
        L* l_end = l + l_len;
        remove_cv<R> const r_nul{};

        for(;; ++l, ++r)
        {
            bool l_done = (l == l_end);
            bool r_done = elem_eq(*r, r_nul);
            if (l_done || r_done) return l_done && r_done;  // true if lengths are same

            if (!elem_eq(*l, *r)) return false;     // we found non-NUL elements that differ -- arrays not equal
        }
    }

    template<class L, class R>
    static bool ntbs_ar_lt(L* l, R* r)
    {
        static_assert(remove_cv<L>{} == remove_cv<R>{}, "");    // this should hold for any T that is is_char<T>

        for(remove_cv<L> const l_nul{}; ; ++l, ++r)
        {
            if (elem_eq(*l, *r))
            {
                if (!elem_eq(*l, l_nul))
                    ;
                else
                    return false;  // l_nul == r_nul && *l == *r && *l == l_nul -> *r == r_nul -> l == r -> !(l < r)
            }
            else    // *l != *r
            {
                if (elem_eq(*l, l_nul)) return true;    // *l == l_nul && *l != *r -> *r != nul -> l < r

                remove_cv<R> const r_nul{};
                if (elem_eq(*r, r_nul)) return false;   // *r == r_nul && *l != *r -> *l != nul -> !(l < r)

                for(bool lexicographical_lt = elem_lt(*l++, *r++); ; ++l, ++r)
                {
                    bool l_done = elem_eq(*l, l_nul);
                    bool r_done = elem_eq(*r, r_nul);
                    if (l_done || r_done)
                        return (l_done && r_done) ? lexicographical_lt : l_done;
                }
            }
        }
    }

    template<class L, class R>
    static bool ntbs_ar_lt(size_t l_len, L* l, R* r)
    {
        L* l_end = l + l_len;
        remove_cv<R> const r_nul{};

        for(;; ++l, ++r)
        {
            bool l_done = (l == l_end);
            bool r_done = elem_eq(*r, r_nul);
            if (l_done || r_done) return l_done && !r_done;  // true if only 'l' end is reached

            if (!elem_eq(*l, *r))   // arrays not equal, lets see if 'l < r'
            {
                for(bool lexicographical_lt = elem_lt(*l++, *r++); ; ++l, ++r)
                {
                    bool l_done = (l == l_end);
                    bool r_done = elem_eq(*r, r_nul);
                    if (l_done || r_done)
                        return (l_done && r_done) ? lexicographical_lt : l_done;
                }
            }
        }
    }

    template<class L, class R>
    static bool ntbs_ar_lt(L* l, size_t r_len, R* r)
    {
        remove_cv<L> const l_nul{};
        R* r_end = r + r_len;

        for(;; ++l, ++r)
        {
            bool l_done = elem_eq(*l, l_nul);
            bool r_done = (r == r_end);
            if (l_done || r_done) return l_done && !r_done;  // true if only 'l' end is reached

            if (!elem_eq(*l, *r))   // arrays not equal, lets see if 'l < r'
            {
                for(bool lexicographical_lt = elem_lt(*l++, *r++); ; ++l, ++r)
                {
                    bool l_done = elem_eq(*l, l_nul);
                    bool r_done = (r == r_end);
                    if (l_done || r_done)
                        return (l_done && r_done) ? lexicographical_lt : l_done;
                }
            }
        }
    }

    // user can't overload operators for these types -- we can use a bit of optimization
    template<class L, class R, enable_if< is_scalar<L> &&  is_scalar<R>>...> static bool ntbs_ar_eq_(L* l, R* r) { return same_ptr (l, r) || ntbs_ar_eq(l, r); }
    template<class L, class R, enable_if< is_scalar<L> &&  is_scalar<R>>...> static bool ntbs_ar_lt_(L* l, R* r) { return !same_ptr(l, r) && ntbs_ar_lt(l, r); }

    // user can overload operators for these types -- stay safe in case he did smth very unusual (like making two objects at the same address not equal)
    template<class L, class R, enable_if<!is_scalar<L> || !is_scalar<R>>...> static bool ntbs_ar_eq_(L* l, R* r) { return ntbs_ar_eq(l, r); }
    template<class L, class R, enable_if<!is_scalar<L> || !is_scalar<R>>...> static bool ntbs_ar_lt_(L* l, R* r) { return ntbs_ar_lt(l, r); }

public:
    // length
    template<class T, enable_if<is_char<T> && !is_volatile<T>>...>                      // char_traits doesn't handle volatile
    static size_t ntbs_len(T* p) { return char_traits<remove_cv<T>>::length(p); }

    template<class T, enable_if<!is_char<T> || is_volatile<T>>...>
    static size_t ntbs_len(T* p)
    {
        size_t count = 0;
        for(remove_cv<T> const nul{}; !elem_eq(*p, nul); ++p) ++count;
        return count;
    }

    // comparisons
    template<class L, class R> static bool ntbs_eq    (L* l, R* r) { return ntbs_ar_eq_(l, r); } 
    template<class L, class R> static bool ntbs_not_eq(L* l, R* r) { return !ntbs_eq   (l, r); }                        // l != r -> !(l == r)
    template<class L, class R> static bool ntbs_lt    (L* l, R* r) { return ntbs_ar_lt_(l, r); }
    template<class L, class R> static bool ntbs_gt    (L* l, R* r) { return ntbs_lt    (r, l); }                        // l >  r -> r <  l
    template<class L, class R> static bool ntbs_lt_eq (L* l, R* r) { return !ntbs_lt   (r, l); }                        // l <= r -> r >= l -> !(r < l)
    template<class L, class R> static bool ntbs_gt_eq (L* l, R* r) { return !ntbs_lt   (l, r); }                        // l >= r -> !(l < r)

    template<class L, class R> static bool ntbs_eq    (size_t l_len, L* l, R* r) { return ntbs_ar_eq(l_len, l, r); }
    template<class L, class R> static bool ntbs_not_eq(size_t l_len, L* l, R* r) { return !ntbs_eq  (l_len, l, r); }    // l != r -> !(l == r)
    template<class L, class R> static bool ntbs_lt    (size_t l_len, L* l, R* r) { return ntbs_ar_lt(l_len, l, r); }
    template<class L, class R> static bool ntbs_gt    (size_t l_len, L* l, R* r) { return ntbs_lt   (r, l_len, l); }    // l >  r -> r <  l
    template<class L, class R> static bool ntbs_lt_eq (size_t l_len, L* l, R* r) { return !ntbs_lt  (r, l_len, l); }    // l <= r -> r >= l -> !(r < l)
    template<class L, class R> static bool ntbs_gt_eq (size_t l_len, L* l, R* r) { return !ntbs_lt  (l_len, l, r); }    // l >= r -> !(l < r)

    template<class L, class R> static bool ntbs_eq    (L* l, size_t r_len, R* r) { return ntbs_eq   (r_len, r, l); }    // l == r -> r == l
    template<class L, class R> static bool ntbs_not_eq(L* l, size_t r_len, R* r) { return !ntbs_eq  (l, r_len, r); }    // l != r -> !(l == r)
    template<class L, class R> static bool ntbs_lt    (L* l, size_t r_len, R* r) { return ntbs_ar_lt(l, r_len, r); }
    template<class L, class R> static bool ntbs_gt    (L* l, size_t r_len, R* r) { return ntbs_lt   (r_len, r, l); }    // l >  r -> r <  l
    template<class L, class R> static bool ntbs_lt_eq (L* l, size_t r_len, R* r) { return !ntbs_lt  (r_len, r, l); }    // l <= r -> r >= l -> !(r < l)
    template<class L, class R> static bool ntbs_gt_eq (L* l, size_t r_len, R* r) { return !ntbs_lt  (l, r_len, r); }    // l >= r -> !(l < r)
};


//------------------------------------------------------------------------------
template<class T, class Traits = parray_traits>
struct parray
{
    typedef T ElemT;
    typedef remove_cv<T> Tc;

    size_t  len;
    T*      p;              // not used if len == 0

    parray() = default;     // default ctor intentionally leaves state undefined 

    // implicit dtor, cctor & op= are ok
    // implicit mctor & opm= are ok

    static parray zero() { return {0, static_cast<T*>(nullptr)}; }                      // change to static constexpr variable in C++17?

    template<class E, enable_if<is_almost_same<E, T>>...>                               // (E*, size_t) -> parray<T> (E* -> T*)
    parray(size_t len, E* p) : len(len), p(p) {}

    template<class E, enable_if<is_almost_same<E, T>>...>                               // parray<E> -> parray<T> (E* -> T*), implicit
    parray(parray<E, Traits> o) : len(o.len), p(o.p) {}

    template<class E, class Traits2, enable_if<is_almost_same<E, T>>...>                // parray<E, Traits2> -> parray<T, Traits> (E* -> T*), explicit
    explicit parray(parray<E, Traits2> o) : len(o.len), p(o.p) {}

    template<class E, size_t len, enable_if<!is_char<E> && is_almost_same<E, T>>...>    // E[] -> parray<T> (E* -> T*), E != char
    parray(E (&r)[len]) : len(len), p(r) {}

    template<class E, class Tr, class A, enable_if<is_almost_same<E const, T>>...>      // string<E> -> parray<T> (E const* -> T*)
    explicit parray(basic_string<E, Tr, A> const& s) : len(s.size()), p(len ? s.data() : nullptr) {}
    
    template<class E, class A, enable_if<is_almost_same<E, T>>...>                      // vector<E> -> parray<T> (E* -> T*)
    explicit parray(vector<E, A>& v) : len(v.size()), p(len ? &v[0] : nullptr) {}

    template<class E, class A, enable_if<is_almost_same<E const, T>>...>                // vector<E> const -> parray<T> (E const* -> T*)
    explicit parray(vector<E, A> const& v) : len(v.size()), p(len ? &v[0] : nullptr) {}

    // cheap access
    size_t size() const             { return len;       }
    bool empty() const              { return len == 0;  }
    T& operator[](size_t i) const   { return p[i];      }

    // expensive conversions
    template<class Tr = char_traits<Tc>, class A = allocator<Tc>>
    basic_string<Tc, Tr, A> str() const { return {p, len}; }

    template<class A = allocator<Tc>>
    vector<Tc, A> vec() const { return {p, p + len}; }

    // comparison operators (forwarding everything to trait in case if user wants to force unusual behaviour)

    // parray<T, Traits> vs parray<E, Traits>
    template<class E> friend inline bool operator==(parray l, parray<E, Traits> r) { return Traits::eq    (l.len, l.p, r.len, r.p); }
    template<class E> friend inline bool operator!=(parray l, parray<E, Traits> r) { return Traits::eq_not(l.len, l.p, r.len, r.p); }
    template<class E> friend inline bool operator< (parray l, parray<E, Traits> r) { return Traits::lt    (l.len, l.p, r.len, r.p); }
    template<class E> friend inline bool operator> (parray l, parray<E, Traits> r) { return Traits::gt    (l.len, l.p, r.len, r.p); }
    template<class E> friend inline bool operator<=(parray l, parray<E, Traits> r) { return Traits::lt_eq (l.len, l.p, r.len, r.p); }
    template<class E> friend inline bool operator>=(parray l, parray<E, Traits> r) { return Traits::gt_eq (l.len, l.p, r.len, r.p); }

    // parray<T, Traits> vs basic_string<E, Tr, A>
    template<class E, class Tr, class A> friend inline bool operator==(parray l, basic_string<E, Tr, A> const& r) { return l == parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator!=(parray l, basic_string<E, Tr, A> const& r) { return l != parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator< (parray l, basic_string<E, Tr, A> const& r) { return l <  parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator> (parray l, basic_string<E, Tr, A> const& r) { return l >  parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator<=(parray l, basic_string<E, Tr, A> const& r) { return l <= parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator>=(parray l, basic_string<E, Tr, A> const& r) { return l >= parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator==(basic_string<E, Tr, A> const& l, parray r) { return parray<E const, Traits>(l) == r; }
    template<class E, class Tr, class A> friend inline bool operator!=(basic_string<E, Tr, A> const& l, parray r) { return parray<E const, Traits>(l) != r; }
    template<class E, class Tr, class A> friend inline bool operator< (basic_string<E, Tr, A> const& l, parray r) { return parray<E const, Traits>(l) <  r; }
    template<class E, class Tr, class A> friend inline bool operator> (basic_string<E, Tr, A> const& l, parray r) { return parray<E const, Traits>(l) >  r; }
    template<class E, class Tr, class A> friend inline bool operator<=(basic_string<E, Tr, A> const& l, parray r) { return parray<E const, Traits>(l) <= r; }
    template<class E, class Tr, class A> friend inline bool operator>=(basic_string<E, Tr, A> const& l, parray r) { return parray<E const, Traits>(l) >= r; }

    // parray<T, Traits> vs vector<E, A>
    template<class E, class A> friend inline bool operator==(parray l, vector<E, A> const& r) { return l == parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator!=(parray l, vector<E, A> const& r) { return l != parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator< (parray l, vector<E, A> const& r) { return l <  parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator> (parray l, vector<E, A> const& r) { return l >  parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator<=(parray l, vector<E, A> const& r) { return l <= parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator>=(parray l, vector<E, A> const& r) { return l >= parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator==(vector<E, A> const& l, parray r) { return parray<E const, Traits>(l) == r; }
    template<class E, class A> friend inline bool operator!=(vector<E, A> const& l, parray r) { return parray<E const, Traits>(l) != r; }
    template<class E, class A> friend inline bool operator< (vector<E, A> const& l, parray r) { return parray<E const, Traits>(l) <  r; }
    template<class E, class A> friend inline bool operator> (vector<E, A> const& l, parray r) { return parray<E const, Traits>(l) >  r; }
    template<class E, class A> friend inline bool operator<=(vector<E, A> const& l, parray r) { return parray<E const, Traits>(l) <= r; }
    template<class E, class A> friend inline bool operator>=(vector<E, A> const& l, parray r) { return parray<E const, Traits>(l) >= r; }

    template<class E, class A> friend inline bool operator==(parray l, vector<E, A>& r) { return l == parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator!=(parray l, vector<E, A>& r) { return l != parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator< (parray l, vector<E, A>& r) { return l <  parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator> (parray l, vector<E, A>& r) { return l >  parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator<=(parray l, vector<E, A>& r) { return l <= parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator>=(parray l, vector<E, A>& r) { return l >= parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator==(vector<E, A>& l, parray r) { return parray<E, Traits>(l) == r; }
    template<class E, class A> friend inline bool operator!=(vector<E, A>& l, parray r) { return parray<E, Traits>(l) != r; }
    template<class E, class A> friend inline bool operator< (vector<E, A>& l, parray r) { return parray<E, Traits>(l) <  r; }
    template<class E, class A> friend inline bool operator> (vector<E, A>& l, parray r) { return parray<E, Traits>(l) >  r; }
    template<class E, class A> friend inline bool operator<=(vector<E, A>& l, parray r) { return parray<E, Traits>(l) <= r; }
    template<class E, class A> friend inline bool operator>=(vector<E, A>& l, parray r) { return parray<E, Traits>(l) >= r; }

    // parray<T, Traits> vs E[len]
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator==(parray l, E (&r)[len]) { return l == parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator!=(parray l, E (&r)[len]) { return l != parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator< (parray l, E (&r)[len]) { return l <  parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator> (parray l, E (&r)[len]) { return l >  parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator<=(parray l, E (&r)[len]) { return l <= parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator>=(parray l, E (&r)[len]) { return l >= parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator==(E (&l)[len], parray r) { return parray<E, Traits>(l) == r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator!=(E (&l)[len], parray r) { return parray<E, Traits>(l) != r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator< (E (&l)[len], parray r) { return parray<E, Traits>(l) <  r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator> (E (&l)[len], parray r) { return parray<E, Traits>(l) >  r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator<=(E (&l)[len], parray r) { return parray<E, Traits>(l) <= r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator>=(E (&l)[len], parray r) { return parray<E, Traits>(l) >= r; }

    // ostream <<
    template<class E, class Tr, enable_if<is_almost_same<T, E const>>...> friend inline basic_ostream<E, Tr>& operator<<(basic_ostream<E, Tr>& os, parray v) { return os.write(v.p, v.len); }
};


//------------------------------------------------------------------------------
// Null-terminated array -- array with last element that is to be ignored (literal string), defined only for non-empty char arrays
//
template<class Traits = parray_traits, class T, size_t len, enable_if<(len > 0) && is_char<T>>...>
inline parray<T, Traits> ntba(T (&r)[len]) { return {len - 1, r}; }


//------------------------------------------------------------------------------
// Null-terminated string -- pointer to a NUL-terminated string (C-style)
//
// Notes:
//  - it is inconvenient to pass array into this function (as intended) -- NUL-terminated arrays are supposed to be handled by 'ntba()'
//  - in case if your array holds string of unknown length (often the case when using things like db-lib) you can:
//      - (expensive) force length recalc via ntbs(&buffer[0]) or
//      - (preferred) use already precalculated length (e.g. len = dbdatlen(...)): rcstring(len, buffer)
//  - be careful with return value -- don't recalc length repeatedly, e.g.:
//      void foo(rcstring v) { ... }
//      ...
//      auto s = ntbs(...);
//      for(...)
//          foo( rcstring(s) );     // bad, length gets calculated here repeatedly
//  - same for comparisons: if same ntbs is going to participate in multiple comparisons -- it is better to convert it into parray
//
template<class T, class Traits>
struct ntbs_t
{
    T* p;

    template<class E, class Traits2, enable_if<is_almost_same<T, E>>...>
    explicit operator parray<E, Traits2>() const { return {Traits::ntbs_len(p), p}; }

    // comparisons

    // ntbs_t<T, Traits> vs ntbs<E, Traits>
    template<class E> friend inline bool operator==(ntbs_t l, ntbs_t<E, Traits> r) { return Traits::ntbs_eq    (l.p, r.p); }
    template<class E> friend inline bool operator!=(ntbs_t l, ntbs_t<E, Traits> r) { return Traits::ntbs_not_eq(l.p, r.p); }
    template<class E> friend inline bool operator< (ntbs_t l, ntbs_t<E, Traits> r) { return Traits::ntbs_lt    (l.p, r.p); }
    template<class E> friend inline bool operator> (ntbs_t l, ntbs_t<E, Traits> r) { return Traits::ntbs_gt    (l.p, r.p); }
    template<class E> friend inline bool operator<=(ntbs_t l, ntbs_t<E, Traits> r) { return Traits::ntbs_lt_eq (l.p, r.p); }
    template<class E> friend inline bool operator>=(ntbs_t l, ntbs_t<E, Traits> r) { return Traits::ntbs_gt_eq (l.p, r.p); }

    // ntbs_t<T, Traits> vs parray<E, Traits>
    template<class E> friend inline bool operator==(ntbs_t l, parray<E, Traits> r) { return Traits::ntbs_eq    (l.p, r.len, r.p); }
    template<class E> friend inline bool operator!=(ntbs_t l, parray<E, Traits> r) { return Traits::ntbs_not_eq(l.p, r.len, r.p); }
    template<class E> friend inline bool operator< (ntbs_t l, parray<E, Traits> r) { return Traits::ntbs_lt    (l.p, r.len, r.p); }
    template<class E> friend inline bool operator> (ntbs_t l, parray<E, Traits> r) { return Traits::ntbs_gt    (l.p, r.len, r.p); }
    template<class E> friend inline bool operator<=(ntbs_t l, parray<E, Traits> r) { return Traits::ntbs_lt_eq (l.p, r.len, r.p); }
    template<class E> friend inline bool operator>=(ntbs_t l, parray<E, Traits> r) { return Traits::ntbs_gt_eq (l.p, r.len, r.p); }
    template<class E> friend inline bool operator==(parray<E, Traits> l, ntbs_t r) { return Traits::ntbs_eq    (l.len, l.p, r.p); }
    template<class E> friend inline bool operator!=(parray<E, Traits> l, ntbs_t r) { return Traits::ntbs_not_eq(l.len, l.p, r.p); }
    template<class E> friend inline bool operator< (parray<E, Traits> l, ntbs_t r) { return Traits::ntbs_lt    (l.len, l.p, r.p); }
    template<class E> friend inline bool operator> (parray<E, Traits> l, ntbs_t r) { return Traits::ntbs_gt    (l.len, l.p, r.p); }
    template<class E> friend inline bool operator<=(parray<E, Traits> l, ntbs_t r) { return Traits::ntbs_lt_eq (l.len, l.p, r.p); }
    template<class E> friend inline bool operator>=(parray<E, Traits> l, ntbs_t r) { return Traits::ntbs_gt_eq (l.len, l.p, r.p); }

    // ntbs_t<T, Traits> vs basic_string<E, Tr, A>
    template<class E, class Tr, class A> friend inline bool operator==(ntbs_t l, basic_string<E, Tr, A> const& r) { return l == parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator!=(ntbs_t l, basic_string<E, Tr, A> const& r) { return l != parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator< (ntbs_t l, basic_string<E, Tr, A> const& r) { return l <  parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator> (ntbs_t l, basic_string<E, Tr, A> const& r) { return l >  parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator<=(ntbs_t l, basic_string<E, Tr, A> const& r) { return l <= parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator>=(ntbs_t l, basic_string<E, Tr, A> const& r) { return l >= parray<E const, Traits>(r); }
    template<class E, class Tr, class A> friend inline bool operator==(basic_string<E, Tr, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) == r; }
    template<class E, class Tr, class A> friend inline bool operator!=(basic_string<E, Tr, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) != r; }
    template<class E, class Tr, class A> friend inline bool operator< (basic_string<E, Tr, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) <  r; }
    template<class E, class Tr, class A> friend inline bool operator> (basic_string<E, Tr, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) >  r; }
    template<class E, class Tr, class A> friend inline bool operator<=(basic_string<E, Tr, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) <= r; }
    template<class E, class Tr, class A> friend inline bool operator>=(basic_string<E, Tr, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) >= r; }

    // ntbs_t<T, Traits> vs vector<E, A>
    template<class E, class A> friend inline bool operator==(ntbs_t l, vector<E, A> const& r) { return l == parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator!=(ntbs_t l, vector<E, A> const& r) { return l != parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator< (ntbs_t l, vector<E, A> const& r) { return l <  parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator> (ntbs_t l, vector<E, A> const& r) { return l >  parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator<=(ntbs_t l, vector<E, A> const& r) { return l <= parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator>=(ntbs_t l, vector<E, A> const& r) { return l >= parray<E const, Traits>(r); }
    template<class E, class A> friend inline bool operator==(vector<E, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) == r; }
    template<class E, class A> friend inline bool operator!=(vector<E, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) != r; }
    template<class E, class A> friend inline bool operator< (vector<E, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) <  r; }
    template<class E, class A> friend inline bool operator> (vector<E, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) >  r; }
    template<class E, class A> friend inline bool operator<=(vector<E, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) <= r; }
    template<class E, class A> friend inline bool operator>=(vector<E, A> const& l, ntbs_t r) { return parray<E const, Traits>(l) >= r; }

    template<class E, class A> friend inline bool operator==(ntbs_t l, vector<E, A>& r) { return l == parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator!=(ntbs_t l, vector<E, A>& r) { return l != parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator< (ntbs_t l, vector<E, A>& r) { return l <  parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator> (ntbs_t l, vector<E, A>& r) { return l >  parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator<=(ntbs_t l, vector<E, A>& r) { return l <= parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator>=(ntbs_t l, vector<E, A>& r) { return l >= parray<E, Traits>(r); }
    template<class E, class A> friend inline bool operator==(vector<E, A>& l, ntbs_t r) { return parray<E, Traits>(l) == r; }
    template<class E, class A> friend inline bool operator!=(vector<E, A>& l, ntbs_t r) { return parray<E, Traits>(l) != r; }
    template<class E, class A> friend inline bool operator< (vector<E, A>& l, ntbs_t r) { return parray<E, Traits>(l) <  r; }
    template<class E, class A> friend inline bool operator> (vector<E, A>& l, ntbs_t r) { return parray<E, Traits>(l) >  r; }
    template<class E, class A> friend inline bool operator<=(vector<E, A>& l, ntbs_t r) { return parray<E, Traits>(l) <= r; }
    template<class E, class A> friend inline bool operator>=(vector<E, A>& l, ntbs_t r) { return parray<E, Traits>(l) >= r; }

    // ntbs_t<T, Traits> vs E[len]
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator==(ntbs_t l, E (&r)[len]) { return l == parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator!=(ntbs_t l, E (&r)[len]) { return l != parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator< (ntbs_t l, E (&r)[len]) { return l <  parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator> (ntbs_t l, E (&r)[len]) { return l >  parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator<=(ntbs_t l, E (&r)[len]) { return l <= parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator>=(ntbs_t l, E (&r)[len]) { return l >= parray<E, Traits>(r); }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator==(E (&l)[len], ntbs_t r) { return parray<E, Traits>(l) == r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator!=(E (&l)[len], ntbs_t r) { return parray<E, Traits>(l) != r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator< (E (&l)[len], ntbs_t r) { return parray<E, Traits>(l) <  r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator> (E (&l)[len], ntbs_t r) { return parray<E, Traits>(l) >  r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator<=(E (&l)[len], ntbs_t r) { return parray<E, Traits>(l) <= r; }
    template<class E, size_t len, enable_if<!is_char<E>>...> friend inline bool operator>=(E (&l)[len], ntbs_t r) { return parray<E, Traits>(l) >= r; }

    // ostream <<
    template<class E, class Tr, enable_if<is_almost_same<T, E const>>...> friend inline basic_ostream<E, Tr>& operator<<(basic_ostream<E, Tr>& os, ntbs_t v) { return os << v.p; }
};

template<class Traits = parray_traits, class T, enable_if<is_char<T>>...>
inline ntbs_t<T, Traits> ntbs(T* const& p) { return {p}; }


//------------------------------------------------------------------------------
} // namespace parray_pvt_
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
using parray_pvt_::parray;
using parray_pvt_::parray_traits;
using parray_pvt_::ntba;
using parray_pvt_::ntbs;
using rbytes    = parray<unsigned char>;
using rcbytes   = parray<unsigned char const>;
using rstring   = parray<char>;
using rcstring  = parray<char const>;
using rwstring  = parray<wchar_t>;
using rcwstring = parray<wchar_t const>;


//------------------------------------------------------------------------------
} // namespace adv
//------------------------------------------------------------------------------


#endif //PARRAY_H_2016_08_30_04_03_32_135_H_
