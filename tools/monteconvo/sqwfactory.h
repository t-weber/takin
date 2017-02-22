/**
 * factory and plugin interface for S(q,w) models
 * @author Tobias Weber
 * @date 2016
 * @license GPLv2
 */

#ifndef __SQW_FACT_H__
#define __SQW_FACT_H__

#include "sqwbase.h"
#include "tlibs/file/prop.h"

#include <tuple>
#include <memory>
#include <vector>
#include <string>


extern std::shared_ptr<SqwBase> construct_sqw(const std::string& strName,
	const std::string& strConfigFile);

// [identifier, long name]
extern std::vector<std::tuple<std::string, std::string>> get_sqw_names();


extern void unload_sqw_plugins();
extern void load_sqw_plugins();


// ----------------------------------------------------------------------------
// saving and loading of parameters

extern bool save_sqw_params(const SqwBase* pSqw,
	std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot);

extern bool load_sqw_params(SqwBase* pSqw,
	tl::Prop<std::string>& xml, const std::string& strXmlRoot);
// ----------------------------------------------------------------------------

#endif
