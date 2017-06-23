/**
 * Loads tabulated spacegroups
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2016
 * @license GPLv2
 */

#ifndef __SG_TAB_IMPL_H__
#define __SG_TAB_IMPL_H__

#include <sstream>
#include "libs/globals.h"	// find_resource


template<class t_real>
SpaceGroups<t_real>::SpaceGroups()
{
	// load general space group list
	m_bOk = LoadSpaceGroups("res/data/sgroups.xml", 1);

	// load custom-defined space groups
	LoadSpaceGroups("res/data/sg_user.xml", 0);
	LoadSpaceGroups("data/sg_user.xml", 0);
}


template<class t_real>
SpaceGroups<t_real>::~SpaceGroups() {}


template<class t_real>
bool SpaceGroups<t_real>::LoadSpaceGroups(const char* pcFile, bool bMandatory)
{
	using t_mat = typename SpaceGroup<t_real>::t_mat;
	//using t_vec = typename SpaceGroup<t_real>::t_vec;

	std::string strTabFile = find_resource(pcFile, bMandatory);
	if(strTabFile == "")
		return false;
	tl::log_debug("Loading space groups from file \"", strTabFile, "\".");

	tl::Prop<std::string> xml;
	if(!xml.Load(strTabFile.c_str(), tl::PropType::XML))
		return false;

	//unsigned int iNumSGs = xml.Query<unsigned int>("sgroups/num_groups", 230);
	typedef typename t_mapSpaceGroups::value_type t_val;

	unsigned int iSg = 0;
	while(1)
	{
		std::ostringstream ostrGroup;
		ostrGroup << "sgroups/group_" << iSg;
		std::string strGroup = ostrGroup.str();

		if(!xml.Exists(strGroup.c_str()))
			break;

		unsigned int iSgNr = xml.Query<unsigned int>((strGroup+"/number").c_str());
		std::string strName = tl::trimmed(xml.Query<std::string>((strGroup+"/name").c_str()));
		std::string strLaue = tl::trimmed(xml.Query<std::string>((strGroup+"/lauegroup").c_str()));
		unsigned int iNumTrafos = xml.Query<unsigned int>((strGroup+"/num_trafos").c_str(), 0);

		std::vector<t_mat> vecTrafos;
		std::vector<unsigned int> vecInvTrafos, vecPrimTrafos, vecCenterTrafos;
		vecTrafos.reserve(iNumTrafos);

		unsigned int iTrafo = 0;
		while(1)
		{
			std::ostringstream ostrTrafo;
			ostrTrafo << strGroup << "/trafo_" << iTrafo;
			std::string strTrafo = ostrTrafo.str();

			if(!xml.Exists(strTrafo.c_str()))
				break;

			std::string strTrafoVal = xml.Query<std::string>(strTrafo.c_str());
			std::pair<std::string, std::string> pairSg = tl::split_first(strTrafoVal, std::string(";"), 1);

			std::istringstream istrMat(pairSg.first);
			t_mat mat;
			istrMat >> mat;

			for(typename std::string::value_type c : pairSg.second)
			{
				if(std::tolower(c)=='p') vecPrimTrafos.push_back(iTrafo);
				if(std::tolower(c)=='i') vecInvTrafos.push_back(iTrafo);
				if(std::tolower(c)=='c') vecCenterTrafos.push_back(iTrafo);
			}

			vecTrafos.push_back(std::move(mat));
			++iTrafo;
		}

		if(iNumTrafos != 0 && iTrafo != iNumTrafos)
		{
			tl::log_warn(iTrafo, " transformations were given, but ", iNumTrafos,
				" were expected in space group ", iSgNr);
		}

		SpaceGroup<t_real> sg;
		sg.SetNr(iSgNr);
		sg.SetName(strName);
		sg.SetLaueGroup(strLaue);
		sg.SetTrafos(std::move(vecTrafos));
		sg.SetInvTrafos(std::move(vecInvTrafos));
		sg.SetPrimTrafos(std::move(vecPrimTrafos));
		sg.SetCenterTrafos(std::move(vecCenterTrafos));

		auto pairSG = g_mapSpaceGroups.insert(t_val(sg.GetName(), std::move(sg)));
		g_vecSpaceGroups.push_back(&pairSG.first->second);
		++iSg;
	}

	// sort by sg number
	std::sort(g_vecSpaceGroups.begin(), g_vecSpaceGroups.end(),
		[](const SpaceGroup<t_real>* sg1, const SpaceGroup<t_real>* sg2) -> bool
		{ return sg1->GetNr() < sg2->GetNr(); });

	if(s_strSrc == "")
		s_strSrc = xml.Query<std::string>("sgroups/source", "");
	if(s_strUrl == "")
		s_strUrl = xml.Query<std::string>("sgroups/source_url", "");


	if(g_vecSpaceGroups.size() < 230)
		tl::log_warn("Less than 230 space groups are defined!");

	return true;
}


template<class t_real>
std::shared_ptr<const SpaceGroups<t_real>> SpaceGroups<t_real>::GetInstance()
{
	std::lock_guard<std::mutex> _guard(s_mutex);

	if(!s_inst)
		s_inst = std::shared_ptr<SpaceGroups>(new SpaceGroups());

	return s_inst;
}


template<class t_real>
const typename SpaceGroups<t_real>::t_mapSpaceGroups* SpaceGroups<t_real>::get_space_groups() const
{
	if(g_mapSpaceGroups.empty())
		tl::log_warn("Space Groups have not been initialised properly.");

	return &g_mapSpaceGroups;
}


template<class t_real>
const typename SpaceGroups<t_real>::t_vecSpaceGroups* SpaceGroups<t_real>::get_space_groups_vec() const
{
	if(g_vecSpaceGroups.empty())
		tl::log_warn("Space Groups have not been initialised properly.");

	return &g_vecSpaceGroups;
}


template<class t_real>
const std::string& SpaceGroups<t_real>::get_sgsource(bool bUrl) const
{
	return bUrl ? s_strUrl : s_strSrc ;
}


// statics
template<class t_real> std::shared_ptr<SpaceGroups<t_real>> SpaceGroups<t_real>::s_inst = nullptr;
template<class t_real> std::mutex SpaceGroups<t_real>::s_mutex;


#endif
