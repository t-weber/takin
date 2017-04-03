/**
 * interface for S(q,w) models
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015, 2016
 * @license GPLv2
 */

#ifndef __MCONV_SQW_BASE_H__
#define __MCONV_SQW_BASE_H__

#include <string>
#include <tuple>
#include <vector>
#include <memory>

#include "../res/defs.h"
#include "tlibs/string/string.h"


/**
 * base class for S(q,w) models
 */
class SqwBase
{
public:
	// basic fields: ident, type, value
	using t_var = std::tuple<std::string, std::string, std::string>;

	// extended fields: ident, error, is fit var?
	using t_var_fit = std::tuple<std::string, std::string, bool>;


protected:
	bool m_bOk = false;
	std::vector<t_var_fit> m_vecFit;


public:
	/**
	 * E(Q) dispersion and spectral weight function (optional)
	 * return [energies, weights]
	 */
	virtual std::tuple<std::vector<t_real_reso>, std::vector<t_real_reso>>
		disp(t_real_reso dh, t_real_reso dk, t_real_reso dl) const
	{ return std::tuple<std::vector<t_real_reso>, std::vector<t_real_reso>>({}, {}); }

	// S(Q,E) dynamical structure factor function
	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const = 0;
	virtual bool IsOk() const { return m_bOk; }

	// return model variables
	virtual std::vector<t_var> GetVars() const = 0;
	virtual const std::vector<t_var_fit>& GetFitVars() const { return m_vecFit; }

	// set model variables
	virtual void SetVars(const std::vector<t_var>&) = 0;
	virtual void SetFitVars(const std::vector<t_var_fit>& vecFit) { m_vecFit = vecFit; }
	virtual bool SetVarIfAvail(const std::string& strKey, const std::string& strNewVal);

	SqwBase() = default;
	virtual ~SqwBase() = default;

	virtual const SqwBase& operator=(const SqwBase& sqw);
	SqwBase(const SqwBase& sqw) { this->operator=(sqw); }

	virtual SqwBase* shallow_copy() const = 0;
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
