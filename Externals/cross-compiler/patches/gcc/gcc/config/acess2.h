#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()      \
  do {                                \
    builtin_define_std ("acess2");      \
    builtin_define_std ("unix");      \
    builtin_assert ("system=acess2");   \
    builtin_assert ("system=unix");   \
  } while(0);

#define LIB_SPEC	"-lc -lld-acess"

/*
#undef TARGET_VERSION                                     // note that adding these two lines cause an error in gcc-4.7.0
#define TARGET_VERSION fprintf(stderr, " (i386 acess2)"); // the build process works fine without them until someone can work out an alternative
*/

