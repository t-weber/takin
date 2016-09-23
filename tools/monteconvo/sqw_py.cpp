/**
 * S(Q,w) python interface
 * @author tweber
 * @date aug-2015
 * @license GPLv2
 */

#include "sqw_py.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"

using t_real = t_real_reso;

#define MAX_PARAM_VAL_SIZE 128

SqwPy::SqwPy(const char* pcFile) : m_pmtx(std::make_shared<std::mutex>())
{
	std::string strFile = pcFile;
	std::string strDir = tl::get_dir(strFile);
	std::string strMod = tl::get_file_noext(tl::get_file(strFile));
	const bool bSetScriptCWD = 1;

	try	// mandatory stuff
	{
		static bool bInited = 0;
		if(!bInited)
		{
			::Py_InitializeEx(0);
			tl::log_debug("Initialised Python interpreter version ", Py_GetVersion(), ".");
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
				tl::log_warn("Cannot set script working directory.");
		}

		// import takin functions
		m_mod = py::import(strMod.c_str());
		py::dict moddict = py::extract<py::dict>(m_mod.attr("__dict__"));
		m_Sqw = moddict["TakinSqw"];
		m_bOk = !!m_Sqw;
		if(!m_bOk)
			tl::log_err("Script has no TakinSqw function.");

		try	// optional stuff
		{
			if(moddict.has_key("TakinInit"))
			{
				m_Init = moddict["TakinInit"];
				if(!!m_Init) m_Init();
			}
			else
			{
				tl::log_warn("Script has no TakinInit function.");
			}
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


t_real SqwPy::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	if(!m_bOk) return t_real(0);

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


std::vector<SqwBase::t_var> SqwPy::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;
	if(!m_bOk) return vecVars;

	try
	{
		py::dict dict = py::extract<py::dict>(m_mod.attr("__dict__"));

		for(py::ssize_t i=0; i<py::len(dict.items()); ++i)
		{
			// name
			std::string strName = py::extract<std::string>(dict.items()[i][0]);
			if(strName.length() == 0) continue;
			if(strName[0] == '_') continue;

			// type
			std::string strType = py::extract<std::string>(dict.items()[i][1]
				.attr("__class__").attr("__name__"));
			if(strType=="module" || strType=="NoneType" || strType=="type")
				continue;
			if(strType.find("func") != std::string::npos)
				continue;

			// value
			std::string strValue = py::extract<std::string>(dict.items()[i][1]
				.attr("__repr__")());
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


void SqwPy::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(!m_bOk) return;

	try
	{
		py::dict dict = py::extract<py::dict>(m_mod.attr("__dict__"));

		for(py::ssize_t i=0; i<py::len(dict.items()); ++i)
		{
			// variable name
			std::string strName = py::extract<std::string>(dict.items()[i][0]);
			if(strName.length() == 0) continue;
			if(strName[0] == '_') continue;

			// look for the variable name in vecVars
			bool bFound = 0;
			std::string strNewVal;
			for(const SqwBase::t_var& var : vecVars)
			{
				if(std::get<0>(var) == strName)
				{
					bFound = 1;
					strNewVal = std::get<2>(var);
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

		// TODO: check for changed parameters and if reinit is needed
		if(!!m_Init) m_Init();
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

	return pSqw;
}
