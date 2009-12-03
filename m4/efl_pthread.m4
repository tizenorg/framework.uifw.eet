dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Macro that check if several ASM instruction sets are available or not.

dnl Usage: EFL_CHECK_EFL_CHECK_PTHREAD([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Call AC_SUBST(EFL_PTHREAD_FLAGS)
dnl Define EFL_HAVE_PTHREAD

AC_DEFUN([EFL_CHECK_PTHREAD],
[

dnl configure option

AC_ARG_ENABLE([pthread],
   [AC_HELP_STRING([--disable-pthread], [enable POSIX threads code @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _efl_enable_pthread="yes"
    else
       _efl_enable_pthread="no"
    fi
   ],
   [_efl_enable_pthread="yes"])

AC_MSG_CHECKING([whether to build POSIX threads code])
AC_MSG_RESULT([${_efl_enable_pthread}])

dnl check if the compiler supports pthreads

_efl_have_pthread="no"

if test "x${_efl_enable_pthread}" = "xyes" ; then
   AC_CHECK_HEADER(pthread.h,
      [_efl_have_pthread="yes"],
      [_efl_have_pthread="no"])
fi

AC_MSG_CHECKING([whether system support POSIX threads])
AC_MSG_RESULT([${_efl_enable_pthread}])

if test "x${_efl_have_pthread}" = "xyes" ; then
   case "$host_os" in
      mingw*)
         EFL_PTHREAD_CFLAGS="-mthreads"
         EFL_PTHREAD_LIBS="-mthreads -lpthreadGC2"
         ;;
      solaris*)
         EFL_PTHREAD_CFLAGS="-mt"
         EFL_PTHREAD_LIBS="-mt"
         ;;
      *)
         EFL_PTHREAD_CFLAGS="-pthread"
         EFL_PTHREAD_LIBS="-pthread"
         ;;
   esac
fi

AC_SUBST(EFL_PTHREAD_CFLAGS)
AC_SUBST(EFL_PTHREAD_LIBS)

if test "x${_efl_have_pthread}" = "xyes" ; then
   AC_DEFINE(EFL_HAVE_PTHREAD, 1, [Define to mention that POSIX threads are supported])
fi

if test "x${_efl_have_pthread}" = "xyes" ; then
   ifelse([$1], , :, [$1])
else
   ifelse([$2], , :, [$2])
fi
])
