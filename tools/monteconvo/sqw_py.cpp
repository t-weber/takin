/**
 * S(Q,w) python interface
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2015
 * @license GPLv2
 */

#include "sqw_py.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/file/file.h"

#include <boost/python/stl_iterator.hpp>

using t_real = t_real_reso;

#define MAX_PARAM_VAL_SIZE 128


SqwPy::SqwPy(const char* pcFile) : m_pmtx(std::make_shared<std::mutex>())
{
	if(!tl::file_exists(pcFile))
	{
		tl::log_err("Could not find Python script file: \"", pcFile, "\".");
		m_bOk = 0;
		return;
	}

	std::string strFile = pcFile;
	std::string strDir = tl::get_dir(strFile);
	std::string strMod = tl::get_file_noext(tl::get_file_nodir(strFile));
	const bool bSetScriptCWD = 1;

	try	// mandatory stuff
	{
		static bool bInited = 0;
		if(!bInited)
		{
			::Py_InitializeEx(0);
			std::string strPy = Py_GetVersion();
			tl::find_all_and_replace(strPy, std::string("\n"), std::string(", "));
			tl::log_debug("Initialised Python interpreter version ", strPy, ".");
			bInited = 1;
		}

		// set script paths
		m_sys = py::import("sys");
		py::dict sysdict = py::extract<py::dict>(m_sys.attr("__dict__"));
		py::list path = py::extract<py::list>(sysdict["path"]);
		path.append(strDir.c_str());
		path.append(".");

		if(bSetScriptCWD)
		{
			// set script working directory -> warning: also sets main cwd
			m_os = py::import("os");
			py::dict osdict = py::extract<py::dict>(m_os.attr("__dict__"));
			py::object pycwd = osdict["chdir"];
			if(!!pycwd)
				pycwd(strDir.c_str());
			else
				tl::log_warn("Cannot set python script working directory.");
		}

		// import takin functions
		m_mod = py::import(strMod.c_str());
		if(m_mod.is_none())
		{
			tl::log_err("Invalid Python module \"", strMod, "\".");
			m_bOk = 0;
			return;
		}

		py::dict moddict = py::extract<py::dict>(m_mod.attr("__dict__"));
		m_Sqw = moddict["TakinSqw"];
		m_bOk = !!m_Sqw;
		if(!m_bOk)
			tl::log_err("Python script has no TakinSqw function.");

		try	// optional stuff
		{
			if(moddict.has_key("TakinInit"))
			{
				m_Init = moddict["TakinInit"];
				if(!!m_Init) m_Init();
			}
			else
			{
				tl::log_warn("Python script has no TakinInit function.");
			}

			if(moddict.has_key("TakinDisp"))
				m_disp = moddict["TakinDisp"];
			else
				tl::log_warn("Python script has no TakinDisp function.");
		}
		catch(const py::error_already_set& ex) {}
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();

		m_bOk = 0;
	}
}

SqwPy::~SqwPy()
{
	/*try
	{
		::Py_Finalize();
		tl::log_debug("Uninitialised Python interpreter.");
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();
	}*/
}


/**
 * E(Q)
 */
std::tuple<std::vector<t_real>, std::vector<t_real>>
	SqwPy::disp(t_real dh, t_real dk, t_real dl) const
{
	if(!m_bOk)
	{
		tl::log_err("Interpreter has not initialised, cannot query S(q,w).");
		return std::make_tuple(std::vector<t_real>(), std::vector<t_real>());
	}


	std::lock_guard<std::mutex> lock(*m_pmtx);

	std::vector<t_real> vecEs, vecWs;

	try
	{
		if(!!m_disp)
		{
			py::object lst = m_disp(dh, dk, dl);
			py::stl_input_iterator<py::object> iterLst(lst);
			py::stl_input_iterator<py::object> endLst;

			if(iterLst != endLst)
			{
				py::object _vecE = *iterLst;
				py::stl_input_iterator<t_real> iterE(_vecE);
				py::stl_input_iterator<t_real> endE;

				while(iterE != endE)
					vecEs.push_back(*iterE++);
			}

			if(++iterLst != endLst)
			{
				py::object _vecW = *iterLst;
				py::stl_input_iterator<t_real> iterW(_vecW);
				py::stl_input_iterator<t_real> endW;

				while(iterW != endW)
					vecWs.push_back(*iterW++);
			}
		}
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();
	}

	return std::make_tuple(vecEs, vecWs);
}


/**
 * S(Q,E)
 */
t_real SqwPy::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	if(!m_bOk)
	{
		tl::log_err("Interpreter has not initialised, cannot query S(q,w).");
		return t_real(0);
	}


	std::lock_guard<std::mutex> lock(*m_pmtx);
	try
	{
		return py::extract<t_real>(m_Sqw(dh, dk, dl, dE));
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();
	}

	return 0.;
}


/**
 * Gets model variables.
 */
std::vector<SqwBase::t_var> SqwPy::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;
	if(!m_bOk)
	{
		tl::log_err("Interpreter has not initialised, cannot get variables.");
		return vecVars;
	}

	try
	{
		py::dict dict = py::extract<py::dict>(m_mod.attr("__dict__"));

		for(py::ssize_t i=0; i<py::len(dict.items()); ++i)
		{
			// name
			std::string strName = py::extract<std::string>(dict.items()[i][0]);
			if(strName.length() == 0) continue;
			if(strName[0] == '_') continue;

			// filter out non-prefixed variables
			if(m_strVarPrefix.size() && strName.substr(0,m_strVarPrefix.size()) != m_strVarPrefix)
				continue;

			// type
			std::string strType = py::extract<std::string>(dict.items()[i][1]
				.attr("__class__").attr("__name__"));
			if(strType=="module" || strType=="NoneType" || strType=="type")
				continue;
			if(strType.find("func") != std::string::npos)
				continue;

			// value
			std::string strValue = py::extract<std::string>(dict.items()[i][1].attr("__repr__")());
			if(strValue.length() > MAX_PARAM_VAL_SIZE)
			{
				//tl::log_warn("Value of variable \"", strName, "\" is too large, skipping.");
				continue;
			}

			SqwBase::t_var var;
			std::get<0>(var) = std::move(strName);
			std::get<1>(var) = std::move(strType);
			std::get<2>(var) = std::move(strValue);

			vecVars.push_back(var);
		}
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();
	}

	return vecVars;
}


/**
 * Sets model variables.
 */
void SqwPy::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(!m_bOk)
	{
		tl::log_err("Interpreter has not initialised, cannot set variables.");
		return;
	}

	/*for(const auto& var : vecVars)
	{
		tl::log_debug("Setting variable \"", std::get<0>(var),
			"\" to \"", std::get<2>(var), "\".");
	}*/

	try
	{
		py::dict dict = py::extract<py::dict>(m_mod.attr("__dict__"));
		std::vector<bool> vecVarsSet;
		for(std::size_t iCurVar=0; iCurVar<vecVars.size(); ++iCurVar)
			vecVarsSet.push_back(0);

		for(py::ssize_t i=0; i<py::len(dict.items()); ++i)
		{
			// variable name
			std::string strName = py::extract<std::string>(dict.items()[i][0]);
			if(strName.length() == 0) continue;
			if(strName[0] == '_') continue;

			// look for the variable name in vecVars
			bool bFound = 0;
			std::string strNewVal;
			for(std::size_t iCurVar=0; iCurVar<vecVars.size(); ++iCurVar)
			{
				const SqwBase::t_var& var = vecVars[iCurVar];
				if(std::get<0>(var) == strName)
				{
					bFound = 1;
					strNewVal = std::get<2>(var);
					vecVarsSet[iCurVar] = 1;
					break;
				}
			}
			if(!bFound)
				continue;

			// cast new value to variable type
			//auto tyVar = dict.items()[i][1].attr("__class__");
			//dict[strName] = tyVar(strNewVal);
			dict[strName] = py::eval(py::str(strNewVal), dict);
			//tl::log_debug(strName, " = ", strNewVal);
		}

		for(std::size_t iCurVar=0; iCurVar<vecVarsSet.size(); ++iCurVar)
		{
			bool bVarSet = vecVarsSet[iCurVar];
			if(!bVarSet)
			{
				tl::log_err("Could not set variable \"", std::get<0>(vecVars[iCurVar]),
					"\" as it was not found.");
			}
		}

		// TODO: check for changed parameters and if reinit is needed
		if(!!m_Init)
		{
			//tl::log_debug("Calling TakinInit...");
			m_Init();
		}
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();
	}
}


SqwBase* SqwPy::shallow_copy() const
{
	SqwPy* pSqw = new SqwPy();
	*static_cast<SqwBase*>(pSqw) = *static_cast<const SqwBase*>(this);

	pSqw->m_pmtx = this->m_pmtx;
	pSqw->m_sys = this->m_sys;
	pSqw->m_os = this->m_os;
	pSqw->m_mod = this->m_mod;
	pSqw->m_Sqw = this->m_Sqw;
	pSqw->m_Init = this->m_Init;
	pSqw->m_disp = this->m_disp;

	return pSqw;
}
