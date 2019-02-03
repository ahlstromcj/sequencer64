"******************************************************************************
"  cpp.vim
"------------------------------------------------------------------------------
"
"  Language:      C/C++
"  Maintainer:    Chris Ahlstrom <ahlstromcj@users.sourceforge.net>
"  Last Change:   2006-09-04 to 2019-02-03
"  Project:       XPC Suite library project
"  Usage:
"
"     This file is a Vim syntax add-on file used in addition to the installed
"     version of the cpp.vim file provided by vim.
"
"     This file is similar to c.vim, but for C++ code.  It adds
"     keywords and syntax highlighting useful to vim users.  Please note that
"     all of the keywords in c.vim also apply to C++ code, so that they
"     do not need to be repeated here.
"
"     Do a ":set runtimepath" command in vim to see what it has in it.  The
"     first entry is usually "~/.vim", so create that directory, then add an
"     "after" and "syntax" directory, so that you end up with this directory:
"
"              ~/.vim/after/syntax
"
"     Then copy the present file (cpp.vim) to 
"
"              ~/.vim/after/syntax/cpp.vim
"
"     Verify that your code now highlights the following symbols when edited.
"
"------------------------------------------------------------------------------

"------------------------------------------------------------------------------
" Our type definitions for new classes and types added by the XPCC++ library
"------------------------------------------------------------------------------

syn keyword XPCC midistring seq64 seq66

"------------------------------------------------------------------------------
" Our type definition for inside comments
"------------------------------------------------------------------------------

syn keyword cTodo contained cpp hpp CPP HPP krufty

"------------------------------------------------------------------------------
" Our Doxygen aliases to highlight inside of comments
"------------------------------------------------------------------------------

syn keyword cTodo contained constructor copyctor ctor
syn keyword cTodo contained defaultctor destructor dtor operator paop paoperator
syn keyword cTodo contained pure singleton virtual

"------------------------------------------------------------------------------
" Our type definitions that are basically standard C++
"------------------------------------------------------------------------------

syn keyword cType auto_ptr bad_alloc begin c_str cbegin cend clear const_iterator
syn keyword cType const_reverse_iterator
syn keyword cType empty end erase exception find first fstream future
syn keyword cType ifstream insert istream istringstream iterator
syn keyword cType length list make_pair map multimap
syn keyword cType ofstream ostream ostringstream pair promise reverse_iterator
syn keyword cType second set shared_ptr size size_type stack std string
syn keyword cType stringstream
syn keyword cType thread unique_ptr value_type vector wstring

"------------------------------------------------------------------------------
" Operators, language constants, or manipulators
"------------------------------------------------------------------------------

syn keyword cppOperator cin cout cerr endl hex left nothrow new npos
syn keyword cppOperator right setw

"------------------------------------------------------------------------------
" Less common C data typedefs
"------------------------------------------------------------------------------

syn keyword cType my_data_t

"------------------------------------------------------------------------------
" Our slough of macros
"------------------------------------------------------------------------------

syn keyword cConstant xxxxxxx
syn keyword cDefine SCUZZGOZIO

"------------------------------------------------------------------------------
" cpp.vim
"------------------------------------------------------------------------------
" vim: ts=3 sw=3 et ft=vim
"------------------------------------------------------------------------------
