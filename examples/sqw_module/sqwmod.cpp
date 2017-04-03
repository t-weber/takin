/**
 * S(q,w) module example
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2016
 * @license GPLv2
 */

// gcc -I. -shared -fPIC -o plugins/sqwmod.so examples/sqw_module/sqwmod.cpp tools/monteconvo/sqwbase.cpp tlibs/log/log.cpp -std=c++11 -lstdc++

#include "sqwmod.h"

#include "libs/version.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/math/neutrons.h"

#include <boost/dll/alias.hpp>


using t_real = typename SqwMod::t_real;


// ----------------------------------------------------------------------------
// constructors

SqwMod::SqwMod() : m_vecG(tl::make_vec<t_vec>({1,0,0}))
{
	SqwBase::m_bOk = 1;
}

SqwMod::SqwMod(const std::string& strCfgFile) : SqwMod()
{
	tl::log_info("Config file: \"", strCfgFile, "\".");
	SqwBase::m_bOk = 1;
}

SqwMod::~SqwMod()
{
}


// ----------------------------------------------------------------------------
// dispersion, spectral weight and structure factor

std::tuple<std::vector<t_real>, std::vector<t_real>>
	SqwMod::disp(t_real dh, t_real dk, t_real dl) const
{
	t_real dEp = 0;		// energy (boson creation)
	t_real dwp = 1;		// spectral weight (boson creation)
	t_real dEm = -dEp;	// energy (boson annihilation)
	t_real dwm = dwp;	// spectral weight (boson annihilation)

	// TODO: calculate dispersion relation

	return std::make_tuple(std::vector<t_real>({dEp, dEm}),
		std::vector<t_real>({dwp, dwm}));
}

t_real SqwMod::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	t_real dcut = t_real(0.02);

	std::vector<t_real> vecE, vecW;
	std::tie(vecE, vecW) = disp(dh, dk, dl);

	t_real dInc=0, dS_p=0, dS_m=0;
	if(!tl::float_equal(m_dIncAmp, t_real(0)))
		dInc = tl::gauss_model(dE, t_real(0), m_dIncSigma, m_dIncAmp, t_real(0));

	t_real dS = 0;
	for(std::size_t iE=0; iE<vecE.size(); ++iE)
	{
		if(!tl::float_equal(vecW[iE], t_real(0)))
			dS += tl::gauss_model(dE, vecE[iE], m_dSigma[0], vecW[iE], t_real(0));
	}

	return m_dS0*dS * tl::bose_cutoff(dE, m_dT, dcut) + dInc;
}



// ----------------------------------------------------------------------------
// get & set variables

std::vector<SqwMod::t_var> SqwMod::GetVars() const
{
	std::vector<t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"T", "real", tl::var_to_str(m_dT)});
	vecVars.push_back(SqwBase::t_var{"sigma_create", "real", tl::var_to_str(m_dSigma[0])});
	vecVars.push_back(SqwBase::t_var{"sigma_annihilate", "real", tl::var_to_str(m_dSigma[1])});
	vecVars.push_back(SqwBase::t_var{"single_sigma", "bool", tl::var_to_str(m_bUseSingleSigma)});
	vecVars.push_back(SqwBase::t_var{"inc_amp", "real", tl::var_to_str(m_dIncAmp)});
	vecVars.push_back(SqwBase::t_var{"inc_sigma", "real", tl::var_to_str(m_dIncSigma)});
	vecVars.push_back(SqwBase::t_var{"S0", "real", tl::var_to_str(m_dS0)});
	vecVars.push_back(SqwBase::t_var{"G", "vector", vec_to_str(m_vecG)});

	return vecVars;
}

void SqwMod::SetVars(const std::vector<SqwMod::t_var>& vecVars)
{
	if(!vecVars.size()) return;

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strVal = std::get<2>(var);

		if(strVar == "T") m_dT = tl::str_to_var<decltype(m_dT)>(strVal);
		else if(strVar == "sigma_create") m_dSigma[0] = tl::str_to_var<t_real>(strVal);
		else if(strVar == "sigma_annihilate") m_dSigma[1] = tl::str_to_var<t_real>(strVal);
		else if(strVar == "single_sigma") m_bUseSingleSigma = tl::str_to_var<bool>(strVal);
		else if(strVar == "inc_amp") m_dIncAmp = tl::str_to_var<decltype(m_dIncAmp)>(strVal);
		else if(strVar == "inc_sigma") m_dIncSigma = tl::str_to_var<decltype(m_dIncSigma)>(strVal);
		else if(strVar == "S0") m_dS0 = tl::str_to_var<decltype(m_dS0)>(strVal);
		else if(strVar == "G") m_vecG = str_to_vec<decltype(m_vecG)>(strVal);
	}
}

bool SqwMod::SetVarIfAvail(const std::string& strKey, const std::string& strNewVal)
{
	return SqwBase::SetVarIfAvail(strKey, strNewVal);
}


// ----------------------------------------------------------------------------
// copy

SqwBase* SqwMod::shallow_copy() const
{
	SqwMod *pMod = new SqwMod();

	pMod->m_dT = this->m_dT;
	pMod->m_dSigma[0] = this->m_dSigma[0];
	pMod->m_dSigma[1] = this->m_dSigma[1];
	pMod->m_bUseSingleSigma = this->m_bUseSingleSigma;
	pMod->m_dIncAmp = this->m_dIncAmp;
	pMod->m_dIncSigma = this->m_dIncSigma;
	pMod->m_vecG = this->m_vecG;
	pMod->m_dS0 = this->m_dS0;

	return pMod;
}



// ----------------------------------------------------------------------------
// SO interface

static const char* pcModIdent = "tstmod";
static const char* pcModName = "Test Module";

std::tuple<std::string, std::string, std::string> sqw_info()
{
	tl::log_info("In ", __func__, ".");

	return std::make_tuple(TAKIN_VER, pcModIdent, pcModName);
}

std::shared_ptr<SqwBase> sqw_construct(const std::string& strCfgFile)
{
	tl::log_info("In ", __func__, ".");

	return std::make_shared<SqwMod>(strCfgFile);
}


// exports from so file
BOOST_DLL_ALIAS(sqw_info, takin_sqw_info);
BOOST_DLL_ALIAS(sqw_construct, takin_sqw);
