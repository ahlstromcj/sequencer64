"******************************************************************************
"  c.vim
"------------------------------------------------------------------------------
"
"  Language:      C
"  Maintainer:    Chris Ahlstrom <ahlstromcj@users.sourceforge.net>
"  Last Change:   2006-08-04 to 2019-02-03
"  Project:       XPC Suite library project
"  License:       None.  Use it in any manner whatsover, and don't blame me.
"  Usage:
"
"     This file is a Vim syntax add-on file for c.vim (and cpp.vim) as
"     installed when vim is installed.
"
"     Do a ":set runtimepath" command in vim to see what it has in it.  The
"     first entry is usually "~/.vim", so create that directory, then add an
"     "after" and "syntax" directory, so that you end up with this directory:
"
"              ~/.vim/after/syntax
"
"     Then copy the present file (c.vim) to 
"
"              ~/.vim/after/syntax/c.vim
"
"     Verify that your code now highlights the following symbols.
"
"     Not necessary if the above instructions are followed:
"
"              :runtime ~/.vim/after/syntax/syncolor.vim
"
"  Tip:
"
"     If you want to quickly validate your Doxygen comments, run the following
"     command to highlight them using doxygen.vim in
"     /usr/share/vim/vimfiles/syntax:
"
"              :set syntax=doxygen
"
"  syn keyword XPCC
"
"------------------------------------------------------------------------------

syn keyword XPCC errno errprintfunc infoprintfunc warnprintfunc
syn keyword XPCC strerrprintfunc strerrorprintfunc strerrnoprintfunc

syn keyword XPCC atomic_int_t bool_string bool_to_code debugprint_func
syn keyword XPCC EXTERN_C_DEC EXTERN_C_END find_data_t
syn keyword XPCC find_function_t find_type_t ftw_function_t
syn keyword XPCC is_invalid_handle is_invalid_thread
syn keyword XPCC is_NULL is_nullptr is_nullptr_2 is_nullptr_3 is_nullptr_4
syn keyword XPCC is_nullfunc
syn keyword XPCC is_posix_error is_posix_success is_socket_error is_thisptr
syn keyword XPCC is_valid_handle is_valid_socket is_valid_thread not_thisptr
syn keyword XPCC not_nullptr not_nullptr_2 not_nullptr_3 not_nullptr_4
syn keyword XPCC not_NULL not_nullfunc not_posix_error not_posix_success
syn keyword XPCC not_null_result
syn keyword XPCC not_socket_error not_valid_socket
syn keyword XPCC perf_bits_enum_t perf_cpu_speed_t perf_function_t perf_hyper_t
syn keyword XPCC perf_lock_enum_t perf_os_enum_t perf_processor_t perf_task_enum_t
syn keyword XPCC perf_test_row_t
syn keyword XPCC POSIX_ERROR POSIX_FAILURE POSIX_SUCCESS
syn keyword XPCC xpc xpc_addrinfo_t xpc_always_inline xpc_hidden
syn keyword XPCC xpc_app_arguments_t xpc_closesocket
syn keyword XPCC xpc_cond_mutex xpc_cond_struct xpc_cond_t xpc_critex_struct
syn keyword XPCC xpc_critex_t xpc_error_level_t xpc_filename_change_t
syn keyword XPCC xpc_errprint_func
syn keyword XPCC XPC_PACKAGE XPC_VERSION XPC_API_VERSION
syn keyword XPCC xpc_filename_split_t xpc_filename_t xpc_filename_type_t
syn keyword XPCC XPC_INLINE_CODE
syn keyword XPCC XPC_INT_CAST xpc_invalid_handle XPC_INVALID_HANDLE
syn keyword XPCC XPC_INVALID_SOCKET XPC_INVALID_THREAD XPC_LOCALHOST
syn keyword XPCC XPC_LOCALHOST_NAME xpc_lseek_offset_t xpc_mutex_struct
syn keyword XPCC xpc_mutex_t xpc_pointer_t xpc_recast
syn keyword XPCC xpc_return XPC_REVISION XPC_REVISION_DECL
syn keyword XPCC xpc_ringbuffer_t xpc_seedings_t xpc_semaphore_struct
syn keyword XPCC xpc_semaphore_t xpc_server_socket_T XPC_SHOWINFO
syn keyword XPCC xpc_string_t
syn keyword XPCC xpc_sockaddr_list_t xpc_sockaddr_storage_t xpc_sock_cs_codes_t
syn keyword XPCC XPC_SOCK_DEF_FRAGSIZE XPC_SOCK_DEF_NODELAY XPC_SOCK_DEF_RETRY
syn keyword XPCC xpc_sock_endpoint_t xpc_sock_error_t XPC_SOCKET_ERRNO
syn keyword XPCC XPC_SOCKET_ERROR xpc_socket_t xpc_sockbase_t xpc_sock_flag_t
syn keyword XPCC xpc_sock_mode_t xpc_sock_parameters_t xpc_sock_pending_t
syn keyword XPCC xpc_sock_port_t xpc_sock_state_t xpc_sock_status_t
syn keyword XPCC XPC_SOCK_TIMEOUT_ERROR XPC_SOCK_TIMEOUT_INF XPC_SOCK_TIMEOUT_NONE
syn keyword XPCC xpc_sock_timeout_t xpc_sock_tos_t xpc_statcast
syn keyword XPCC xpc_test_arguments_t xpc_test_function_t xpc_test_row_t
syn keyword XPCC xpc_timeout_t xpc_inline xpc_unused
syn keyword XPCC XPC_VERSION XPC_VERSION_TAG XPC_LIB_VERSION
syn keyword XPCC xpc_widetime_t
syn keyword XPCC xpc_xdr_discrim_t xpc_xdr_enum_t xpc_xdrf_destroy_t
syn keyword XPCC xpc_xdrf_func_t xpc_xdrf_getbyte_t xpc_xdrf_getint32_t
syn keyword XPCC xpc_xdrf_getlong_t xpc_xdrf_getpost_t xpc_xdrf_inline_t
syn keyword XPCC xpc_xdrf_putbyte_t xpc_xdrf_putint32_t xpc_xdrf_putlong_t
syn keyword XPCC xpc_xdrf_setpost_t xpc_xdrf_sock_t xpc_xdr_mem_alloc
syn keyword XPCC xpc_xdr_mem_free xpc_xdr_netobj_t xpc_xdr_ops_t
syn keyword XPCC xpc_xdr_op_t xpc_xdr_t set_invalid_socket
syn keyword XPCC xpc_strerrnoprint_func xpc_strerrprint_func
syn keyword XPCC xpc_infoprint_func xpc_warnprint_func
syn keyword XPCC safe_array_delete safe_ptr_delete safe_pointer_delete thisptr
syn keyword XPCC set_nullptr set_posix_error set_posix_success
syn keyword XPCC bussbyte colorbyte ctrlkey ctrlop
syn keyword XPCC midibool midibooleans midibyte midibyte_t midilong
syn keyword XPCC midipulse midishort midibpm

"------------------------------------------------------------------------------
" More keywords from POSIX itself.  Includes less common C data typedefs and
" GNU keywords.
"------------------------------------------------------------------------------

syn keyword cType __int64
syn keyword cType caddr_t u_char u_short u_int u_long quad_t u_quad_t
syn keyword cType off_t pid_t uid_t socklen_t wchar_t
syn keyword cType pollfd timeval timespec

"------------------------------------------------------------------------------
" Keywords from GNU
"------------------------------------------------------------------------------

syn keyword cType __inline__ __attribute__

"------------------------------------------------------------------------------
" Keywords from POSIX.
"------------------------------------------------------------------------------

syn keyword cType pthread_cond_t pthread_mutex_t sem_t addrinfo sockaddr_in
syn keyword cType sockaddr_in6 sockaddr sockaddr_un sockaddr_nl
syn keyword cType sockaddr_storeage SOCKET in_port_t in_addr_t stat

"------------------------------------------------------------------------------
" Keywords from Windows.
"------------------------------------------------------------------------------

syn keyword cType CRITICAL_SECTION
syn keyword cType HANDLE SOCKET _finddata_t _stat

"------------------------------------------------------------------------------
" Change the Todo coloring to something less obtrusive than black-on-yellow.
"------------------------------------------------------------------------------

highlight todo term=standout cterm=NONE ctermfg=DarkGreen ctermbg=NONE gui=NONE guifg=DarkGreen guibg=Yellow

"------------------------------------------------------------------------------
"  Doxygen tags.
"------------------------------------------------------------------------------
"
"  'sideeffect', 'usage', and many others are ALIASES in doc/doxygen.cfg.
"  The rest are provided by Doxygen or by convention.  They appear only in
"  comments, and so we can use the existing cTodo highlight group.
"
"------------------------------------------------------------------------------

syn keyword cTodo contained FIXME WARNING WTF XXX TBD unknown HUH __func__
syn keyword cTodo contained abc abstract accessor algorithm application
syn keyword cTodo contained assumptions author
syn keyword cTodo contained background bit32 bit64 brief bug
syn keyword cTodo contained callback callgraph case change code class
syn keyword cTodo contained configuration convention
syn keyword cTodo contained contents
syn keyword cTodo contained crossplatform cutnpaste cygwin
syn keyword cTodo contained date debug default deprecated designdoc
syn keyword cTodo contained doxygen dummy endcode
syn keyword cTodo contained endverbatim enum example exception
syn keyword cTodo contained file followup freebsd friend friendof goals
syn keyword cTodo contained gcc getter group gnu hardwired history idea inline
syn keyword cTodo contained image install interface internal
syn keyword cTodo contained library license linux
syn keyword cTodo contained macosx macro mainpage namespace note obsolete
syn keyword cTodo contained op overload page par param posix
syn keyword cTodo contained private protected xpc xpcc XPCC public
syn keyword cTodo contained question recursive return ref references
syn keyword cTodo contained relates relatesalso security
syn keyword cTodo contained section seealso setter sideeffect
syn keyword cTodo contained solaris steps struct subpage subsection
syn keyword cTodo contained subsubsection summary template
syn keyword cTodo contained test tests threadsafe threadunsafe throw todo
syn keyword cTodo contained tip tricky typedef undocumented usecase
syn keyword cTodo contained unittests unix updates usage
syn keyword cTodo contained validator var verbatim version
syn keyword cTodo contained warning win32 win64
syn keyword cTodo contained utility xoperator
syn keyword cTodo contained Revision Chris Ahlstrom

"------------------------------------------------------------------------------
" Our boolean definitions
"------------------------------------------------------------------------------

syn keyword cType cbool_t ubool_t

"------------------------------------------------------------------------------
" Our slough of macros, including a pre C++-TR-1 definition for 'nullptr' used
" in both C and C++, and some underscore macros related to GNU gettext()
" support.
"------------------------------------------------------------------------------

syn keyword cConstant nullptr NULLptr
syn keyword cDefine _ N_ M_

"------------------------------------------------------------------------------
" Additional POSIX/BSD error codes not provided by vim
"------------------------------------------------------------------------------

syn keyword cConstant EHOSTUNREACH ENETUNREACH EADDRNOTAVAIL ECONNREFUSED
syn keyword cConstant ECONNRESET ENOTSOCK ENOTCONN ENOBUFS

"------------------------------------------------------------------------------
" WinSock 2 error codes
"------------------------------------------------------------------------------

syn keyword cConstant WSAENETDOWN WSAEINPROGRESS WSAEADDRNOTAVAIL
syn keyword cConstant WSAECONNREFUSED WSAENETUNREACH

"------------------------------------------------------------------------------
" XPCC socket error codes
"------------------------------------------------------------------------------

syn keyword cDefine XPC_SOCK_ERR_SUCCESS XPC_SOCK_ERR_CREATEFAILED
syn keyword cDefine XPC_SOCK_ERR_COPYFAILED XPC_SOCK_ERR_INPUT
syn keyword cDefine XPC_SOCK_ERR_INPUTINTERRUPT XPC_SOCK_ERR_RESOURCEFAILURE
syn keyword cDefine XPC_SOCK_ERR_OUTPUT XPC_SOCK_ERR_OUTPUTINTERRUPT
syn keyword cDefine XPC_SOCK_ERR_NOTCONNECTED XPC_SOCK_ERR_CONNECTREFUSED
syn keyword cDefine XPC_SOCK_ERR_CONNECTREJECTED
syn keyword cDefine XPC_SOCK_ERR_CONNECTRESETBYPEER
syn keyword cDefine XPC_SOCK_ERR_CONNECTTIMEOUT XPC_SOCK_ERR_CONNECTFAILED
syn keyword cDefine XPC_SOCK_ERR_CONNECTINVALID XPC_SOCK_ERR_CONNECTBUSY
syn keyword cDefine XPC_SOCK_ERR_CONNECTNOROUTE XPC_SOCK_ERR_BINDINGFAILED
syn keyword cDefine XPC_SOCK_ERR_LISTENFAILED XPC_SOCK_ERR_BROADCASTDENIED
syn keyword cDefine XPC_SOCK_ERR_ROUTINGDENIED XPC_SOCK_ERR_KEEPALIVEDENIED
syn keyword cDefine XPC_SOCK_ERR_SERVICEDENIED XPC_SOCK_ERR_SERVICEUNAVAILABLE
syn keyword cDefine XPC_SOCK_ERR_MULTICASTDISABLED XPC_SOCK_ERR_TIMEOUT
syn keyword cDefine XPC_SOCK_ERR_NODELAY XPC_SOCK_ERR_BADDESCRIPTOR
syn keyword cDefine XPC_SOCK_ERR_NOTASOCKET XPC_SOCK_ERR_INVALIDADDRESS
syn keyword cDefine XPC_SOCK_ERR_BLOCKINGMODE XPC_SOCK_ERR_WOULDBLOCK
syn keyword cDefine XPC_SOCK_ERR_OTHER XPC_SOCK_ERR_GENERIC
syn keyword cDefine XPC_SOCK_ERR_OS XPC_SOCK_ERR_EXTENDED

"------------------------------------------------------------------------------
" c.vim
"------------------------------------------------------------------------------
" vim: ts=3 sw=3 et ft=vim
"------------------------------------------------------------------------------
