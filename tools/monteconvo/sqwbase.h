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
#include "tlibs/string/string.h"


/**
 * base class for S(q,w) models
 */
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


// ----------------------------------------------------------------------------


template<class t_vec>
std::string vec_to_str(const t_vec& vec)
{
	std::ostringstream ostr;
	for(const typename t_vec::value_type& t : vec)
		ostr << t << " ";
	return ostr.str();
}

template<class t_vec>
t_vec str_to_vec(const std::string& str)
{
	typedef typename t_vec::value_type T;

	std::vector<T> vec0;
	tl::get_tokens<T, std::string, std::vector<T>>(str, " \t", vec0);

	t_vec vec(vec0.size());
	for(std::size_t i=0; i<vec0.size(); ++i)
		vec[i] = vec0[i];
	return vec;
}

// ----------------------------------------------------------------------------

#endif
