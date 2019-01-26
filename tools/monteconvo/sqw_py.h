/**
 * S(Q,w) python interface
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2015
 * @license GPLv2
 */

#ifndef __SQW_PY_H__
#define __SQW_PY_H__

#include "sqw.h"
#include <mutex>
#include <memory>
#include <string>

#define slots_bck slots
#undef slots
// get "slots" out of the way as it is a qt keyword...
#include <boost/python.hpp>
#define slots slots_bck
namespace py = boost::python;


class SqwPy : public SqwBase
{
protected:
	mutable std::shared_ptr<std::mutex> m_pmtx;

	py::object m_sys, m_os, m_mod;
	py::object m_Sqw, m_disp, m_Init;

	// filter variables that don't start with the given prefix
	std::string m_strVarPrefix = "g_";

public:
	SqwPy() = default;
	SqwPy(const char* pcFile);
	virtual ~SqwPy();

	virtual std::tuple<std::vector<t_real_reso>, std::vector<t_real_reso>>
		disp(t_real_reso dh, t_real_reso dk, t_real_reso dl) const override;
	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;

	void SetVarPrefix(const char* pcFilter) { m_strVarPrefix = pcFilter; }
};

#endif
