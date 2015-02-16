#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()      \
  do {                                \
    builtin_define_std ("acess2");      \
    builtin_define_std ("unix");      \
    builtin_assert ("system=acess2");   \
    builtin_assert ("system=unix");   \
    builtin_assert ("system=posix");   \
  } while(0);

#define LIB_SPEC	"-lc -lld-acess -lposix %{pthread:-lpthread}"
#define LIBSTDCXX "c++"
#undef STARTFILE_SPEC
#undef ENDFILE_SPEC
#define STARTFILE_SPEC	"crti.o%s %{static:crtbeginT.o%s;shared|pie:crtbeginS.o%s;:crtbegin.o%s} %{shared:crt0S.o%s;:crt0.o%s}"
#define ENDFILE_SPEC	"%{static:crtendT.o%s;shared|pie:crtendS.o%s;:crtend.o%s} crtn.o%s"
#undef LINK_SPEC
#define LINK_SPEC	"%{shared:-shared} %{!shared:%{!static:%{rdynamic:-export-dynamic}%{!dynamic-linker:-dynamic-linker /Acess/Libs/ld-acess.so}}}"

/*
#undef TARGET_VERSION                                     // note that adding these two lines cause an error in gcc-4.7.0
#define TARGET_VERSION fprintf(stderr, " (i386 acess2)"); // the build process works fine without them until someone can work out an alternative
*/

