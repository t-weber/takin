/**
 * Convert monteconvo format to convofit
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date mar-2017
 * @license GPLv2
 */

#ifndef _CONVOFIT_IMPORT_H_
#define _CONVOFIT_IMPORT_H_

#include "tlibs/file/prop.h"
#include <string>


extern std::string convert_monteconvo(
	const tl::Prop<std::string>& propMC, std::string strFile="",
	bool bRelPaths=0);

#endif
