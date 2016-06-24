/**
 * globals
 * @author tweber
 * @date 20-mar-2015
 * @license GPLv2
 */

#ifndef __TAKIN_GLOBALS_H__
#define __TAKIN_GLOBALS_H__

#include <string>


//using t_real_glob = float;
using t_real_glob = double;


extern unsigned int g_iPrec;
extern unsigned int g_iPrecGfx;

extern t_real_glob g_dEps;
extern t_real_glob g_dEpsGfx;

extern bool g_bHasFormfacts;
extern bool g_bHasMagFormfacts;
extern bool g_bHasScatlens;
extern bool g_bHasSpaceGroups;
extern bool g_bShowFsq;

extern void add_resource_path(const std::string& strPath);
extern std::string find_resource(const std::string& strFile);
extern std::string find_resource_dir(const std::string& strDir);


#endif
