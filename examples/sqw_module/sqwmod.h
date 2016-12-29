/**
 * S(q,w) module example
 * @author tweber
 * @date 2016
 * @license GPLv2
 */

#ifndef __MCONV_SQW_MOD_H__
#define __MCONV_SQW_MOD_H__

#include "tools/monteconvo/sqwbase.h"
#include "tlibs/math/linalg.h"
#include <tuple>

class SqwMod : public SqwBase
{
	public:
		using SqwBase::t_var;
		using t_real = t_real_reso;
		using t_vec = tl::ublas::vector<t_real>;

	protected:
		t_real m_dT = t_real(100);
		t_real m_dSigma = t_real(0.05);
		t_vec m_vecG;

	public:
		SqwMod();
		SqwMod(const std::string& strCfgFile);
		virtual ~SqwMod();

		std::tuple<t_real, t_real>
		dispersion(t_real dh, t_real dk, t_real dl) const;

		virtual t_real operator()(t_real dh, t_real dk, t_real dl, t_real dE) const override;

		virtual std::vector<t_var> GetVars() const override;
		virtual void SetVars(const std::vector<t_var>&) override;
		virtual bool SetVarIfAvail(const std::string& strKey, const std::string& strNewVal) override;

		virtual SqwBase* shallow_copy() const override;
};

#endif
