/**
 * mini scan monitor
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015
 * @license GPLv2
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <string>

extern bool open_progress(const std::string& strTitle);
extern void close_progress();
extern void set_progress(int iPerc, const std::string& strTxt);

#endif
