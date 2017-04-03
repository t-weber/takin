/**
 * S(Q,w) processes
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date sep-2016
 * @license GPLv2
 */

#ifndef __SQW_PROC_IMPL_H__
#define __SQW_PROC_IMPL_H__

#include "sqw_proc.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/math/rand.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>

#define MSG_QUEUE_SIZE 128
#define PARAM_MEM 1024*1024


namespace ipr = boost::interprocess;
using t_real = t_real_reso;


// ----------------------------------------------------------------------------
// types and conversions

template<class t_ch=char>
using t_sh_str_alloc_gen = ipr::allocator<t_ch, ipr::managed_shared_memory::segment_manager>;
template<class t_ch=char>
using t_sh_str_gen = ipr::basic_string<t_ch, std::char_traits<t_ch>, t_sh_str_alloc_gen<t_ch>>;

using t_sh_str_alloc = t_sh_str_alloc_gen<char>;
using t_sh_str = t_sh_str_gen<char>;


/**
 * converts the energy and weight vectors of the dispersion to a string
 */
static void disp_to_str(
	t_sh_str& str, const std::tuple<std::vector<t_real>, std::vector<t_real>>& tup)
{
	try
	{
		str.clear();
		std::string strVecE = tl::cont_to_str<std::vector<t_real>>(std::get<0>(tup), ",");
		std::string strVecW = tl::cont_to_str<std::vector<t_real>>(std::get<1>(tup), ",");

		std::size_t iLenTotal = strVecE.length() + strVecW.length() + 3 + 1;
		if(iLenTotal >= std::size_t(PARAM_MEM))
		{
			tl::log_err("Process buffer limit reached. Cannot proceed.");
			return;
		}

		str.append(strVecE.c_str()); str.append("#,#");
		str.append(strVecW.c_str());
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}


/**
 * converts a string to the energy and weight vectors of the dispersion
 */
static std::tuple<std::vector<t_real>, std::vector<t_real>>
str_to_disp(const t_sh_str& _str)
{
	std::vector<t_real> vecE, vecW;

	try
	{
		std::string str;
		for(const t_sh_str::value_type& ch : _str) str.push_back(ch);

		std::string strVecE, strVecW;
		std::tie(strVecE, strVecW) = tl::split_first<std::string>(str, "#,#", 1, 1);

		tl::get_tokens<t_real, std::string, decltype(vecE)>(strVecE, ",", vecE);
		tl::get_tokens<t_real, std::string, decltype(vecW)>(strVecW, ",", vecW);
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}

	return std::make_tuple(vecE, vecW);
}


/**
 * converts the model parameters to a string
 */
static void pars_to_str(t_sh_str& str, const std::vector<SqwBase::t_var>& vec)
{
	try
	{
		str.clear();

		std::size_t iLenTotal = 0;
		for(const SqwBase::t_var& var : vec)
		{
			iLenTotal += std::get<0>(var).length() +
				std::get<1>(var).length() +
				std::get<2>(var).length() + 3*3 + 1;
			if(iLenTotal >= std::size_t(PARAM_MEM*0.9))
			{
				tl::log_err("Process buffer limit imminent. Truncating parameter list.");
				break;
			}

			str.append(std::get<0>(var).c_str()); str.append("#,#");
			str.append(std::get<1>(var).c_str()); str.append("#,#");
			str.append(std::get<2>(var).c_str()); str.append("#;#");
		}
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}


/**
 * converts a string to the model parameters
 */
static std::vector<SqwBase::t_var> str_to_pars(const t_sh_str& _str)
{
	std::vector<SqwBase::t_var> vec;
	try
	{
		std::string str;
		for(const t_sh_str::value_type& ch : _str) str.push_back(ch);

		std::vector<std::string> vecLines;
		tl::get_tokens_seq<std::string, std::string, std::vector>
			(str, "#;#", vecLines, 1);


		vec.reserve(vecLines.size());

		for(const std::string& strLine : vecLines)
		{
			if(strLine.length() == 0) continue;

			std::vector<std::string> vecVals;
			tl::get_tokens_seq<std::string, std::string, std::vector>
				(strLine, "#,#", vecVals, 1);

			if(vecVals.size() != 3)
			{
				tl::log_err("Wrong size of parameters: \"", strLine, "\".");
				continue;
			}

			SqwBase::t_var var;
			std::get<0>(var) = vecVals[0];
			std::get<1>(var) = vecVals[1];
			std::get<2>(var) = vecVals[2];
			vec.push_back(var);
		}
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
	return vec;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// messages

enum class ProcMsgTypes
{
	QUIT,
	NOP,

	DISP,
	SQW,
	GET_VARS,
	SET_VARS,

	IS_OK,
	READY,
};

struct ProcMsg
{
	ProcMsgTypes ty = ProcMsgTypes::NOP;

	t_real dParam1, dParam2, dParam3, dParam4;
	t_real dRet;
	bool bRet;

	t_sh_str *pPars = nullptr;
};

static void msg_send(ipr::message_queue& msgqueue, const ProcMsg& msg)
{
	try
	{
		msgqueue.send(&msg, sizeof(msg), 0);
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}

static ProcMsg msg_recv(ipr::message_queue& msgqueue)
{
	ProcMsg msg;
	try
	{
		std::size_t iSize;
		unsigned int iPrio;
		msgqueue.receive(&msg, sizeof(msg), iSize, iPrio);

		if(iSize != sizeof(msg))
			tl::log_err("Message size mismatch.");
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
	return msg;
}

// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// child process

// ----------------------------------------------------------------------------

template<class t_sqw>
static void child_proc(ipr::message_queue& msgToParent, ipr::message_queue& msgFromParent,
	const char* pcCfg)
{
	std::unique_ptr<t_sqw> pSqw(new t_sqw(pcCfg));

	// tell parent that pSqw is inited
	ProcMsg msgReady;
	msgReady.ty = ProcMsgTypes::READY;
	msgReady.bRet = pSqw->IsOk();
	msg_send(msgToParent, msgReady);

	while(1)
	{
		ProcMsg msg = msg_recv(msgFromParent);
		ProcMsg msgRet;

		switch(msg.ty)
		{
			case ProcMsgTypes::DISP:	// dispersion
			{
				msgRet.ty = msg.ty;
				msgRet.pPars = msg.pPars;	// use provided pointer to shared mem
				disp_to_str(*msgRet.pPars, pSqw->disp(msg.dParam1, msg.dParam2, msg.dParam3));
				msg_send(msgToParent, msgRet);
				break;
			}
			case ProcMsgTypes::SQW:		// structure factor
			{
				msgRet.ty = msg.ty;
				msgRet.dRet = pSqw->operator()(msg.dParam1, msg.dParam2, msg.dParam3, msg.dParam4);
				msg_send(msgToParent, msgRet);
				break;
			}
			case ProcMsgTypes::GET_VARS:	// get variables
			{
				msgRet.ty = msg.ty;
				msgRet.pPars = msg.pPars;	// use provided pointer to shared mem
				pars_to_str(*msgRet.pPars, pSqw->GetVars());
				msg_send(msgToParent, msgRet);
				break;
			}
			case ProcMsgTypes::SET_VARS:	// set variables
			{
				pSqw->SetVars(str_to_pars(*msg.pPars));
				msgRet.ty = ProcMsgTypes::READY;
				msgRet.bRet = 1;
				msg_send(msgToParent, msgRet);
				break;
			}
			case ProcMsgTypes::IS_OK:
			{
				msgRet.ty = msg.ty;
				msgRet.bRet = pSqw->IsOk();
				msg_send(msgToParent, msgRet);
				break;
			}
			case ProcMsgTypes::QUIT:
			{
				//tl::log_debug("Exiting child process");
				exit(0);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}


// ----------------------------------------------------------------------------
// parent process

template<class t_sqw>
SqwProc<t_sqw>::SqwProc()
{
	++m_iRefCnt;
}


/**
 * create sub-process
 */
template<class t_sqw>
SqwProc<t_sqw>::SqwProc(const char* pcCfg)
	: m_pmtx(std::make_shared<std::mutex>()),
	m_strProcName(tl::rand_name<std::string>(8))
{
	++m_iRefCnt;

	try
	{
		tl::log_debug("Creating process memory \"", "takin_sqw_proc_*_", m_strProcName, "\".");

		m_pMem = std::make_shared<ipr::managed_shared_memory>(ipr::create_only,
			("takin_sqw_proc_mem_" + m_strProcName).c_str(), PARAM_MEM);
		m_pSharedPars = static_cast<void*>(m_pMem->construct<t_sh_str>
			(("takin_sqw_proc_params_" + m_strProcName).c_str())
			(t_sh_str_alloc(m_pMem->get_segment_manager())));

		m_pmsgIn = std::make_shared<ipr::message_queue>(ipr::create_only,
			("takin_sqw_proc_in_" + m_strProcName).c_str(), MSG_QUEUE_SIZE, sizeof(ProcMsg));
		m_pmsgOut = std::make_shared<ipr::message_queue>(ipr::create_only,
			("takin_sqw_proc_out_" + m_strProcName).c_str(), MSG_QUEUE_SIZE, sizeof(ProcMsg));

		m_pidChild = fork();
		if(m_pidChild < 0)
		{
			tl::log_err("Cannot fork process.");
			return;
		}
		else if(m_pidChild == 0)
		{
			child_proc<t_sqw>(*m_pmsgIn, *m_pmsgOut, pcCfg);
			exit(0);
		}

		tl::log_debug("Waiting for client to become ready...");
		ProcMsg msgReady = msg_recv(*m_pmsgIn);
		if(!msgReady.bRet)
			tl::log_err("Client reports failure.");
		else
			tl::log_debug("Client is ready.");

		m_bOk = msgReady.bRet;
	}
	catch(const std::exception& ex)
	{
		m_bOk = 0;
		tl::log_err(ex.what());
	}
}


/**
 * clean up sub-process
 */
template<class t_sqw>
SqwProc<t_sqw>::~SqwProc()
{
	// is this instance the last?
	if(m_pMem.use_count() > 1) return;

	try
	{
		if(m_pmsgOut)
		{
			ProcMsg msg;
			msg.ty = ProcMsgTypes::QUIT;
			msg_send(*m_pmsgOut, msg);
		}

		if(--m_iRefCnt == 0)
		{
			tl::log_debug("Removing process memory \"", "takin_sqw_proc_*_", m_strProcName, "\".");

			ipr::shared_memory_object::remove(("takin_sqw_proc_mem_" + m_strProcName).c_str());

			ipr::message_queue::remove(("takin_sqw_proc_in_" + m_strProcName).c_str());
			ipr::message_queue::remove(("takin_sqw_proc_out_" + m_strProcName).c_str());
		}
	}
	catch(const std::exception&)
	{}
}


/**
 * query dispersion
 */
template<class t_sqw>
std::tuple<std::vector<t_real>, std::vector<t_real>>
SqwProc<t_sqw>::disp(t_real dh, t_real dk, t_real dl) const
{
	std::lock_guard<std::mutex> lock(*m_pmtx);

	ProcMsg msg;
	msg.ty = ProcMsgTypes::DISP;
	msg.dParam1 = dh;
	msg.dParam2 = dk;
	msg.dParam3 = dl;
	msg.pPars = static_cast<decltype(msg.pPars)>(m_pSharedPars);
	msg_send(*m_pmsgOut, msg);

	ProcMsg msgDisp = msg_recv(*m_pmsgIn);
	return str_to_disp(*msgDisp.pPars);
}


/**
 * query dynamical structure factor
 */
template<class t_sqw>
t_real SqwProc<t_sqw>::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	std::lock_guard<std::mutex> lock(*m_pmtx);

	ProcMsg msg;
	msg.ty = ProcMsgTypes::SQW;
	msg.dParam1 = dh;
	msg.dParam2 = dk;
	msg.dParam3 = dl;
	msg.dParam4 = dE;
	msg_send(*m_pmsgOut, msg);

	ProcMsg msgS = msg_recv(*m_pmsgIn);
	return msgS.dRet;
}

template<class t_sqw>
bool SqwProc<t_sqw>::IsOk() const
{
	if(!m_bOk) return false;
	std::lock_guard<std::mutex> lock(*m_pmtx);

	ProcMsg msg;
	msg.ty = ProcMsgTypes::IS_OK;
	msg_send(*m_pmsgOut, msg);

	ProcMsg msgRet = msg_recv(*m_pmsgIn);
	return msgRet.bRet;
}


/**
 * query variables
 */
template<class t_sqw>
std::vector<SqwBase::t_var> SqwProc<t_sqw>::GetVars() const
{
	std::lock_guard<std::mutex> lock(*m_pmtx);

	ProcMsg msg;
	msg.ty = ProcMsgTypes::GET_VARS;
	msg.pPars = static_cast<decltype(msg.pPars)>(m_pSharedPars);
	msg_send(*m_pmsgOut, msg);

	ProcMsg msgRet = msg_recv(*m_pmsgIn);
	return str_to_pars(*msg.pPars);
}


/**
 * set variables
 */
template<class t_sqw>
void SqwProc<t_sqw>::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	std::lock_guard<std::mutex> lock(*m_pmtx);

	ProcMsg msg;
	msg.ty = ProcMsgTypes::SET_VARS;
	msg.pPars = static_cast<decltype(msg.pPars)>(m_pSharedPars);
	pars_to_str(*msg.pPars, vecVars);
	//tl::log_debug("Message string: ", *msg.pPars);
	msg_send(*m_pmsgOut, msg);

	ProcMsg msgRet = msg_recv(*m_pmsgIn);
	if(!msgRet.bRet)
		tl::log_err("Could not set variables.");
}


template<class t_sqw>
SqwBase* SqwProc<t_sqw>::shallow_copy() const
{
	SqwProc* pSqw = new SqwProc();
	*static_cast<SqwBase*>(pSqw) = *static_cast<const SqwBase*>(this);

	pSqw->m_pmtx = this->m_pmtx;
	pSqw->m_pMem = this->m_pMem;
	pSqw->m_pmsgIn = this->m_pmsgIn;
	pSqw->m_pmsgOut = this->m_pmsgOut;
	pSqw->m_strProcName = this->m_strProcName;
	pSqw->m_pidChild = this->m_pidChild;
	pSqw->m_pSharedPars = this->m_pSharedPars;

	return pSqw;
}

// ----------------------------------------------------------------------------

#endif
