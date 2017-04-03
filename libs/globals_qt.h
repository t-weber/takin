/**
 * globals
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 20-mar-2015
 * @license GPLv2
 */

#ifndef __TAKIN_GLOBALS_QT_H__
#define __TAKIN_GLOBALS_QT_H__

#include <string>

#include <QIcon>
#include <QPixmap>
#include <QFont>


extern QIcon load_icon(const std::string& strIcon);
extern QPixmap load_pixmap(const std::string& strPixmap);

extern QFont g_fontGen, g_fontGfx/*, g_fontGL*/;
extern std::string g_strFontGL;
extern int g_iFontGLSize;


#endif
