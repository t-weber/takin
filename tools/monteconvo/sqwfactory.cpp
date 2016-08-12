/**
 * factory and plugin interface for S(q,w) models
 * @author Tobias Weber
 * @date 2016
 * @license GPLv2
 */

#include "sqwfactory.h"
#include "sqw.h"
#ifndef NO_PY
	#include "sqw_py.h"
#endif

#include "tlibs/log/log.h"
#include "tlibs/file/file.h"
#include "libs/globals.h"
#include "libs/version.h"

#include <algorithm>
#include <functional>
#include <unordered_map>


// sqw info function: "takin_sqw_info"
// returns: [takin ver, ident, long name]
using t_pfkt_info = std::tuple<std::string, std::string, std::string>(*)();
using t_fkt_info = typename std::remove_pointer<t_pfkt_info>::type;

// sqw module function: "takin_sqw"
using t_pfkt = std::shared_ptr<SqwBase>(*)(const std::string&);
using t_fkt = typename std::remove_pointer<t_pfkt>::type;

// key: identifier, value: [func, long name]
using t_mapSqw = std::unordered_map<std::string, std::tuple<t_pfkt, std::string>>;

static t_mapSqw g_mapSqw =
{
	{ "kd", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwKdTree>(strCfgFile.c_str()); },
		"Table" } },
	{ "phonon", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwPhonon>(strCfgFile.c_str()); }, 
		"Simple Phonon Model" } },
	{ "magnon", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwMagnon>(strCfgFile.c_str()); }, 
		"Simple Magnon Model" } },
#ifndef NO_PY
	{ "py", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwPy>(strCfgFile.c_str()); }, 
		"Python Model" } },
#endif
	{ "elastic", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwElast>(strCfgFile.c_str()); }, 
		"Elastic Model" } },
};



std::vector<std::tuple<std::string, std::string>> get_sqw_names()
{
	using t_tup = std::tuple<std::string, std::string>;
	std::vector<t_tup> vec;
	vec.reserve(g_mapSqw.size());

	for(const t_mapSqw::value_type& val : g_mapSqw)
	{
		t_tup tup;
		std::get<0>(tup) = val.first;
		std::get<1>(tup) = std::get<1>(val.second);

		vec.push_back(std::move(tup));
	}

	std::sort(vec.begin(), vec.end(), [](const t_tup& tup0, const t_tup& tup1) -> bool
	{
		const std::string& str0 = std::get<1>(tup0);
		const std::string& str1 = std::get<1>(tup1);

		return std::lexicographical_compare(str0.begin(), str0.end(), str1.begin(), str1.end());
	});
	return vec;
}

std::shared_ptr<SqwBase> construct_sqw(const std::string& strName,
	const std::string& strConfigFile)
{
	typename t_mapSqw::const_iterator iter = g_mapSqw.find(strName);
	if(iter == g_mapSqw.end())
	{
		tl::log_err("No S(q,w) model of name \"", strName, "\" found.");
		return nullptr;
	}

	//tl::log_debug("Constructing ", iter->first, ".");
	return (*std::get<0>(iter->second))(strConfigFile);
}


#ifdef USE_PLUGINS

#include <boost/dll/shared_library.hpp>
#include <boost/dll/import.hpp>

void load_sqw_plugins()
{
	namespace so = boost::dll;
	// tracking modules for refcounting
	static std::vector<std::shared_ptr<so::shared_library>> g_vecMods;

	static bool bPluginsLoaded = 0;
	if(bPluginsLoaded) return;

	std::string strPlugins = find_resource_dir("plugins");
	if(strPlugins != "")
	{
		tl::log_info("Plugin directory: ", strPlugins);

		std::vector<std::string> vecPlugins = tl::get_all_files(strPlugins.c_str());
		for(const std::string& strPlugin : vecPlugins)
		{
			try
			{
				std::shared_ptr<so::shared_library> pmod =
					std::make_shared<so::shared_library>(strPlugin);
				if(!pmod) continue;

				// import info function
				std::function<t_fkt_info> fktInfo =
					pmod->get<t_pfkt_info>("takin_sqw_info");
				if(!fktInfo) continue;

				auto tupInfo = fktInfo();
				const std::string& strTakVer = std::get<0>(tupInfo);
				const std::string& strModIdent = std::get<1>(tupInfo);
				const std::string& strModLongName = std::get<2>(tupInfo);
				tl::log_debug("Module ident: ", strModIdent);

				if(strTakVer != TAKIN_VER)
				{
					tl::log_err("Skipping S(q,w) plugin \"", strPlugin,
					"\" as it was compiled for Takin version ", strTakVer,
					" but this is version ", TAKIN_VER, ".");
					continue;
				}


				// import factory function
				t_pfkt pFkt = pmod->get<t_pfkt>("takin_sqw");
				if(!pFkt) continue;

				g_mapSqw.insert( t_mapSqw::value_type {
					strModIdent,
					t_mapSqw::mapped_type { pFkt, strModLongName }
				});


				g_vecMods.push_back(pmod);
				tl::log_info("Loaded plugin: ", strPlugin);
			}
			catch(const std::exception& ex)
			{
				//tl::log_err("Could not load ", strPlugin, ". Reason: ", ex.what());
			}
		}
	}

	bPluginsLoaded = 1;
}

#else

void load_sqw_plugins()
{
	tl::log_warn("No S(q,w) plugin interface available.");
}

#endif
