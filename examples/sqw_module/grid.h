/**
 * S(q,w) module example for a pre-calculated grid
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 30-sep-2018
 * @license GPLv2
 */

#ifndef __MCONV_SQW_GRID_H__
#define __MCONV_SQW_GRID_H__

#include "tools/monteconvo/sqwbase.h"
#include "tlibs/math/linalg.h"


class SqwMod : public SqwBase
{
	public:
		using SqwBase::t_var;
		using t_real = t_real_reso;
		using t_vec = tl::ublas::vector<t_real>;

	protected:
		// temperature for Bose factor
		t_real m_dT = t_real(100);

		// Bose cutoff
		t_real m_dcut = t_real(0.02);

		// peak width for creation and annihilation
		t_real m_dSigma = t_real(0.05);

		// S(q,E) scaling factor
		t_real m_dS0 = t_real(1.);

		// incoherent amplitude and width
		t_real m_dIncAmp = t_real(0.);
		t_real m_dIncSigma = t_real(0.05);

		// grid descriptors
		std::string m_strIndexFile, m_strDataFile;

		t_real m_hmin=0., m_hmax=0., m_hstep=0.;
		t_real m_kmin=0., m_kmax=0., m_kstep=0.;
		t_real m_lmin=0., m_lmax=0., m_lstep=0.;


	public:
		SqwMod();
		SqwMod(const std::string& strCfgFile);
		virtual ~SqwMod();

		virtual std::tuple<std::vector<t_real>, std::vector<t_real>>
			disp(t_real dh, t_real dk, t_real dl) const override;
		virtual t_real operator()(t_real dh, t_real dk, t_real dl, t_real dE) const override;

		virtual std::vector<t_var> GetVars() const override;
		virtual void SetVars(const std::vector<t_var>&) override;
		virtual bool SetVarIfAvail(const std::string& strKey, const std::string& strNewVal) override;

		virtual SqwBase* shallow_copy() const override;
};

#endif
