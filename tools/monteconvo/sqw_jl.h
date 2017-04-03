/**
 * S(Q,w) julia interface
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date dec-2016
 * @license GPLv2
 */

#ifndef __SQW_JL_H__
#define __SQW_JL_H__

#include "sqw.h"
#include <mutex>
#include <string>
#include <memory>


class SqwJl : public SqwBase
{
protected:
	mutable std::shared_ptr<std::mutex> m_pmtx;

	/*jl_function_t*/ void *m_pInit = nullptr;
	/*jl_function_t*/ void *m_pSqw = nullptr;
	/*jl_function_t*/ void *m_pDisp = nullptr;

	// filter variables that don't start with the given prefix
	std::string m_strVarPrefix = "g_";

protected:
	void PrintExceptions() const;

public:
	SqwJl() = default;
	SqwJl(const char* pcFile);
	virtual ~SqwJl();

	virtual std::tuple<std::vector<t_real_reso>, std::vector<t_real_reso>>
		disp(t_real_reso dh, t_real_reso dk, t_real_reso dl) const override;
	virtual t_real_reso
		operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;

	void SetVarPrefix(const char* pcFilter) { m_strVarPrefix = pcFilter; }
};

#endif
