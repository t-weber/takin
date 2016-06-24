find_path(tlibs_INCLUDE_DIRS
	NAMES version.h
	PATH_SUFFIXES tlibs
	HINTS /usr/include/tlibs /usr/local/include/tlibs /opt/local/include/tlibs
	DOC "tlibs include directory"
)

find_library(tlibs_LIBRARIES
	NAMES libtlibs.so
	HINTS /usr/lib64/ /usr/lib/ /usr/local/lib64 /usr/local/lib /opt/local/lib
	DOC "tlibs library"
)

find_path(tlibs_SHARED_DIRS
	NAMES AUTHORS COPYING.GPLv2 COPYING.GPLv3 LICENSES FindMinuit2.cmake
	HINTS /usr/share/tlibs /usr/local/share/tlibs /opt/local/share/tlibs
	DOC "tlibs shared directory"
)


if(tlibs_LIBRARIES)
	set(tlibs_FOUND 1)
	message("tLibs include directories: ${tlibs_INCLUDE_DIRS}")
	message("tLibs library: ${tlibs_LIBRARIES}")
else()
	set(tlibs_FOUND 0)
	message("Error: tLibs could not be found!")
endif()
