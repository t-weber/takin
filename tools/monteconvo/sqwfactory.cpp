/**
 * factory and plugin interface for S(q,w) models
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2016 -- 2018
 * @license GPLv2
 */

#include "sqwfactory.h"
#include "sqw.h"

#if !defined(NO_PY) || defined(USE_JL)
	#include "sqw_proc.h"
	#include "sqw_proc_impl.h"
#endif
#ifndef NO_PY
	#include "sqw_py.h"
#endif
#ifdef USE_JL
	#include "sqw_jl.h"
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
		"4D Nearest-Point Raster of the Form (h, k, l, E, S)" } },
	{ "table_1d", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwTable1d>(strCfgFile.c_str()); },
		"1D Nearest-Point Raster of the Form (q, E, S)" } },
	{ "phonon", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwPhonon>(strCfgFile.c_str()); },
		"Simple Phonon Model" } },
	{ "phonon_single", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwPhononSingleBranch>(strCfgFile.c_str()); },
		"Simple Single-Branch Phonon Model" } },
	{ "magnon", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		{ return std::make_shared<SqwMagnon>(strCfgFile.c_str()); },
		"Simple Magnon Model" } },
#ifndef NO_PY
	{ "py", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		//{ return std::make_shared<SqwPy>(strCfgFile.c_str()); },
		{ return std::make_shared<SqwProc<SqwPy>>(strCfgFile.c_str()); },
		"Python Model" } },
#endif
#ifdef USE_JL
	{ "jl", t_mapSqw::mapped_type {
		[](const std::string& strCfgFile) -> std::shared_ptr<SqwBase>
		//{ return std::make_shared<SqwJl>(strCfgFile.c_str()); },
		{ return std::make_shared<SqwProc<SqwJl>>(strCfgFile.c_str()); },
		"Julia Model" } },
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

namespace so = boost::dll;

// tracking modules for refcounting
static std::vector<std::shared_ptr<so::shared_library>> g_vecMods;

void unload_sqw_plugins()
{
	for(auto& pMod : g_vecMods)
		pMod->unload();
	g_vecMods.clear();
}

void load_sqw_plugins()
{
	static bool bPluginsLoaded = 0;
	if(bPluginsLoaded) return;

	std::vector<std::string> vecPlugins = find_resource_dirs("plugins");
	for(const std::string& strPlugins : vecPlugins)
	{
		tl::log_info("Loading plugins from directory: ", strPlugins, ".");

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

				if(strTakVer != TAKIN_VER)
				{
					tl::log_err("Skipping S(q,w) plugin \"", strPlugin,
					"\" as it was compiled for Takin version ", strTakVer,
					", but this is version ", TAKIN_VER, ".");
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
				tl::log_info("Loaded plugin: ", strPlugin,
					" -> ", strModIdent, " (\"", strModLongName, "\").");
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

void unload_sqw_plugins()
{
}

void load_sqw_plugins()
{
	tl::log_warn("No S(q,w) plugin interface available.");
}

#endif



// ----------------------------------------------------------------------------
// saving and loading of parameters

bool save_sqw_params(const SqwBase* pSqw,
    std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot)
{
	if(!pSqw) return 0;

	const std::vector<SqwBase::t_var> vecVars = pSqw->GetVars();
	const std::vector<SqwBase::t_var_fit>& vecFitVars = pSqw->GetFitVars();

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strVal = std::get<2>(var);

		mapConf[strXmlRoot + "sqw_params/" + strVar] = strVal;
	}

	for(const SqwBase::t_var_fit& var : vecFitVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strErr = std::get<1>(var);
		const bool bFit = std::get<2>(var);

		mapConf[strXmlRoot + "sqw_errors/" + strVar] = strErr;
		mapConf[strXmlRoot + "sqw_fitvar/" + strVar] = bFit ? "1" : "0";
	}

	return 1;
}

bool load_sqw_params(SqwBase* pSqw,
	tl::Prop<std::string>& xml, const std::string& strXmlRoot)
{
	if(!pSqw) return 0;

	std::vector<std::string> vecChildren =
		xml.GetChildNodes(strXmlRoot + "sqw_params/");

	std::vector<SqwBase::t_var> vecVars;
	std::vector<SqwBase::t_var_fit> vecVarsFit;
	vecVars.reserve(vecChildren.size());
	vecVarsFit.reserve(vecChildren.size());

	for(const std::string& strChild : vecChildren)
	{
		boost::optional<std::string> opVal =
			xml.QueryOpt<std::string>(strXmlRoot + "sqw_params/" + strChild);
		if(opVal)
		{
			SqwBase::t_var var;
			std::get<0>(var) = strChild;
			std::get<2>(var) = *opVal;
			vecVars.push_back(std::move(var));
		}

		boost::optional<std::string> opErr =
			xml.QueryOpt<std::string>(strXmlRoot + "sqw_errors/" + strChild);
		boost::optional<bool> opFit =
			xml.QueryOpt<bool>(strXmlRoot + "sqw_fitvar/" + strChild);
		SqwBase::t_var_fit varFit;
		std::get<0>(varFit) = strChild;
		std::get<1>(varFit) = opErr ? *opErr : "0";
		std::get<2>(varFit) = opFit ? *opFit : 0;
		vecVarsFit.push_back(std::move(varFit));
	}

	pSqw->SetVars(vecVars);
	pSqw->SetFitVars(vecVarsFit);

	return 1;
}
// ----------------------------------------------------------------------------
