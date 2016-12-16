/**
 * S(Q,w) julia interface
 * @author tweber
 * @date dec-2016
 * @license GPLv2
 */

#ifndef __SQW_JL_H__
#define __SQW_JL_H__

#include "sqw.h"
#include <mutex>
#include <memory>


class SqwJl : public SqwBase
{
protected:
	mutable std::shared_ptr<std::mutex> m_pmtx;

	/*jl_function_t*/ void* m_pInit = nullptr;
	/*jl_function_t*/ void* m_pSqw = nullptr;

public:
	SqwJl() = default;
	SqwJl(const char* pcFile);
	virtual ~SqwJl();

	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};

#endif
