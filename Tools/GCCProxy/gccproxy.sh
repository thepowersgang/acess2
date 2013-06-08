#!/bin/bash

# Get invocation path (which could be a symlink in $PATH)
fullpath=`which "$0"`
if [[ !$? ]]; then
	fullpath="$0"
fi

# Resolve symlink
fullpath=`readlink -f "$fullpath"`

# Get base directory
BASEDIR=`dirname "$fullpath"`

_miscargs=""
_compile=0
_linktype=Applications

while [[ $# -gt 0 ]]; do
	case "$1" in
	-E)
		_preproc=1
		;;
	-M)
		_makedep=1
		;;
	-c)
		_compile=1
		;;
	-o)
		shift
		_outfile="-o $1"
		;;
	-shared)
		_ldflags=$_ldflags" -shared -lc -lgcc"
		_linktype=Libraries
		;;
	-I|-D|-O)
		_cflags=$_cflags" $1 $2"
		shift
		;;
	-I*|-D*|-O*)
		_cflags=$_cflags" $1"
		;;
	-Wl,*)
		arg=$1
		arg=${arg#-Wl}
		arg=${arg/,/ }
		_ldflags=$_ldflags" ${arg}"
		;;
	-l|-L)
		_libs=$_libs" $1$2"
		shift
		;;
	-l*|-L*)
		_libs=$_libs" $1"
		;;
	-v|--version|-V)
		_verarg=$_verarg" $1"
		;;
	--inv=ld)
		_actas=ld
		;;
	-print-prog-name=ld)
		echo $0 --inv=ld
		exit 0
		;;
	*)
		_miscargs=$_miscargs" $1"
		;;
	esac
	shift
done

run() {
	if [[ "x$GCCPROXY_DEBUG" != "x" ]]; then
		echo --- $*
	fi
	$*
	return $?
}

_ldflags="-lposix -lpsocket "$_ldflags
_cflags=$_cflags" -fno-omit-frame-pointer"

cfgfile=`mktemp`
make --no-print-directory -f $BASEDIR/getconfig.mk ARCH=x86 TYPE=$_linktype > $cfgfile
. $cfgfile
rm $cfgfile

#echo "_compile = $_compile, _preproc = $_preproc"

if [[ "x$_actas" == "xld" ]]; then
	run $_LD $LDFLAGS $_ldflags $_outfile $_miscargs $LIBGCC_PATH $_libs
	exit $?
fi

if [[ $_preproc -eq 1 ]]; then
	run $_CC -E $CFLAGS $_cflags $_miscargs $_outfile
elif [[ $_makedep -eq 1 ]]; then
	run $_CC -M $CFLAGS $_cflags $_miscargs $_outfile
elif [[ $_compile -eq 1 ]]; then
	run $_CC $CFLAGS $_cflags $_miscargs -c $_outfile
elif echo " $_miscargs" | grep '\.c' >/dev/null; then
	tmpout=`mktemp acess_gccproxy.XXXXXXXXXX.o --tmpdir`
	run $_CC $CFLAGS $_cflags $_miscargs -c -o $tmpout
	run $_LD $LDFLAGS $_ldflags $_libs $tmpout $_outfile $_libs $LIBGCC_PATH
	_rv=$?
	rm $tmpout
	exit $_rv
else
	run $_LD $_ldflags $_miscargs $_outfile $LDFLAGS $_libs $LIBGCC_PATH
fi

