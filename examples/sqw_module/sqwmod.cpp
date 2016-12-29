/**
 * S(q,w) module example
 * @author tweber
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
// dispersion

std::tuple<t_real, t_real>
SqwMod::dispersion(t_real dh, t_real dk, t_real dl) const
{
	t_real dE = 0;	// energy
	t_real dw = 1;	// spectral weight

	return std::make_tuple(dE, dw);
}

t_real SqwMod::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	t_real dcut = t_real(0.02);

	t_real dE_peak, dw_peak;
	std::tie(dE_peak, dw_peak) = dispersion(dh, dk, dl);

	t_real dS_p = tl::gauss_model(dE, dE_peak, m_dSigma, dw_peak, t_real(0));
	t_real dS_m = tl::gauss_model(dE, -dE_peak, m_dSigma, dw_peak, t_real(0));

	return (dS_p + dS_m) * tl::bose_cutoff(dE, m_dT, dcut);
}



// ----------------------------------------------------------------------------
// get & set variables

std::vector<SqwMod::t_var> SqwMod::GetVars() const
{
	std::vector<t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"T", "real", tl::var_to_str(m_dT)});
	vecVars.push_back(SqwBase::t_var{"sigma", "real", tl::var_to_str(m_dSigma)});
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
		else if(strVar == "sigma") m_dSigma = tl::str_to_var<decltype(m_dSigma)>(strVal);
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
	pMod->m_dSigma = this->m_dSigma;

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
