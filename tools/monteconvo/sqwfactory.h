/**
 * factory and plugin interface for S(q,w) models
 * @author Tobias Weber
 * @date 2016
 * @license GPLv2
 */

#ifndef __SQW_FACT_H__
#define __SQW_FACT_H__

#include "sqwbase.h"
#include <tuple>
#include <memory>
#include <vector>
#include <string>


extern std::shared_ptr<SqwBase> construct_sqw(const std::string& strName,
	const std::string& strConfigFile);

// [identifier, long name]
extern std::vector<std::tuple<std::string, std::string>> get_sqw_names();


extern void load_sqw_plugins();
#endif
