#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "parray.h"
#include "parray_tools.h"
#include "catch.h"
#include <iostream>
#include "str_printf.h"


//------------------------------------------------------------------------------
using namespace std;
using namespace adv;
using common::str_printf;


//------------------------------------------------------------------------------
TEST_CASE("parray test", "[parray]")
{
    SECTION("simple parray tests")
    {
        rcstring r1 = ntba("123");          // creates array with len = 3
        REQUIRE(r1.size() == 3);

        char const* p = "123";
        REQUIRE( r1 == ntbs(p) );

        parray<char volatile const> r2 { r1 };
        REQUIRE( r1 == r2 );
        REQUIRE( r2 == r1 );
    }

    parray<int> p1{};
    parray<int const> p2(p1);
    parray<int volatile> p3 = {0, p1.p};
    parray<int const> p4{p1.str()};
    parray<int const> p5{p3.vec()};

    SECTION("compile-time checks")
    {
        REQUIRE(is_constructible<parray<int>>::value);
        REQUIRE(is_trivially_constructible<parray<int>>::value);
        REQUIRE(is_nothrow_constructible<parray<int>>::value);

        REQUIRE(is_default_constructible<parray<int>>::value);
        REQUIRE(is_trivially_default_constructible<parray<int>>::value);
        REQUIRE(is_nothrow_default_constructible<parray<int>>::value);

        REQUIRE(is_copy_constructible<parray<int>>::value);
        REQUIRE(is_trivially_copy_constructible<parray<int>>::value);
        REQUIRE(is_nothrow_copy_constructible<parray<int>>::value);

        REQUIRE(is_move_constructible<parray<int>>::value);
        REQUIRE(is_trivially_move_constructible<parray<int>>::value);
        REQUIRE(is_nothrow_move_constructible<parray<int>>::value);

        REQUIRE(is_copy_assignable<parray<int>>::value);
        REQUIRE(is_trivially_copy_assignable<parray<int>>::value);
        REQUIRE(is_nothrow_copy_assignable<parray<int>>::value);

        REQUIRE(is_move_assignable<parray<int>>::value);
        REQUIRE(is_trivially_move_assignable<parray<int>>::value);
        REQUIRE(is_nothrow_move_assignable<parray<int>>::value);

        REQUIRE(is_destructible<parray<int>>::value);
        REQUIRE(is_trivially_destructible<parray<int>>::value);
        REQUIRE(is_nothrow_destructible<parray<int>>::value);

        REQUIRE(!has_virtual_destructor<parray<int>>::value);
        REQUIRE(is_pod<parray<int>>::value);
        REQUIRE(is_standard_layout<parray<int>>::value);
        REQUIRE(is_trivial<parray<int>>::value);
    }

    SECTION("")
    {
        basic_string<int> s;
        parray<int const> d(s);
        REQUIRE(d == s);
    }

    SECTION("")
    {
        rcstring p = ntba("9876543210");

        char buf[1024];
        str_printf(buf, "%.*s", (int)p.len, p.p);
        REQUIRE(string(buf) == "9876543210");
    }

    SECTION("")
    {
        wchar_t s[] = L"123";
        parray<wchar_t> p = ntba(s);

        char buf[1024];
        str_printf(buf, "%.*ls", (int)p.len, p.p);
        REQUIRE(string(buf) == "123");
    }

   SECTION("")
    {
        rcstring p{5, "9876543210"};

        char buf[1024];
        str_printf(buf, "%.*s", (int)p.len, p.p);
        REQUIRE(string(buf) == "98765");
    }

    SECTION("")
    {
        int k[] = {1,2,3,4};
        parray<int> p = k;

        char buf[1024];
        str_printf(buf, "%zu", p.len);
        REQUIRE(string(buf) == "4");
    }

    SECTION("")
    {
        char const* s = "0123456789";
        rcstring p { ntbs(&s[0]) };

        char buf[1024];
        str_printf(buf, "%.*s", (int)p.len, p.p);
        REQUIRE(string(buf) == "0123456789");
    }

    SECTION("")
    {
        parray<volatile char const> p1 = ntba("ABC");
        parray<volatile char const> p2{ ntbs("ABC"+0) };

        REQUIRE(p1 == ntbs("ABC"+0));
        REQUIRE(p2 == ntba("ABC"));
    }

    SECTION("")
    {
        volatile char s[] = "abcd";
        parray<volatile const char> p { ntbs(&s[0]) };

        char buf[1024];
        str_printf(buf, "%.*s", (int)p.len, p.p);
        REQUIRE(string(buf) == "abcd");

        REQUIRE_FALSE(p == rcstring());
        REQUIRE_FALSE(p == ntba("AAA"));
        REQUIRE_FALSE(p == ntbs("AAA"+0));
    }

    SECTION("")
    {
        REQUIRE( ntba("") == ntba(""));
        REQUIRE( ntba("abc") == ntba("abc"));
        REQUIRE( ntba("abc") != ntba("abd"));
        REQUIRE( ntba("ab") < ntba("abc"));
        REQUIRE( ntba("ab") <= ntba("abc"));
        REQUIRE( !(ntba("ab") > ntba("abc")));
        REQUIRE( !(ntba("ab") >= ntba("abc")));
    }

    SECTION("")
    {
        char volatile s[] = "abc";
        parray<char volatile> p1 = ntba(s);
        REQUIRE( p1 == ntba("abc") );

        int d[] = {1,2,3};
        parray<int> p2 = d;
        
        REQUIRE( p1 != p2 );

        parray<char volatile const> p3 = ntba(s);

        REQUIRE( p1 == p3 );

        REQUIRE( p2 != string("abc") );
        REQUIRE( p1 == string("abc") );

        vector<int> v1 = {2,3,4};
        REQUIRE( v1 != p2 );
        REQUIRE( v1 > p2 );

        vector<int> v2 = {1,2,3};
        REQUIRE( v2 == p2 );
        REQUIRE( v2 >= p2 );

        REQUIRE( d == p2 );
        REQUIRE( p1 != d );
        REQUIRE( p1 > d );
        // REQUIRE( p1 != "abc" );  // should not compile

        ostringstream buf;
        buf << ntba("abcde");
        REQUIRE(buf.str() == "abcde");
    }

    SECTION("")
    {
        char const* p1 = "abc";
        REQUIRE( ntba("abbd") > ntbs(p1) );
        REQUIRE( !(ntba("ad") > ntbs(p1)) );
        REQUIRE( ntbs(p1) == ntbs(p1) );
        REQUIRE( ntbs("a"+0) != ntbs("b"+0) );
        REQUIRE( ntbs("a"+0) < ntbs("b"+0) );
        REQUIRE( ntbs("a"+0) <= ntbs("b"+0) );
        REQUIRE( ntbs("a"+0) != ntbs("bb"+0) );
        REQUIRE( ntbs("a"+0) < ntbs("bb"+0) );
        REQUIRE( ntbs("a"+0) <= ntbs("bb"+0) );

        int volatile d[] = {65};
        REQUIRE( (ntbs("A"+0) == d) == true );
        REQUIRE( (ntbs("A"+0) >= d) == true );
        REQUIRE( (ntbs("A"+0) <= d) == true );
        REQUIRE( (ntbs("a"+0) != d) == true );
        REQUIRE( (ntbs("a"+0) > d) == true );
        REQUIRE( (ntbs("a"+0) >= d) == true );
        REQUIRE( (ntbs("AA"+0) != d) == true );
        REQUIRE( (ntbs("AA"+0) > d) == true );
        REQUIRE( (ntbs("AA"+0) >= d) == true );
        REQUIRE( (d == ntbs("A"+0)) == true );
        REQUIRE( (d <= ntbs("A"+0) ) == true );
        REQUIRE( (d >= ntbs("A"+0) ) == true);
        REQUIRE( (d != ntbs("a"+0) ) == true);
        REQUIRE( (d < ntbs("a"+0) ) == true);
        REQUIRE( (d <= ntbs("a"+0) ) == true);
        REQUIRE( (d != ntbs("AA"+0) ) == true);
        REQUIRE( (d < ntbs("AA"+0) ) == true);
        REQUIRE( (d <= ntbs("AA"+0) ) == true);

        parray<int const> e{};
        REQUIRE( ntbs(""+0) == e );
        REQUIRE( ntbs(""+0) >= e );
        REQUIRE( ntbs(""+0) <= e );
        REQUIRE( ntbs("A"+0) != e );
        REQUIRE( ntbs("A"+0) > e );
        REQUIRE( ntbs("A"+0) >= e );

        REQUIRE( e == ntbs(""+0) );
        REQUIRE( e <= ntbs(""+0) );
        REQUIRE( e >= ntbs(""+0) );
        REQUIRE( e != ntbs("A"+0) );
        REQUIRE( e < ntbs("A"+0) );
        REQUIRE( e <= ntbs("A"+0) );
    }
}


//------------------------------------------------------------------------------
TEST_CASE("parray_tools test", "[parray_tools]")
{
    SECTION("trim family")
    {
        REQUIRE( trim( ntba("   123   ") ) == ntba("123") );
        REQUIRE( trim( ntba(" \t  123 \n  ") ) == ntba("123") );
        REQUIRE( trim( ntba("   123") ) == ntba("123") );
        REQUIRE( trim( ntba("123 \n  ") ) == ntba("123") );
        REQUIRE( trim( ntba("\n  ") ) == ntba("") );

        REQUIRE( trim_left( ntba("   123   ") )  == ntba("123   ") );
        REQUIRE( trim_left( ntba(" \n  123") )  == ntba("123") );
        REQUIRE( trim_left( ntba("\n  ") )  == ntba("") );

        REQUIRE( trim_right( ntba("   123   ") )  == ntba("   123") );
        REQUIRE( trim_right( ntba("123 \n  ") )  == ntba("123") );
        REQUIRE( trim_right( ntba("\n  ") )  == ntba("") );
    }

    SECTION("starts_with/ends_with 1")
    {
        rcstring data(ntba("123"));

        REQUIRE( starts_with( data, ntba("12") ) );
        REQUIRE( starts_with( data, ntba("123") ) );
        REQUIRE( starts_with( data, ntba("1") ) );
        REQUIRE( starts_with( data, ntba("") ) );
        REQUIRE( !starts_with( data, ntba("23") ) );

        REQUIRE( ends_with( data, ntba("23") ) );
        REQUIRE( ends_with( data, ntba("123") ) );
        REQUIRE( ends_with( data, ntba("3") ) );
        REQUIRE( ends_with( data, ntba("") ) );
        REQUIRE( !ends_with( data, ntba("12") ) );
    }
    
    SECTION("starts_with/ends_with 2")
    { // same but for rstring
        char s[] = "123";
        rstring data(ntba(s));

        REQUIRE( starts_with( data, ntba("12") ) );
        REQUIRE( starts_with( data, ntba("123") ) );
        REQUIRE( starts_with( data, ntba("1") ) );
        REQUIRE( starts_with( data, ntba("") ) );
        REQUIRE( !starts_with( data, ntba("23") ) );

        REQUIRE( ends_with( data, ntba("23") ) );
        REQUIRE( ends_with( data, ntba("123") ) );
        REQUIRE( ends_with( data, ntba("3") ) );
        REQUIRE( ends_with( data, ntba("") ) );
        REQUIRE( !ends_with( data, ntba("12") ) );
    }

    SECTION("contains")
    {
        REQUIRE( contains(ntba("123"), ntba("2")) );
        REQUIRE( contains(ntba("123"), ntba("123")) );
        REQUIRE( !contains(ntba("123"), ntba("1234")) );
        REQUIRE( !contains(ntba("123"), ntba("4")) );
        REQUIRE( contains(ntba("123 123"), ntba("123")) );
        REQUIRE( contains(ntba("123 123"), ntba("3 1")) );

        char v1[] = "123 123";
        REQUIRE( contains(ntba(v1), rcstring(3, v1 + 4)) == v1 + 4 );
    }

    SECTION("split")
    {
        rcstring s = ntba("1 2  3");
        split(s, ' ', [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( split(s, ' ') == (deque<rcstring>{ntba("1"), ntba("2"), ntba(""), ntba("3")}) );

        rcstring buf[3];
        REQUIRE( split(s, ' ', buf) == 3 );
        REQUIRE( buf[0] == ntba("1") );
        REQUIRE( buf[1] == ntba("2") );
        REQUIRE( buf[2] == ntba(" 3") );
    }

    SECTION("split parray")
    {
        rcstring delim = ntba(" ,");
        rcstring s = ntba("1 2, 3");
        split(s, delim, [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( split(s, delim) == (deque<rcstring>{ntba("1"), ntba("2"), ntba(""), ntba("3")}) );

        rcstring buf[3];
        REQUIRE( split(s, delim, buf) == 3 );
        REQUIRE( buf[0] == ntba("1") );
        REQUIRE( buf[1] == ntba("2") );
        REQUIRE( buf[2] == ntba(" 3") );
    }

    SECTION("split_se")
    {
        rcstring s = ntba("1 2  3");
        split_se(s, ' ', [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( split_se(s, ' ') == (deque<rcstring>{ntba("1"), ntba("2"), ntba("3")}) );

        rcstring buf[2];
        REQUIRE( split_se(s, ' ', buf) == 2 );
        REQUIRE( buf[0] == ntba("1") );
        REQUIRE( buf[1] == ntba("2  3") );

        {
        rcstring buf[3];
        REQUIRE( split_se(s, ' ', buf) == 3 );
        REQUIRE( buf[0] == ntba("1") );
        REQUIRE( buf[1] == ntba("2") );
        REQUIRE( buf[2] == ntba("3") );
        }
    }

    SECTION("split_se parray")
    {
        rcstring delim = ntba(" ,");
        rcstring s = ntba("1 2, 3");
        split_se(s, delim, [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( split_se(s, delim) == (deque<rcstring>{ntba("1"), ntba("2"), ntba("3")}) );

        rcstring buf[2];
        REQUIRE( split_se(s, delim, buf) == 2 );
        REQUIRE( buf[0] == ntba("1") );
        REQUIRE( buf[1] == ntba("2, 3") );

        {
        rcstring buf[3];
        REQUIRE( split_se(s, delim, buf) == 3 );
        REQUIRE( buf[0] == ntba("1") );
        REQUIRE( buf[1] == ntba("2") );
        REQUIRE( buf[2] == ntba("3") );
        }
    }

    SECTION("rsplit")
    {
        printf("----------------\n");

        rcstring s = ntba("1 2  3");
        rsplit(s, ' ', [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( rsplit(s, ' ') == (deque<rcstring>{ntba("3"), ntba(""), ntba("2"), ntba("1")}) );

        rcstring buf[3];
        REQUIRE( rsplit(s, ' ', buf) == 3 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("") );
        REQUIRE( buf[2] == ntba("1 2") );

        {
        rcstring buf[10];
        REQUIRE( rsplit(s, ' ', buf) == 4 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("") );
        REQUIRE( buf[2] == ntba("2") );
        REQUIRE( buf[3] == ntba("1") );
        }
    }

    SECTION("rsplit parray")
    {
        rcstring delim = ntba(" ,");
        rcstring s = ntba("1 2, 3");
        rsplit(s, delim, [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( rsplit(s, delim) == (deque<rcstring>{ntba("3"), ntba(""), ntba("2"), ntba("1")}) );

        rcstring buf[3];
        REQUIRE( rsplit(s, delim, buf) == 3 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("") );
        REQUIRE( buf[2] == ntba("1 2") );
    }

    SECTION("rsplit_se")
    {
        rcstring s = ntba("1  2  3");
        rsplit_se(s, ' ', [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( rsplit_se(s, ' ') == (deque<rcstring>{ntba("3"), ntba("2"), ntba("1")}) );

        rcstring buf[2];
        REQUIRE( rsplit_se(s, ' ', buf) == 2 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("1  2") );

        {
        rcstring buf[3];
        REQUIRE( rsplit_se(s, ' ', buf) == 3 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("2") );
        REQUIRE( buf[2] == ntba("1") );
        }
    }

    SECTION("rsplit_se parray")
    {
        rcstring delim = ntba(" ,");
        rcstring s = ntba("1 ,2  3");
        rsplit_se(s, delim, [](auto v){ printf("'%.*s'\n", (int)v.len, v.p); return false; });

        REQUIRE( rsplit_se(s, delim) == (deque<rcstring>{ntba("3"), ntba("2"), ntba("1")}) );

        rcstring buf[2];
        REQUIRE( rsplit_se(s, delim, buf) == 2 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("1 ,2") );

        {
        rcstring buf[3];
        REQUIRE( rsplit_se(s, delim, buf) == 3 );
        REQUIRE( buf[0] == ntba("3") );
        REQUIRE( buf[1] == ntba("2") );
        REQUIRE( buf[2] == ntba("1") );
        }
    }

    SECTION("join")
    {
        rcstring delim = ntba(" ,");
        rcstring s = ntba("1 2, 3");
        auto v = split(s, delim);

        REQUIRE( split(s, delim) == (deque<rcstring>{ntba("1"), ntba("2"), ntba(""), ntba("3")}) );
        REQUIRE( join<vector<char>>(begin(v), end(v), ' ') == ntba("1 2  3"));
        REQUIRE( join_se<vector<char>>(begin(v), end(v), ntba(" ")) == ntba("1 2 3"));
    }

    SECTION("rjoin")
    {
        rcstring delim = ntba(" ,");
        rcstring s = ntba("1 2, 3");
        auto v = rsplit(s, delim);

        REQUIRE( rsplit(s, delim) == (deque<rcstring>{ntba("3"), ntba(""), ntba("2"), ntba("1")}) );
        REQUIRE( rjoin<vector<char>>(begin(v), end(v), ' ') == ntba("1 2  3") ) ;
        REQUIRE( rjoin_se<string>(begin(v), end(v), ' ') == ntba("1 2 3"));
    }
}


TEST_CASE("split_and_join", "[split_and_join]")
{
    SECTION("")
    {   // unbounded split with functor
        char s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        split(ntba(s), ',', [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ntba("1"), ntba("2"), ntba("3"), ntba(""), ntba(" 4 .")}) );
        REQUIRE( s == join<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",1,2,3,, 4 .,";
        deque<rcstring> q = split(ntba(s), ',');

        REQUIRE( q == (deque<rcstring>{ntba(""), ntba("1"), ntba("2"), ntba("3"), ntba(""), ntba(" 4 ."), ntba("")}) );
        REQUIRE( s == join<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = "1,2,3,, 4 .";
        rcstring buf[6];
        size_t res = split(ntba(s), ',', &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3,, 4 .") == buf[2] );
        REQUIRE( s == join<string>(buf, buf + res, ',') );

        res = split(ntba(s), ',', &buf[0], 5);

        REQUIRE( 5 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("") == buf[3] );
        REQUIRE( ntba(" 4 .") == buf[4] );
        REQUIRE( s == join<string>(buf, buf + res, ',') );

        res = split(ntba(s), ',', &buf[0], 6);

        REQUIRE( 5 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("") == buf[3] );
        REQUIRE( ntba(" 4 .") == buf[4] );
        REQUIRE( s == join<string>(buf, buf + res, ',') );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = "1,2,3,, 4 .";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = split(ntba(s), ',', buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3,, 4 .") == buf[2] );
            REQUIRE( s == join<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = split(ntba(s), ',', buf);

            REQUIRE( 5 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("") == buf[3] );
            REQUIRE( ntba(" 4 .") == buf[4] );
            REQUIRE( s == join<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[6];
            size_t res = split(ntba(s), ',', buf);

            REQUIRE( 5 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("") == buf[3] );
            REQUIRE( ntba(" 4 .") == buf[4] );
            REQUIRE( s == join<string>(buf, buf + res, ',') );
        }
    }

    // same but with 'skip empty'

    SECTION("")
    {   // unbounded split with functor
        char s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        split_se(ntba(s), ',', [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ntba("1"), ntba("2"), ntba("3"), ntba(" 4 .")}) );
        REQUIRE( "1,2,3, 4 ." == join<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",,,1,,,2,3,, 4 .,,,";
        deque<rcstring> q = split_se(ntba(s), ',');

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(" 4 .") }) );
        REQUIRE( "1,2,3, 4 ." == join_se<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = ",1,,,2,3,, 4 .,";
        rcstring buf[6];
        size_t res = split_se(ntba(s), ',', &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3,, 4 .,") == buf[2] );
        REQUIRE( "1,2,3,, 4 .," == join_se<string>(buf, buf + res, ',') );

        res = split_se(ntba(s), ',', &buf[0], 4);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 .,") == buf[3] );
        REQUIRE( "1,2,3, 4 .," == join_se<string>(buf, buf + res, ',') );

        res = split_se(ntba(s), ',', &buf[0], 5);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 .") == buf[3] );
        REQUIRE( "1,2,3, 4 ." == join_se<string>(buf, buf + res, ',') );

        res = split_se(ntba(s), ',', &buf[0], 6);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 .") == buf[3] );
        REQUIRE( "1,2,3, 4 ." == join_se<string>(buf, buf + res, ',') );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = ",,,1,,,2,3,, 4 .,";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = split_se(ntba(s), ',', buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3,, 4 .,") == buf[2] );
            REQUIRE( "1,2,3,, 4 .," == join_se<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[4];
            size_t res = split_se(ntba(s), ',', buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba(" 4 .,") == buf[3] );
            REQUIRE( "1,2,3, 4 .," == join_se<string>(buf, buf + res, ',') );

            buf[2].len = 0;
            REQUIRE( "1,2, 4 .," == join_se<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = split_se(ntba(s), ',', buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba(" 4 .") == buf[3] );
            REQUIRE( "1,2,3, 4 ." == join_se<string>(buf, buf + res, ',') );
        }
    }
}


TEST_CASE("rsplit_and_rjoin", "[rsplit_and_rjoin]")
{
    SECTION("")
    {   // unbounded rsplit with functor
        char s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        rsplit(ntba(s), ',', [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ntba(" 4 ."), ntba(""), ntba("3"), ntba("2"), ntba("1")}) );
        REQUIRE( s == rjoin<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // unbounded rsplit
        char const s[] = ",1,2,3,, 4 .,";
        deque<rcstring> q = rsplit(ntba(s), ',');

        REQUIRE( q == (deque<rcstring>{ntba(""), ntba(" 4 ."), ntba(""), ntba("3"), ntba("2"), ntba("1"), ntba("")}) );
        REQUIRE( s == rjoin<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // bounded rsplit with dynamic buffer
        char const s[] = "1,2,3,, 4 .";
        rcstring buf[6];
        size_t res = rsplit(ntba(s), ',', &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("") == buf[1] );
        REQUIRE( ntba("1,2,3") == buf[2] );
        REQUIRE( s == rjoin<string>(buf, buf + res, ',') );

        res = rsplit(ntba(s), ',', &buf[0], 5);

        REQUIRE( 5 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("2") == buf[3] );
        REQUIRE( ntba("1") == buf[4] );
        REQUIRE( s == rjoin<string>(buf, buf + res, ',') );

        res = rsplit(ntba(s), ',', &buf[0], 6);

        REQUIRE( 5 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("2") == buf[3] );
        REQUIRE( ntba("1") == buf[4] );
        REQUIRE( s == rjoin<string>(buf, buf + res, ',') );
    }

    SECTION("")
    {   // bounded rsplit with static buffer
        char const s[] = "1,2,3,, 4 .";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = rsplit(ntba(s), ',', buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba(" 4 .") == buf[0] );
            REQUIRE( ntba("") == buf[1] );
            REQUIRE( ntba("1,2,3") == buf[2] );
            REQUIRE( s == rjoin<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = rsplit(ntba(s), ',', buf);

            REQUIRE( 5 == res );
            REQUIRE( ntba(" 4 .") == buf[0] );
            REQUIRE( ntba("") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("2") == buf[3] );
            REQUIRE( ntba("1") == buf[4] );
            REQUIRE( s == rjoin<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[6];
            size_t res = rsplit(ntba(s), ',', buf);

            REQUIRE( 5 == res );
            REQUIRE( ntba(" 4 .") == buf[0] );
            REQUIRE( ntba("") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("2") == buf[3] );
            REQUIRE( ntba("1") == buf[4] );
            REQUIRE( s == rjoin<string>(buf, buf + res, ',') );
        }
    }

    // same but with 'skip empty'

    SECTION("")
    {   // unbounded rsplit with functor
        char s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        rsplit_se(ntba(s), ',', [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ntba(" 4 ."), ntba("3"), ntba("2"), ntba("1")}) );
        REQUIRE( "1,2,3, 4 ." == rjoin<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // unbounded rsplit
        char const s[] = ",,,1,,,2,3,, 4 .,,,";
        deque<rcstring> q = rsplit_se(ntba(s), ',');

        REQUIRE( q == (deque<rcstring>{ ntba(" 4 ."), ntba("3"), ntba("2"), ntba("1") }) );
        REQUIRE( "1,2,3, 4 ." == rjoin_se<string>(begin(q), end(q), ',') );
    }

    SECTION("")
    {   // bounded rsplit with dynamic buffer
        char const s[] = ",1,,,2,3,, 4 .,";
        rcstring buf[6];
        size_t res = rsplit_se(ntba(s), ',', &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba(",1,,,2") == buf[2] );
        REQUIRE( ",1,,,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );

        res = rsplit_se(ntba(s), ',', &buf[0], 4);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba(",1") == buf[3] );
        REQUIRE( ",1,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );

        res = rsplit_se(ntba(s), ',', &buf[0], 5);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba("1") == buf[3] );
        REQUIRE( "1,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );

        res = rsplit_se(ntba(s), ',', &buf[0], 6);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 .") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba("1") == buf[3] );
        REQUIRE( "1,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );
    }

    SECTION("")
    {   // bounded rsplit with static buffer
        char const s[] = ",,,1,,,2,3,, 4 .,";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = rsplit_se(ntba(s), ',', buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba(" 4 .") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba(",,,1,,,2") == buf[2] );
            REQUIRE( ",,,1,,,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[4];
            size_t res = rsplit_se(ntba(s), ',', buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba(" 4 .") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba("2") == buf[2] );
            REQUIRE( ntba(",,,1") == buf[3] );
            REQUIRE( ",,,1,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );

            buf[2].len = 0;
            REQUIRE( ",,,1,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = rsplit_se(ntba(s), ',', buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba(" 4 .") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba("2") == buf[2] );
            REQUIRE( ntba("1") == buf[3] );
            REQUIRE( "1,2,3, 4 ." == rjoin_se<string>(buf, buf + res, ',') );
        }
    }
}


TEST_CASE("split_and_join_bitset", "[split_and_join_bitset]")
{
    bitset_delim<> delim(ntba(",."));
    char jdelim[] = "_._";

    SECTION("")
    {   // unbounded split with functor
        char s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        split(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(""), ntba(" 4 "), ntba("") }) );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",1,2,3,, 4 .,";
        deque<rcstring> q = split(ntba(s), delim);

        REQUIRE( q == (deque<rcstring>{ ntba(""), ntba("1"), ntba("2"), ntba("3"), ntba(""), ntba(" 4 "), ntba(""), ntba("") }) );
        REQUIRE( "_._1_._2_._3_.__._ 4 _.__._" == join<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char s[] = "1,2,3,, 4 .";
        rstring buf[9];
        size_t res = split(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3,, 4 .") == buf[2] );
        REQUIRE( "1_._2_._3,, 4 ." == join<string>(buf, buf + res, ntba(jdelim)) );

        res = split(ntba(s), delim, &buf[0], 6);

        REQUIRE( 6 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("") == buf[3] );
        REQUIRE( ntba(" 4 ") == buf[4] );
        REQUIRE( ntba("") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );

        res = split(ntba(s), delim, &buf[0], 7);

        REQUIRE( 6 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("") == buf[3] );
        REQUIRE( ntba(" 4 ") == buf[4] );
        REQUIRE( ntba("") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = "1,2,3,, 4 .";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = split(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3,, 4 .") == buf[2] );
            REQUIRE( "1_._2_._3,, 4 ." == join<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[6];
            size_t res = split(ntba(s), delim, buf);

            REQUIRE( 6 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("") == buf[3] );
            REQUIRE( ntba(" 4 ") == buf[4] );
            REQUIRE( ntba("") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[7];
            size_t res = split(ntba(s), delim, buf);

            REQUIRE( 6 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("") == buf[3] );
            REQUIRE( ntba(" 4 ") == buf[4] );
            REQUIRE( ntba("") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );
        }
    }

    // same but with 'skip empty'

    SECTION("")
    {   // unbounded split with functor
        char const s[] = ",1,,,2,3,, 4 .,";
        deque<rcstring> q;
        split_se(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(" 4 ") }) );
        REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",,,1,,,2,3,, 4 .,,,";
        deque<rcstring> q = split_se(ntba(s), delim);

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(" 4 ") }) );
        REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = ",1,,,2,3,, 4 .,";
        rcstring buf[6];
        size_t res = split_se(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3,, 4 .,") == buf[2] );
        REQUIRE( "1_._2_._3,, 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );

        res = split_se(ntba(s), delim, &buf[0], 4);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 .,") == buf[3] );
        REQUIRE( "1_._2_._3_._ 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );

        res = split_se(ntba(s), delim, &buf[0], 5);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 ") == buf[3] );
        REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = ",,,1,,,2,3,, 4 .,";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = split_se(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3,, 4 .,") == buf[2] );
            REQUIRE( "1_._2_._3,, 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[4];
            size_t res = split_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba(" 4 .,") == buf[3] );
            REQUIRE( "1_._2_._3_._ 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );

            buf[2].len = 0;
            REQUIRE( "1_._2_._ 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = split_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba(" 4 ") == buf[3] );
            REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(buf, buf + res, ntba(jdelim)) );
        }
    }
}


TEST_CASE("rsplit_and_rjoin_bitset", "[rsplit_and_rjoin_bitset]")
{
    bitset_delim<> delim(ntba(",."));
    char jdelim[] = "_._";

    SECTION("")
    {   // unbounded split with functor
        char const s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        rsplit(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( 6 == q.size() );
        REQUIRE( ntba("") == q[0] );
        REQUIRE( ntba(" 4 ") == q[1] );
        REQUIRE( ntba("") == q[2] );
        REQUIRE( ntba("3") == q[3] );
        REQUIRE( ntba("2") == q[4] );
        REQUIRE( ntba("1") == q[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",1,2,3,, 4 .,";
        deque<rcstring> q = rsplit(ntba(s), delim);

        REQUIRE( 8 == q.size() );
        REQUIRE( ntba("") == q[0] );
        REQUIRE( ntba("") == q[1] );
        REQUIRE( ntba(" 4 ") == q[2] );
        REQUIRE( ntba("") == q[3] );
        REQUIRE( ntba("3") == q[4] );
        REQUIRE( ntba("2") == q[5] );
        REQUIRE( ntba("1") == q[6] );
        REQUIRE( ntba("") == q[7] );
        REQUIRE( "_._1_._2_._3_.__._ 4 _.__._" == rjoin<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = "1,2,3,, 4 .";
        rcstring buf[6];
        size_t res = rsplit(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("") == buf[0] );
        REQUIRE( ntba(" 4 ") == buf[1] );
        REQUIRE( ntba("1,2,3,") == buf[2] );
        REQUIRE( "1,2,3,_._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit(ntba(s), delim, &buf[0], 6);

        REQUIRE( 6 == res );
        REQUIRE( ntba("") == buf[0] );
        REQUIRE( ntba(" 4 ") == buf[1] );
        REQUIRE( ntba("") == buf[2] );
        REQUIRE( ntba("3") == buf[3] );
        REQUIRE( ntba("2") == buf[4] );
        REQUIRE( ntba("1") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit(ntba(s), delim, &buf[0], 7);

        REQUIRE( 6 == res );
        REQUIRE( ntba("") == buf[0] );
        REQUIRE( ntba(" 4 ") == buf[1] );
        REQUIRE( ntba("") == buf[2] );
        REQUIRE( ntba("3") == buf[3] );
        REQUIRE( ntba("2") == buf[4] );
        REQUIRE( ntba("1") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = "1,2,3,, 4 .";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = rsplit(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("") == buf[0] );
            REQUIRE( ntba(" 4 ") == buf[1] );
            REQUIRE( ntba("1,2,3,") == buf[2] );
            REQUIRE( "1,2,3,_._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[6];
            size_t res = rsplit(ntba(s), delim, buf);

            REQUIRE( 6 == res );
            REQUIRE( ntba("") == buf[0] );
            REQUIRE( ntba(" 4 ") == buf[1] );
            REQUIRE( ntba("") == buf[2] );
            REQUIRE( ntba("3") == buf[3] );
            REQUIRE( ntba("2") == buf[4] );
            REQUIRE( ntba("1") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[7];
            size_t res = rsplit(ntba(s), delim, buf);

            REQUIRE( ntba("") == buf[0] );
            REQUIRE( ntba(" 4 ") == buf[1] );
            REQUIRE( ntba("") == buf[2] );
            REQUIRE( ntba("3") == buf[3] );
            REQUIRE( ntba("2") == buf[4] );
            REQUIRE( ntba("1") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
        }
    }

    // same but with 'skip_empty_t'

    SECTION("")
    {   // unbounded split with functor
        char const s[] = ",1,,,2,3,, 4 .,";
        deque<rcstring> q;
        rsplit_se(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( 4 == q.size() );
        REQUIRE( ntba(" 4 ") == q[0] );
        REQUIRE( ntba("3") == q[1] );
        REQUIRE( ntba("2") == q[2] );
        REQUIRE( ntba("1") == q[3] );
        REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",,,1,,,2,3,, 4 .,,,";
        deque<rcstring> q = rsplit_se(ntba(s), delim);

        REQUIRE( 4 == q.size() );
        REQUIRE( ntba(" 4 ") == q[0] );
        REQUIRE( ntba("3") == q[1] );
        REQUIRE( ntba("2") == q[2] );
        REQUIRE( ntba("1") == q[3] );
        REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = ",1,,,2,3,, 4 .,";
        rcstring buf[6];
        size_t res = rsplit_se(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba(" 4 ") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba(",1,,,2") == buf[2] );
        REQUIRE( ",1,,,2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit_se(ntba(s), delim, &buf[0], 4);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 ") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba(",1") == buf[3] );
        REQUIRE( ",1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit_se(ntba(s), delim, &buf[0], 5);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 ") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba("1") == buf[3] );
        REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = ",1,,,2,3,, 4 .,,,";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = rsplit_se(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba(" 4 ") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba(",1,,,2") == buf[2] );
            REQUIRE( ",1,,,2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[4];
            size_t res = rsplit_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba(" 4 ") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba("2") == buf[2] );
            REQUIRE( ntba(",1") == buf[3] );
            REQUIRE( ",1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );

            buf[2].len = 0;
            REQUIRE( ",1_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = rsplit_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba(" 4 ") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba("2") == buf[2] );
            REQUIRE( ntba("1") == buf[3] );
            REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
        }
    }
}


TEST_CASE("split_and_join_parray", "[split_and_join_parray]")
{
    rcstring delim = ntba(",.");
    char jdelim[] = "_._";

    SECTION("")
    {   // unbounded split with functor
        char s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        split(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(""), ntba(" 4 "), ntba("") }) );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",1,2,3,, 4 .,";
        deque<rcstring> q = split(ntba(s), delim);

        REQUIRE( q == (deque<rcstring>{ ntba(""), ntba("1"), ntba("2"), ntba("3"), ntba(""), ntba(" 4 "), ntba(""), ntba("") }) );
        REQUIRE( "_._1_._2_._3_.__._ 4 _.__._" == join<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char s[] = "1,2,3,, 4 .";
        rstring buf[9];
        size_t res = split(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3,, 4 .") == buf[2] );
        REQUIRE( "1_._2_._3,, 4 ." == join<string>(buf, buf + res, ntba(jdelim)) );

        res = split(ntba(s), delim, &buf[0], 6);

        REQUIRE( 6 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("") == buf[3] );
        REQUIRE( ntba(" 4 ") == buf[4] );
        REQUIRE( ntba("") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );

        res = split(ntba(s), delim, &buf[0], 7);

        REQUIRE( 6 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba("") == buf[3] );
        REQUIRE( ntba(" 4 ") == buf[4] );
        REQUIRE( ntba("") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = "1,2,3,, 4 .";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = split(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3,, 4 .") == buf[2] );
            REQUIRE( "1_._2_._3,, 4 ." == join<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[6];
            size_t res = split(ntba(s), delim, buf);

            REQUIRE( 6 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("") == buf[3] );
            REQUIRE( ntba(" 4 ") == buf[4] );
            REQUIRE( ntba("") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[7];
            size_t res = split(ntba(s), delim, buf);

            REQUIRE( 6 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba("") == buf[3] );
            REQUIRE( ntba(" 4 ") == buf[4] );
            REQUIRE( ntba("") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == join<string>(buf, buf + res, ntba(jdelim)) );
        }
    }

    // same but with 'skip empty'

    SECTION("")
    {   // unbounded split with functor
        char const s[] = ",1,,,2,3,, 4 .,";
        deque<rcstring> q;
        split_se(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(" 4 ") }) );
        REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",,,1,,,2,3,, 4 .,,,";
        deque<rcstring> q = split_se(ntba(s), delim);

        REQUIRE( q == (deque<rcstring>{ ntba("1"), ntba("2"), ntba("3"), ntba(" 4 ") }) );
        REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = ",1,,,2,3,, 4 .,";
        rcstring buf[6];
        size_t res = split_se(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3,, 4 .,") == buf[2] );
        REQUIRE( "1_._2_._3,, 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );

        res = split_se(ntba(s), delim, &buf[0], 4);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 .,") == buf[3] );
        REQUIRE( "1_._2_._3_._ 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );

        res = split_se(ntba(s), delim, &buf[0], 5);

        REQUIRE( 4 == res );
        REQUIRE( ntba("1") == buf[0] );
        REQUIRE( ntba("2") == buf[1] );
        REQUIRE( ntba("3") == buf[2] );
        REQUIRE( ntba(" 4 ") == buf[3] );
        REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = ",,,1,,,2,3,, 4 .,";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = split_se(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3,, 4 .,") == buf[2] );
            REQUIRE( "1_._2_._3,, 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[4];
            size_t res = split_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba(" 4 .,") == buf[3] );
            REQUIRE( "1_._2_._3_._ 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );

            buf[2].len = 0;
            REQUIRE( "1_._2_._ 4 .," == join_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = split_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba("1") == buf[0] );
            REQUIRE( ntba("2") == buf[1] );
            REQUIRE( ntba("3") == buf[2] );
            REQUIRE( ntba(" 4 ") == buf[3] );
            REQUIRE( "1_._2_._3_._ 4 " == join_se<string>(buf, buf + res, ntba(jdelim)) );
        }
    }
}


TEST_CASE("rsplit_and_rjoin_parray", "[rsplit_and_rjoin_parray]")
{
    rcstring delim = ntba(",.");
    char jdelim[] = "_._";

    SECTION("")
    {   // unbounded split with functor
        char const s[] = "1,2,3,, 4 .";
        deque<rcstring> q;
        rsplit(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( 6 == q.size() );
        REQUIRE( ntba("") == q[0] );
        REQUIRE( ntba(" 4 ") == q[1] );
        REQUIRE( ntba("") == q[2] );
        REQUIRE( ntba("3") == q[3] );
        REQUIRE( ntba("2") == q[4] );
        REQUIRE( ntba("1") == q[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",1,2,3,, 4 .,";
        deque<rcstring> q = rsplit(ntba(s), delim);

        REQUIRE( 8 == q.size() );
        REQUIRE( ntba("") == q[0] );
        REQUIRE( ntba("") == q[1] );
        REQUIRE( ntba(" 4 ") == q[2] );
        REQUIRE( ntba("") == q[3] );
        REQUIRE( ntba("3") == q[4] );
        REQUIRE( ntba("2") == q[5] );
        REQUIRE( ntba("1") == q[6] );
        REQUIRE( ntba("") == q[7] );
        REQUIRE( "_._1_._2_._3_.__._ 4 _.__._" == rjoin<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = "1,2,3,, 4 .";
        rcstring buf[6];
        size_t res = rsplit(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba("") == buf[0] );
        REQUIRE( ntba(" 4 ") == buf[1] );
        REQUIRE( ntba("1,2,3,") == buf[2] );
        REQUIRE( "1,2,3,_._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit(ntba(s), delim, &buf[0], 6);

        REQUIRE( 6 == res );
        REQUIRE( ntba("") == buf[0] );
        REQUIRE( ntba(" 4 ") == buf[1] );
        REQUIRE( ntba("") == buf[2] );
        REQUIRE( ntba("3") == buf[3] );
        REQUIRE( ntba("2") == buf[4] );
        REQUIRE( ntba("1") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit(ntba(s), delim, &buf[0], 7);

        REQUIRE( 6 == res );
        REQUIRE( ntba("") == buf[0] );
        REQUIRE( ntba(" 4 ") == buf[1] );
        REQUIRE( ntba("") == buf[2] );
        REQUIRE( ntba("3") == buf[3] );
        REQUIRE( ntba("2") == buf[4] );
        REQUIRE( ntba("1") == buf[5] );
        REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = "1,2,3,, 4 .";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = rsplit(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba("") == buf[0] );
            REQUIRE( ntba(" 4 ") == buf[1] );
            REQUIRE( ntba("1,2,3,") == buf[2] );
            REQUIRE( "1,2,3,_._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[6];
            size_t res = rsplit(ntba(s), delim, buf);

            REQUIRE( 6 == res );
            REQUIRE( ntba("") == buf[0] );
            REQUIRE( ntba(" 4 ") == buf[1] );
            REQUIRE( ntba("") == buf[2] );
            REQUIRE( ntba("3") == buf[3] );
            REQUIRE( ntba("2") == buf[4] );
            REQUIRE( ntba("1") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[7];
            size_t res = rsplit(ntba(s), delim, buf);

            REQUIRE( ntba("") == buf[0] );
            REQUIRE( ntba(" 4 ") == buf[1] );
            REQUIRE( ntba("") == buf[2] );
            REQUIRE( ntba("3") == buf[3] );
            REQUIRE( ntba("2") == buf[4] );
            REQUIRE( ntba("1") == buf[5] );
            REQUIRE( "1_._2_._3_.__._ 4 _._" == rjoin<string>(buf, buf + res, ntba(jdelim)) );
        }
    }

    // same but with 'skip_empty_t'

    SECTION("")
    {   // unbounded split with functor
        char const s[] = ",1,,,2,3,, 4 .,";
        deque<rcstring> q;
        rsplit_se(ntba(s), delim, [&q](auto v) { q.push_back(v); return false; });

        REQUIRE( 4 == q.size() );
        REQUIRE( ntba(" 4 ") == q[0] );
        REQUIRE( ntba("3") == q[1] );
        REQUIRE( ntba("2") == q[2] );
        REQUIRE( ntba("1") == q[3] );
        REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // unbounded split
        char const s[] = ",,,1,,,2,3,, 4 .,,,";
        deque<rcstring> q = rsplit_se(ntba(s), delim);

        REQUIRE( 4 == q.size() );
        REQUIRE( ntba(" 4 ") == q[0] );
        REQUIRE( ntba("3") == q[1] );
        REQUIRE( ntba("2") == q[2] );
        REQUIRE( ntba("1") == q[3] );
        REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(begin(q), end(q), ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with dynamic buffer
        char const s[] = ",1,,,2,3,, 4 .,";
        rcstring buf[6];
        size_t res = rsplit_se(ntba(s), delim, &buf[0], 3);

        REQUIRE( 3 == res );
        REQUIRE( ntba(" 4 ") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba(",1,,,2") == buf[2] );
        REQUIRE( ",1,,,2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit_se(ntba(s), delim, &buf[0], 4);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 ") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba(",1") == buf[3] );
        REQUIRE( ",1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );

        res = rsplit_se(ntba(s), delim, &buf[0], 5);

        REQUIRE( 4 == res );
        REQUIRE( ntba(" 4 ") == buf[0] );
        REQUIRE( ntba("3") == buf[1] );
        REQUIRE( ntba("2") == buf[2] );
        REQUIRE( ntba("1") == buf[3] );
        REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
    }

    SECTION("")
    {   // bounded split with static buffer
        char const s[] = ",1,,,2,3,, 4 .,,,";

        SECTION("")
        {
            rcstring buf[3];
            size_t res = rsplit_se(ntba(s), delim, buf);

            REQUIRE( 3 == res );
            REQUIRE( ntba(" 4 ") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba(",1,,,2") == buf[2] );
            REQUIRE( ",1,,,2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[4];
            size_t res = rsplit_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba(" 4 ") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba("2") == buf[2] );
            REQUIRE( ntba(",1") == buf[3] );
            REQUIRE( ",1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );

            buf[2].len = 0;
            REQUIRE( ",1_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
        }

        SECTION("")
        {
            rcstring buf[5];
            size_t res = rsplit_se(ntba(s), delim, buf);

            REQUIRE( 4 == res );
            REQUIRE( ntba(" 4 ") == buf[0] );
            REQUIRE( ntba("3") == buf[1] );
            REQUIRE( ntba("2") == buf[2] );
            REQUIRE( ntba("1") == buf[3] );
            REQUIRE( "1_._2_._3_._ 4 " == rjoin_se<string>(buf, buf + res, ntba(jdelim)) );
        }
    }
}
