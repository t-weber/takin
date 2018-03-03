/**
 * globals
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 20-mar-2015
 * @license GPLv2
 */

#ifndef __TAKIN_GLOBALS_H__
#define __TAKIN_GLOBALS_H__

#include <vector>
#include <string>


//using t_real_glob = float;
using t_real_glob = double;


extern unsigned int g_iPrec;
extern unsigned int g_iPrecGfx;

extern t_real_glob g_dEps;
extern t_real_glob g_dEpsGfx;

extern unsigned int g_iMaxThreads;

extern std::size_t GFX_NUM_POINTS;
extern std::size_t g_iMaxNN;

extern bool g_bHasElements;
extern bool g_bHasFormfacts;
extern bool g_bHasMagFormfacts;
extern bool g_bHasScatlens;
extern bool g_bHasSpaceGroups;
extern bool g_bShowFsq;
extern bool g_b3dBZ;

extern t_real_glob g_dFontSize;

extern void add_resource_path(const std::string& strPath, bool bToBack=1);
extern std::string find_resource(const std::string& strFile, bool bLogErr=1);
extern std::vector<std::string> find_resource_dirs(const std::string& strDir, bool bLogErr=1);

extern void add_global_path(const std::string& strPath, bool bToBack=1);
extern const std::vector<std::string>& get_global_paths();
extern void clear_global_paths();

extern unsigned int get_max_threads();

#endif
