dnl *********** rpcgen -C ****************
define(AC_RPCGEN_C,
  [AC_MSG_CHECKING(for rpcgen -C)
  AC_CACHE_VAL(poco_cv_prog_RPCGEN_C,
  [if $RPCGEN -C -c </dev/null >/dev/null 2>/dev/null
  then
    poco_cv_prog_RPCGEN_C=yes
  else
    poco_cv_prog_RPCGEN_C=no
  fi])dnl
  AC_MSG_RESULT($poco_cv_prog_RPCGEN_C)
  if test $poco_cv_prog_RPCGEN_C = yes; then
    RPCGEN_C=-C
  fi
  AC_SUBST(RPCGEN_C)
  test -n "$RPCGEN_C" && AC_DEFINE(HAVE_RPCGEN_C)
])dnl


dnl --- look for RTAI includes ------------------------------------------
dnl AC_CHECK_RTAI_INCLUDES(var, path)
AC_DEFUN([AC_CHECK_RTAI_INCLUDES],
[
   AC_MSG_CHECKING([for RTAI includes])
   AC_CACHE_VAL(ac_cv_path_rtai,
    [
       IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
        ac_tmppath="$2:/usr/realtime/include:/usr/src/rtai/include"
        for ac_dir in $ac_tmppath; do 
            test -z "$ac_dir" && ac_dir=.
            if eval test -f $ac_dir/rtai.h; then
               eval ac_cv_path_rtai="$ac_dir"
               break
            fi
       done
       IFS="$ac_save_ifs"
    ])
   $1="$ac_cv_path_rtai"
   if test -n "[$]$1"; then
      AC_MSG_RESULT([$]$1)
   else
      AC_MSG_ERROR([cannot find RTAI includes], 2)
   fi
   AC_SUBST($1)
])


dnl --- look for linux kernel includes ----------------------------------
dnl AC_CHECK_LINUXKERNEL_INCLUDES(var, path)
AC_DEFUN([AC_CHECK_LINUXKERNEL_INCLUDES],
[
   AC_MSG_CHECKING([for linux kernel includes])
   AC_CACHE_VAL(ac_cv_path_kernel,
    [
       IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
        ac_tmppath="$2:/usr/realtime/include:/usr/src/linux/include:/usr/include"
        for ac_dir in $ac_tmppath; do 
            test -z "$ac_dir" && ac_dir=.
            if eval test -f $ac_dir/linux/kernel.h; then
               eval ac_cv_path_kernel="$ac_dir"
               break
            fi
       done
       IFS="$ac_save_ifs"
    ])
   $1="$ac_cv_path_kernel"
   if test -n "[$]$1"; then
      AC_MSG_RESULT([$]$1)
   else
      AC_MSG_ERROR([cannot find linux kernel includes], 2)
   fi
   AC_SUBST($1)
])
