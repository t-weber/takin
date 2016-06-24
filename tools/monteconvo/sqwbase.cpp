/**
 * interface for S(q,w) models
 * @author tweber
 * @date 2015, 2016
 * @license GPLv2
 */

#include "sqwbase.h"


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
