/**
 * S(q,w) module example
 * @author tweber
 * @date 2016
 * @license GPLv2
 */

// gcc -I. -shared -fPIC -o sqwmod.so examples/sqw_module/sqwmod.cpp tools/monteconvo/sqwbase.cpp -std=c++11 -lstdc++
#include "sqwmod.h"

using t_real = t_real_reso;


SqwMod::SqwMod()
{
	SqwBase::m_bOk = 1;
}

SqwMod::~SqwMod()
{
}

t_real SqwMod::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	return t_real(0);
}

std::vector<SqwMod::t_var> SqwMod::GetVars() const
{
	std::vector<t_var> vec;

	return vec;
}

void SqwMod::SetVars(const std::vector<SqwMod::t_var>& vec)
{
}

bool SqwMod::SetVarIfAvail(const std::string& strKey, const std::string& strNewVal)
{
	return 0;
}

SqwBase* SqwMod::shallow_copy() const
{
	return new SqwMod();
}



// ----------------------------------------------------------------------------
// SO interface

#include <memory>
#include <tuple>
#include <boost/dll/alias.hpp>
#include <iostream>
#include "libs/version.h"

std::tuple<std::string, std::string, std::string> sqw_info()
{
	std::cout << "In " << __func__ << "." << std::endl;

	return std::make_tuple(TAKIN_VER, "tstmod", "Test Module");
}

std::shared_ptr<SqwBase> sqw_construct(const std::string& strCfgFile)
{
	std::cout << "In " << __func__ << "." << std::endl;

	return std::make_shared<SqwMod>();
}


// exports from so file
BOOST_DLL_ALIAS(sqw_info, takin_sqw_info);
BOOST_DLL_ALIAS(sqw_construct, takin_sqw);
