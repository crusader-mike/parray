# parray\<T, Tr\>

This is a lightweight class representing raw pointer to an array of elements of type T. It is essentially a (length, T*) pair.
Trait (Tr) contains array comparison logic. Originally this class was designed to be used with _char_ and meant to solve problems C++ has with string handling, but I've generalized it for any T.

Note that since this code is freshly rewritten -- it may potentially have some problems. Please, let me know if you find something.

# What is wrong with string handling in C++?

## C-style NTBS (nul-terminated byte string)

This is a contiguous arrays of _char_ located somewhere in memory terminated by '\\0'. C++ inherited it from C, there is a language-level support both in syntax (string literals have implicit '\\0') and library (_strcmp()_ and etc). They are passed around in form of a pointer to first element (_char*_ or _char const*_) and language provides convenient conversion (array-to-pointer decay).

Most obvious problem with NTBS is loss of (often known at compile time) length -- to compare two strings we need to dereference string pointer and compare arrays element-by-element until we reach conclusion or one of terminating nuls (i.e. perform lexicographical comparison). 

But do you really need to perform a memory scan to tell if given two strings are not equal? No! If you know their length differ that means strings are not equal. Consider this -- in general case you could get an answer to (s1 != s2) question without dereferencing string pointer. On modern architectures avoiding a memory access is a big win.

Now, how often do you really need to use lexicographical comparison? In my experience -- very rarely. All these sets and dictionaries you create in memory during typical coding will work just fine if you change your comparison rules to check for length first. So, if s1.len is less than s2.len -- s1 is less than s2, and if s1.len == s2.len we fall back to element-by-element comparison. In this case majority of comparisons can be performed without touching memory containing string elements! This is exactly what default parray trait implements. And in my experience performance gain can be very big... In addition to avoiding unnecessary memory reads we reduce effect pf 'large working set' -- in typical application cache misses increase along with working set size (memory footprint).

Another problem with C-style strings is the fact that all provided tools demand terminating nul. You can't read text file into memory, find a substring that interest you and process it in some way -- you have to copy it into another location, append nul and make sure you don't forget about freeing it later. Besides obvious performance loss it is inconvenient -- now you have to deal with lifetimes of additional objects.

And finally, array-to-pointer decay is a constant PITA, especially if you write generic code. The fact that arrays are second-class citizens in C++ often gets in a way, makes type system unnecessarily complicated.

## std::basic_string\<T, Tr, A\>

You might say that std::basic_string fixes problems mentioned in previous section. Well, surprise! It doesn't...

If you parse a text input you will still end up allocating memory and copying data to hold your tokens even though you never meant to change them.

Even though (in C++03) standard did not require data 'under the hood' to be nul-terminated (hence difference between _data()_ and _c_str()_ methods) -- it strongly encouraged it and every implementation I saw did it. One of the side effects is that every time you append a character to a string -- you end up writing two characters. Makes absolutely no sense if you do this 1000 times in a row -- you end up with 999 completely unnecessary 'write 0' ops. Another side effect is that _c_str()_ can't be declared _nothrow_ (until C++11).

Typical implementation of std::basic_string consists of string length and contiguous block of memory (ignoring _ropes_ here) containing string data (characters). Sometimes lengths is stored along the data, sometimes it is stored on string class along with pointer to data. Most popular implementations also use _COW_ and/or _SSO_ (just google it).

You might think that they'll use string length when figuring out if s1 != s2... Wrong! Long ago someone decided that it would be a nice idea to allow std::basic_string to contain embedded nul (i.e. "abc\\0def"), another good idea was to maintain backward compatibility with C-style strings, i.e. if strcmp(s1, s2) != 0 then std::string(s1) != std::string(s2). As result even if s1.size() != s2.size() underlying implementation of comparison operator will call strcmp() which will scan memory with related consequences.

One more thing -- _COW_ price goes up with number of CPUs in your system. Because every time you copy it, it has to use atomic operations which get more expensive with number of CPUs. That is assuming you compiled your code with MT enabled (which is typical). And price of _SSO_ is extra branch in every piece of logic that accesses underlying data.

Overall, std::basic_string is a good example of how good intentions could lead to mediocre results. While you might be able to dig trenches and cut down trees with a swiss knife -- shovel and axe will do the job much better. :-)

# So, why parray?

Well, parray was created to address a problem I had encountered many times -- application logic that receives a text (e.g. xml), parses it and extracts information it is looking for. First of all, there was absolutely no need to create myriads of these std::strings for every xml tag/value -- original text was staying in memory for duration of entire operation. Second, in order to find what it needs the logic was performing a lot of comparisons with string literals (e.g. _if (cnode->name == "abcde") ..._) -- it was 24 hours a day scanning the same string literals over and over even though lengths are known at compile time and no data involved could have embedded nuls. Needless to say that after I updated logic to use parrays performance increased quite dramatically. I imagine you could use parray in similar cases.

On typical hardware parray has the size of two pointers (two CPU words). It is cheap to pass by value (yay! less typing!). Last time I tested it (C++03 version) it was (very marginally) better to pass it by reference in MSVC. With GCC (on IA-32) there was absolutely no difference -- for some reason GCC was passing parray twice (via stack and registers, see https://gcc.gnu.org/ml/gcc-bugs/2011-10/msg01742.html).

Compared to C-style string you end up passing twice more data around (pointer and length), but benefits greatly outweigh the price (and nothing prevents you from passing parray by reference anyway).

# parray_tools.h

Defines few function families designed to be used with parray. Namely:
- trim() -- to get rid of whitespaces
- contains() -- to figure out if given array is a subarray of another
- split() -- to split array into subarrays according to delimiter
- join() -- to combine arrays into one using various delimiters

all functions are self-explanatory and well-documented in the code.

# Examples of usage
