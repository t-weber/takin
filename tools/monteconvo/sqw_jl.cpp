/**
 * S(Q,w) julia interface
 * @author tweber
 * @date dec-2016
 * @license GPLv2
 */

#include "sqw_jl.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/file/file.h"
#include <julia.h>

using t_real = t_real_reso;

#define MAX_PARAM_VAL_SIZE 128

SqwJl::SqwJl(const char* pcFile) : m_pmtx(std::make_shared<std::mutex>())
{
	if(!tl::file_exists(pcFile))
	{
		tl::log_err("Could not find Julia script file: \"", pcFile, "\".");
		m_bOk = 0;
		return;
	}

	std::string strFile = pcFile;
	std::string strDir = tl::get_dir(strFile);
	const bool bSetScriptCWD = 1;

	// init interpreter
	static bool bInited = 0;
	if(!bInited)
	{
		jl_init(0);
		std::string strJl = jl_ver_string();
		tl::log_debug("Initialised Julia interpreter version ", strJl, ".");
		bInited = 1;
	}

	// include module
	jl_function_t *pInc = jl_get_function(jl_base_module, "include");
	jl_value_t *pMod = jl_cstr_to_string(pcFile);
	jl_call1(pInc, pMod);

	// working dir
	if(bSetScriptCWD)
	{
		jl_function_t *pCwd = jl_get_function(jl_base_module, "cd");
		jl_value_t *pDir = jl_cstr_to_string(strDir.c_str());
		jl_call1(pCwd, pDir);
	}

	// import takin functions
	m_pInit = jl_get_function(jl_main_module, "TakinInit");
	m_pSqw = jl_get_function(jl_main_module, "TakinSqw");
	m_pDisp = jl_get_function(jl_main_module, "TakinDisp");

	PrintExceptions();

	if(!m_pSqw)
	{
		m_bOk = 0;
		tl::log_err("Julia script has no TakinSqw function.");
		return;
	}
	else
	{
		m_bOk = 1;
	}

	if(!m_pDisp)
		tl::log_warn("Julia script has no TakinDisp function.");

	if(m_pInit)
		jl_call0((jl_function_t*)m_pInit);
	else
		tl::log_warn("Julia script has no TakinInit function.");
}

SqwJl::~SqwJl()
{
	//jl_atexit_hook(0);
}


/**
 * Checks for and prints possible exceptions
 */
void SqwJl::PrintExceptions() const
{
	jl_value_t* pEx = jl_exception_occurred();
	if(pEx)
	{
		std::string strEx;
		jl_value_t *pExStr = nullptr;

		jl_function_t *pPrint = jl_get_function(jl_base_module, "string");
		if(pPrint)
			pExStr = jl_call1(pPrint, pEx);
		if(pExStr)
			strEx = jl_string_ptr(pExStr);

		if(strEx != "")
			tl::log_err("Julia error: ", strEx, ".");
		else
			tl::log_err("Unknown Julia error occurred.");
	}

	jl_exception_clear();
}


/**
 * E(Q)
 */
std::tuple<std::vector<t_real>, std::vector<t_real>>
	SqwJl::disp(t_real dh, t_real dk, t_real dl) const
{
	if(!m_bOk)
	{
		tl::log_err("Julia interpreter has not initialised, cannot query S(q,w).");
		return std::make_tuple(std::vector<t_real>(), std::vector<t_real>());
	}

	std::lock_guard<std::mutex> lock(*m_pmtx);
	std::vector<t_real> vecE, vecW;

	if(m_pDisp)
	{
		jl_value_t *phkl[3] = { jl_box_float64(dh), jl_box_float64(dk), jl_box_float64(dl) };
		jl_array_t *pEW = reinterpret_cast<jl_array_t*>(jl_call((jl_function_t*)m_pDisp, phkl, 3));

		if(jl_array_len(pEW) != 2)
		{
			tl::log_err("TakinDisp has to return [arrEnergies, arrWeights].");
			return std::make_tuple(vecE, vecW);
		}

		jl_array_t *parrE = reinterpret_cast<jl_array_t*>(jl_arrayref(pEW, 0));
		jl_array_t *parrW = reinterpret_cast<jl_array_t*>(jl_arrayref(pEW, 1));

		std::size_t iSizeE = jl_array_len(parrE);
		std::size_t iSizeW = jl_array_len(parrW);

		if(iSizeE != iSizeW)
			tl::log_warn("Size mismatch between energies and weights array in Julia script.");

		for(std::size_t iElem=0; iElem<std::min(iSizeE, iSizeW); ++iElem)
		{
			t_real dE = jl_unbox_float64(jl_arrayref(parrE, iElem));
			t_real dW = jl_unbox_float64(jl_arrayref(parrE, iElem));

			vecE.push_back(dE);
			vecW.push_back(dW);
		}
	}

	PrintExceptions();
	return std::make_tuple(vecE, vecW);
}


/**
 * S(Q,E)
 */
t_real SqwJl::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	if(!m_bOk)
	{
		tl::log_err("Julia interpreter has not initialised, cannot query S(q,w).");
		return t_real(0);
	}

	std::lock_guard<std::mutex> lock(*m_pmtx);

	jl_value_t *phklE[4] =
		{ jl_box_float64(dh), jl_box_float64(dk),
		jl_box_float64(dl), jl_box_float64(dE) };
	jl_value_t *pSqw = jl_call((jl_function_t*)m_pSqw, phklE, 4);

	PrintExceptions();
	return t_real(jl_unbox_float64(pSqw));
}


std::vector<SqwBase::t_var> SqwJl::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;
	if(!m_bOk)
	{
		tl::log_err("Julia interpreter has not initialised, cannot get variables.");
		return vecVars;
	}

	jl_function_t *pNames = jl_get_function(jl_base_module, "names");
	jl_function_t *pGetField = jl_get_function(jl_base_module, "getfield");
	jl_function_t *pPrint = jl_get_function(jl_base_module, "string");

	if(!pNames || !pGetField || !pPrint)
	{
		tl::log_err("Required Julia functions not available.");
		return vecVars;
	}

	jl_array_t* pArrNames = (jl_array_t*)jl_call1(pNames, (jl_value_t*)jl_main_module);
	if(!pArrNames)
		return vecVars;

	std::size_t iSyms = jl_array_len(pArrNames);
	for(std::size_t iSym=0; iSym<iSyms; ++iSym)
	{
		jl_sym_t* pSym = (jl_sym_t*)jl_array_ptr_ref(pArrNames, iSym);
		if(!pSym) continue;

		// name
		std::string strName = jl_symbol_name(pSym);
		if(strName.length() == 0) continue;

		// filter out non-prefixed variables
		if(m_strVarPrefix.size() && strName.substr(0,m_strVarPrefix.size()) != m_strVarPrefix)
			continue;

		// type
		jl_value_t* pFld = jl_call2(pGetField, (jl_value_t*)jl_main_module, (jl_value_t*)pSym);
		if(!pFld) continue;
		std::string strType = jl_typeof_str(pFld);
		if(strType.length() == 0) continue;
		if(strType[0] == '#' || strType == "Module") continue;	// filter funcs and mods

		// value
		jl_value_t* pFldPr = jl_call1(pPrint, pFld);
		if(!pFldPr) continue;
		std::string strValue = jl_string_ptr(pFldPr);

		SqwBase::t_var var;
		std::get<0>(var) = std::move(strName);
		std::get<1>(var) = std::move(strType);
		std::get<2>(var) = std::move(strValue);

		vecVars.push_back(var);
	}

	PrintExceptions();
	return vecVars;
}


void SqwJl::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(!m_bOk)
	{
		tl::log_err("Julia interpreter has not initialised, cannot set variables.");
		return;
	}

	std::ostringstream ostrEval;
	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strName = std::get<0>(var);
		const std::string& strType = std::get<1>(var);
		const std::string& strValue = std::get<2>(var);

		if(!strName.length()) continue;

		ostrEval << strName << " = ";

		if(strType.length())
		{
			// if a type is given, filter out some names
			if(strType[0] == '#' || strType == "Module")
				continue;

			// with cast
			ostrEval << strType << "(" << strValue << ");\n";
		}
		else
		{
			//without cast
			ostrEval << strValue << ";\n";
		}
	}
	jl_eval_string(ostrEval.str().c_str());

	PrintExceptions();
}


SqwBase* SqwJl::shallow_copy() const
{
	SqwJl* pSqw = new SqwJl();
	*static_cast<SqwBase*>(pSqw) = *static_cast<const SqwBase*>(this);

	pSqw->m_pInit = this->m_pInit;
	pSqw->m_pSqw = this->m_pSqw;
	pSqw->m_pDisp = this->m_pDisp;
	pSqw->m_pmtx = this->m_pmtx;

	return pSqw;
}
