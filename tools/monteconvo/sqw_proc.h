/**
 * delegates S(Q,w) functions through processes
 * @author tweber
 * @date sep-2016
 * @license GPLv2
 */

#ifndef __SQW_PROC_H__
#define __SQW_PROC_H__

#include "sqw.h"
#include <mutex>
#include <memory>
#include <unistd.h>
#include <boost/interprocess/ipc/message_queue.hpp>


template<class t_sqw>
class SqwProc : public SqwBase
{
private:
	std::size_t m_iRefCnt = 0;

protected:
	mutable std::shared_ptr<std::mutex> m_pmtx;

	std::string m_strProcName;
	pid_t m_pidChild = 0;

	std::shared_ptr<boost::interprocess::managed_shared_memory> m_pMem;
	std::shared_ptr<boost::interprocess::message_queue> m_pmsgIn, m_pmsgOut;
	void *m_pSharedPars;

public:
	SqwProc();
	SqwProc(const char* pcCfg);
	virtual ~SqwProc();

	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;
	virtual bool IsOk() const override;

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};

#endif
