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
