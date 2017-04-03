/**
 * interface for S(q,w) models
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015, 2016
 * @license GPLv2
 */

#include "sqwbase.h"


/**
 * if the variable "strKey" is known, update it with the value "strNewVal"
 */
bool SqwBase::SetVarIfAvail(const std::string& strKey, const std::string& strNewVal)
{
	std::vector<t_var> vecVars = GetVars();
	for(const t_var& var : vecVars)
	{
		if(strKey == std::get<0>(var))
		{
			t_var varNew = var;
			std::get<2>(varNew) = strNewVal;
			SetVars(std::vector<t_var>({varNew}));

			return true;
		}
	}

	return false;
}


const SqwBase& SqwBase::operator=(const SqwBase& sqw)
{
	this->m_bOk = sqw.m_bOk;
	this->m_vecFit = sqw.m_vecFit;

	return *this;
}
