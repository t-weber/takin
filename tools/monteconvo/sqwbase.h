/**
 * interface for S(q,w) models
 * @author tweber
 * @date 2015, 2016
 * @license GPLv2
 */

#ifndef __MCONV_SQW_BASE_H__
#define __MCONV_SQW_BASE_H__

#include <string>
#include <tuple>
#include <memory>
#include "../res/defs.h"


class SqwBase
{
public:
	// name, type, value
	using t_var = std::tuple<std::string, std::string, std::string>;

protected:
	bool m_bOk = false;

public:
	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const = 0;
	virtual bool IsOk() const { return m_bOk; }

	virtual std::vector<t_var> GetVars() const = 0;
	virtual void SetVars(const std::vector<t_var>&) = 0;
	virtual bool SetVarIfAvail(const std::string& strKey, const std::string& strNewVal);

	virtual SqwBase* shallow_copy() const = 0;
	virtual ~SqwBase() {}
};

#endif
