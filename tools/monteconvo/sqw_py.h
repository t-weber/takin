/*
 * S(Q,w) python interface
 * @author tweber
 * @date aug-2015
 * @license GPLv2
 */

#ifndef __SQW_PY_H__
#define __SQW_PY_H__

#include "sqw.h"
#include <mutex>
#include <memory>
#include <boost/python.hpp>
namespace py = boost::python;


class SqwPy : public SqwBase
{
protected:
	mutable std::shared_ptr<std::mutex> m_pmtx;
	py::object m_sys, m_mod;
	py::object m_Sqw, m_Init;

public:
	SqwPy() = default;
	SqwPy(const char* pcFile);
	virtual ~SqwPy();

	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};

#endif
