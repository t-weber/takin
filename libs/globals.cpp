/**
 * globals
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 20-mar-2015
 * @license GPLv2
 */

#include "globals.h"
#include "tlibs/log/log.h"
#include "tlibs/file/file.h"
#include "tlibs/string/string.h"
#include <thread>


// -----------------------------------------------------------------------------

unsigned int g_iPrec = 6;
unsigned int g_iPrecGfx = 4;

t_real_glob g_dEps = 1e-6;
t_real_glob g_dEpsGfx = 1e-4;

bool g_bHasElements = 0;
bool g_bHasFormfacts = 0;
bool g_bHasMagFormfacts = 0;
bool g_bHasScatlens = 0;
bool g_bHasSpaceGroups = 0;
bool g_bShowFsq = 1;
bool g_b3dBZ = 1;

std::size_t GFX_NUM_POINTS = 512;
std::size_t g_iMaxNN = 4;

t_real_glob g_dFontSize = 10.;


// -----------------------------------------------------------------------------

unsigned int g_iMaxThreads = std::thread::hardware_concurrency();

unsigned int get_max_threads()
{
	unsigned int iMaxThreads = std::thread::hardware_concurrency();
	return std::min(iMaxThreads, g_iMaxThreads);
}

// -----------------------------------------------------------------------------


static std::vector<std::string> s_vecInstallPaths =
{
	".",
#ifdef INSTALL_PREFIX
	INSTALL_PREFIX "/share/takin",
#endif
};


void add_resource_path(const std::string& strPath, bool bToBack)
{
	if(bToBack)
		s_vecInstallPaths.push_back(strPath);
	else	// insert at beginning, after "."
		s_vecInstallPaths.insert(s_vecInstallPaths.begin()+1, strPath);
}


std::string find_resource(const std::string& strFile, bool bLogErr)
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

	if(bLogErr)
		tl::log_err("Could not find resource file \"", strFile, "\".");
	return "";
}


std::vector<std::string> find_resource_dirs(const std::string& strDir, bool bLogErr)
{
	std::vector<std::string> vecDirs;

	for(const std::string& strPrefix : s_vecInstallPaths)
	{
		std::string _strDir = strPrefix + "/" + strDir;
		if(tl::dir_exists(_strDir.c_str()))
			vecDirs.push_back(_strDir);
	}

	if(bLogErr && vecDirs.size()==0)
		tl::log_err("Could not find resource directory \"", strDir, "\".");

	return vecDirs;
}

// -----------------------------------------------------------------------------
