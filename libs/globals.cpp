/**
 * globals
 * @author tweber
 * @date 20-mar-2015
 * @license GPLv2
 */

#include "globals.h"
#include "tlibs/log/log.h"
#include "tlibs/file/file.h"


// -----------------------------------------------------------------------------

unsigned int g_iPrec = 6;
unsigned int g_iPrecGfx = 4;

t_real_glob g_dEps = 1e-6;
t_real_glob g_dEpsGfx = 1e-4;

bool g_bHasFormfacts = 0;
bool g_bHasMagFormfacts = 0;
bool g_bHasScatlens = 0;
bool g_bHasSpaceGroups = 0;
bool g_bShowFsq = 1;

unsigned int GFX_NUM_POINTS = 512;

// -----------------------------------------------------------------------------


static std::vector<std::string> s_vecInstallPaths =
{
	".",
#ifdef INSTALL_PREFIX
	INSTALL_PREFIX "/share/takin",
#endif
};


void add_resource_path(const std::string& strPath)
{
	s_vecInstallPaths.push_back(strPath);
}

std::string find_resource(const std::string& strFile)
{
	for(const std::string& strPrefix : s_vecInstallPaths)
	{
		std::string _strFile = strPrefix + "/" + strFile;
		//tl::log_debug("Looking for file: ", _strFile);
		if(tl::file_exists(_strFile.c_str()))
			return _strFile;
		else if(tl::file_exists((_strFile + ".gz").c_str()))
			return _strFile + ".gz";
		else if(tl::file_exists((_strFile + ".bz2").c_str()))
			return _strFile + ".bz2";
	}

	tl::log_err("Could not load resource file \"", strFile, "\".");
	return "";
}

std::string find_resource_dir(const std::string& strDir)
{
	for(const std::string& strPrefix : s_vecInstallPaths)
	{
		std::string _strDir = strPrefix + "/" + strDir;
		if(tl::dir_exists(_strDir.c_str()))
			return _strDir;
	}

	tl::log_err("Could not load resource directory \"", strDir, "\".");
	return "";
}

// -----------------------------------------------------------------------------
