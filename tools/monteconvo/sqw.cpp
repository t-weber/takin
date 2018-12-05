/**
 * monte carlo convolution tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015 -- 2018
 * @license GPLv2
 */

#include "sqw.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/math/math.h"
#include "tlibs/math/linalg.h"
#include "tlibs/phys/neutrons.h"
#include <fstream>
#include <list>

using t_real = t_real_reso;


//------------------------------------------------------------------------------


SqwElast::SqwElast(const char* pcFile) : m_bLoadedFromFile(true)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr)
	{
		m_bLoadedFromFile = false;
		tl::log_err("Cannot open config file.");
		return;
	}

	std::string strLine;
	while(std::getline(ifstr, strLine))
	{
		tl::trim(strLine);
		if(strLine.length()==0 || strLine[0]=='#')
			continue;

		std::istringstream istr(strLine);
		t_real h=0., k=0. ,l=0., dSigQ=0., dSigE=0., dS=0.;
		istr >> h >> k >> l >> dSigQ >> dSigE >> dS;

		AddPeak(h,k,l, dSigQ, dSigE, dS);
	}

	tl::log_info("Number of elastic peaks: ", m_lstPeaks.size());
	SqwBase::m_bOk = true;
}

void SqwElast::AddPeak(t_real h, t_real k, t_real l, t_real dSigQ, t_real dSigE, t_real dS)
{
	ElastPeak pk;
	pk.h = h; pk.k = k; pk.l = l;
	pk.dSigQ = dSigQ; pk.dSigE = dSigE;
	pk.dS = dS;
	m_lstPeaks.push_back(std::move(pk));
}

t_real SqwElast::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	const ublas::vector<t_real> vecCur = tl::make_vec({dh, dk, dl});

	if(!m_bLoadedFromFile)	// use nearest integer bragg peak
	{
		const ublas::vector<t_real> vecPt = tl::make_vec({std::round(dh), std::round(dk), std::round(dl)});

		const t_real dDistQ = ublas::norm_2(vecPt-vecCur);
		const t_real dSigmaQ = 0.02;
		const t_real dSigmaE = 0.02;

		return tl::gauss_model<t_real>(dDistQ, 0., dSigmaQ, 1., 0.) *
			tl::gauss_model<t_real>(dE, 0., dSigmaE, 1., 0.);
	}
	else	// use bragg peaks from config file
	{
		t_real dS = 0.;

		for(const ElastPeak& pk : m_lstPeaks)
		{
			const ublas::vector<t_real> vecPk = tl::make_vec({pk.h, pk.k, pk.l});
			const t_real dDistQ = ublas::norm_2(vecPk-vecCur);

			dS += pk.dS * tl::gauss_model<t_real>(dDistQ, 0., pk.dSigQ, 1., 0.) *
				tl::gauss_model<t_real>(dE, 0., pk.dSigE, 1., 0.);
		}

		return dS;
	}
}


std::vector<SqwBase::t_var> SqwElast::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;
	return vecVars;
}


void SqwElast::SetVars(const std::vector<SqwBase::t_var>&)
{
}


SqwBase* SqwElast::shallow_copy() const
{
	SqwElast *pElast = new SqwElast();
	*static_cast<SqwBase*>(pElast) = *static_cast<const SqwBase*>(this);

	pElast->m_bLoadedFromFile = m_bLoadedFromFile;
	pElast->m_lstPeaks = m_lstPeaks;	// not a shallow copy!
	return pElast;
}




//------------------------------------------------------------------------------




SqwKdTree::SqwKdTree(const char* pcFile)
{
	if(pcFile)
		m_bOk = open(pcFile);
}


bool SqwKdTree::open(const char* pcFile)
{
	m_kd = std::make_shared<tl::Kd<t_real>>();

	std::ifstream ifstr(pcFile);
	if(!ifstr.is_open())
		return false;

	std::list<std::vector<t_real>> lstPoints;
	std::size_t iCurPoint = 0;
	while(!ifstr.eof())
	{
		std::string strLine;
		std::getline(ifstr, strLine);
		tl::trim(strLine);

		if(strLine.length() == 0)
			continue;

		if(strLine[0] == '#')
		{
			strLine[0] = ' ';
			m_mapParams.insert(tl::split_first(strLine, std::string(":"), 1));
			continue;
		}

		std::vector<t_real> vecSqw;
		tl::get_tokens<t_real>(strLine, std::string(" \t"), vecSqw);
		if(vecSqw.size() != 5)
		{
			tl::log_err("Need h,k,l,E,S data.");
			return false;
		}

		lstPoints.push_back(vecSqw);
		++iCurPoint;
	}

	tl::log_info("Loaded ",  iCurPoint, " S(q,w) points.");
	m_kd->Load(lstPoints, 4);
	tl::log_info("Generated k-d tree.");

	//std::ofstream ofstrkd("kd.dbg");
	//m_kd->GetRootNode()->print(ofstrkd);
	return true;
}


t_real SqwKdTree::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	// meV and rlu units will have equal scaling in the kd tree!
	std::vector<t_real> vechklE = {dh, dk, dl, dE};
	if(!m_kd->IsPointInGrid(vechklE))
		return 0.;

	std::vector<t_real> vec = m_kd->GetNearestNode(vechklE);

	/*t_real dDist = std::sqrt(std::pow(vec[0]-vechklE[0], 2.) +
			std::pow(vec[1]-vechklE[1], 2.) +
			std::pow(vec[2]-vechklE[2], 2.) +
			std::pow(vec[3]-vechklE[3], 2.));
	tl::log_info("Distance to node: ", dDist);*/

	//tl::log_info("Nearest node: ", vec[0], ", ", vec[1], ", ", vec[2], ", ", vec[3], ", ", vec[4]);
	return vec[4];
}


std::vector<SqwBase::t_var> SqwKdTree::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;

	return vecVars;
}


void SqwKdTree::SetVars(const std::vector<SqwBase::t_var>&)
{
}


SqwBase* SqwKdTree::shallow_copy() const
{
	SqwKdTree *pTree = new SqwKdTree();
	*static_cast<SqwBase*>(pTree) = *static_cast<const SqwBase*>(this);

	pTree->m_mapParams = m_mapParams;
	pTree->m_kd = m_kd;

	return pTree;
}





//------------------------------------------------------------------------------




SqwTable1d::SqwTable1d(const char* pcFile)
{
	m_bOk = false;
	if(pcFile)
		m_bOk = open(pcFile);
}


bool SqwTable1d::open(const char* pcFile)
{
	tl::log_debug("Loading \"", pcFile, "\"", ".");
	m_dat = std::make_shared<tl::DatFile<t_real>>(pcFile);
	m_bOk = m_dat->IsOk();
	CreateKd();
	return m_bOk;
}


void SqwTable1d::CreateKd()
{
	if(!m_bOk)
	{
		tl::log_err("Data table not yet loaded.");
		return;
	}

	m_kd = std::make_shared<tl::Kd<t_real>>();
	std::list<std::vector<t_real>> lstPoints;

	t_real minq = std::numeric_limits<t_real>::max();
	t_real maxq = -std::numeric_limits<t_real>::max();
	t_real minE = std::numeric_limits<t_real>::max();
	t_real maxE = -std::numeric_limits<t_real>::max();

	for(std::size_t iRow=0; iRow<m_dat->GetRowCount(); ++iRow)
	{
		t_real q = m_dat->GetColumn(m_qcol)[iRow];
		t_real E = m_dat->GetColumn(m_Ecol)[iRow];
		t_real S = m_dat->GetColumn(m_Scol)[iRow];

		minq = std::min(minq, q);
		maxq = std::max(maxq, q);
		minE = std::min(minE, E);
		maxE = std::max(maxE, E);

		lstPoints.emplace_back(std::vector<t_real>{{ q, E, S }});
	}

	tl::log_info("Loaded ", m_dat->GetRowCount(), " S(q,w) points.");
	tl::log_info("q range: ", minq, "..", maxq, ", E range: ", minE, "..", maxE, ".");

	m_kd->Load(lstPoints, 2);
	tl::log_info("Generated k-d tree.");
}


t_real SqwTable1d::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	if(!m_bOk)
		return 0.;

	// get reduced q
	dh -= m_G[0]; dk -= m_G[1]; dl -= m_G[2];
	t_real dq = std::sqrt(dh*dh + dk*dk + dl*dl);

	// meV and rlu units will have equal scaling in the kd tree!
	std::vector<t_real> vecqE{{dq, dE}};

	if(!m_kd->IsPointInGrid(vecqE))
		return 0.;

	std::vector<t_real> vec = m_kd->GetNearestNode(vecqE);
	//tl::log_info("Queried node: ", vecqE[0], ", ", vecqE[1],
	//	" -> nearest node: ", vec[0], ", ", vec[1], ", ", vec[2], ".");

	return vec[2];
}


std::vector<SqwBase::t_var> SqwTable1d::GetVars() const
{
	std::ostringstream ostr;
	ostr.precision(g_iPrec);
	ostr << m_G[0] << " " << m_G[1] << " " << m_G[2];


	std::vector<SqwBase::t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"q_column", "uint", tl::var_to_str(m_qcol)});
	vecVars.push_back(SqwBase::t_var{"E_column", "uint", tl::var_to_str(m_Ecol)});
	vecVars.push_back(SqwBase::t_var{"S_column", "uint", tl::var_to_str(m_Scol)});
	vecVars.push_back(SqwBase::t_var{"G", "vector", ostr.str()});

	return vecVars;
}


void SqwTable1d::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(vecVars.size() == 0)
		return;

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strVal = std::get<2>(var);

		if(strVar == "q_column") m_qcol = tl::str_to_var<decltype(m_qcol)>(strVal);
		else if(strVar == "E_column") m_Ecol = tl::str_to_var<decltype(m_Ecol)>(strVal);
		else if(strVar == "S_column") m_Scol = tl::str_to_var<decltype(m_Scol)>(strVal);
		else if(strVar == "G")
		{
			std::istringstream istr(strVal);
			istr >> m_G[0] >> m_G[1] >> m_G[2];
		}
	}

	CreateKd();
}


SqwBase* SqwTable1d::shallow_copy() const
{
	SqwTable1d *pTab = new SqwTable1d();
	*static_cast<SqwBase*>(pTab) = *static_cast<const SqwBase*>(this);

	pTab->m_dat = m_dat;
	pTab->m_kd = m_kd;

	return pTab;
}




//------------------------------------------------------------------------------




t_real SqwPhonon::phonon_disp(t_real dq, t_real da, t_real df)
{
	return std::abs(da*std::sin(dq*df));
}


void SqwPhonon::create()
{
#ifdef USE_RTREE
	m_rt = std::make_shared<tl::Rt<t_real,3,RT_ELEMS>>();
#else
	m_kd = std::make_shared<tl::Kd<t_real>>();
#endif

	const bool bSaveOnlyIndices = 1;
	destroy();

	if(m_vecBragg.size()==0 || m_vecLA.size()==0 || m_vecTA1.size()==0 || m_vecTA2.size()==0)
	{
		// examples
		m_vecBragg = tl::make_vec({1., 0., 0.});
		m_vecLA = tl::make_vec({1., 0., 0.});
		m_vecTA1 = tl::make_vec({0., 1., 0.});
		m_vecTA2 = tl::make_vec({0., 0., 1.});

		//m_bOk = 0;
		//return;
	}

	m_vecLA /= ublas::norm_2(m_vecLA);
	m_vecTA1 /= ublas::norm_2(m_vecTA1);
	m_vecTA2 /= ublas::norm_2(m_vecTA2);

	tl::log_info("LA: ", m_vecLA);
	tl::log_info("TA1: ", m_vecTA1);
	tl::log_info("TA2: ", m_vecTA2);

	std::list<std::vector<t_real>> lst;
	for(t_real dq=-1.; dq<1.; dq+=1./t_real(m_iNumqs))
	{
		ublas::vector<t_real> vecQLA = dq*m_vecLA;
		ublas::vector<t_real> vecQTA1 = dq*m_vecTA1;
		ublas::vector<t_real> vecQTA2 = dq*m_vecTA2;

		t_real dELA = phonon_disp(dq, m_dLA_amp, m_dLA_freq);
		t_real dETA1 = phonon_disp(dq, m_dTA1_amp, m_dTA1_freq);
		t_real dETA2 = phonon_disp(dq, m_dTA2_amp, m_dTA2_freq);

		t_real dLA_E_HWHM = m_dLA_E_HWHM;
		t_real dLA_q_HWHM = m_dLA_q_HWHM;
		t_real dTA1_E_HWHM = m_dTA1_E_HWHM;
		t_real dTA1_q_HWHM = m_dTA1_q_HWHM;
		t_real dTA2_E_HWHM = m_dTA2_E_HWHM;
		t_real dTA2_q_HWHM = m_dTA2_q_HWHM;

		t_real dLA_S0 = m_dLA_S0;
		t_real dTA1_S0 = m_dTA1_S0;
		t_real dTA2_S0 = m_dTA2_S0;

		if(bSaveOnlyIndices)
		{
			dTA1_E_HWHM = dTA1_q_HWHM = dTA1_S0 = -1.;
			dTA2_E_HWHM = dTA2_q_HWHM = dTA2_S0 = -2.;
			dLA_E_HWHM = dLA_q_HWHM = dLA_S0 = -3.;
		}

		// only generate exact phonon branches, no arcs
		if(m_iNumArc==0 || m_iNumArc==1)
		{
			lst.push_back(std::vector<t_real>({vecQLA[0]+m_vecBragg[0], vecQLA[1]+m_vecBragg[1], vecQLA[2]+m_vecBragg[2], dELA, dLA_S0, dLA_E_HWHM, dLA_q_HWHM}));
			lst.push_back(std::vector<t_real>({vecQTA1[0]+m_vecBragg[0], vecQTA1[1]+m_vecBragg[1], vecQTA1[2]+m_vecBragg[2], dETA1, dTA1_S0, dTA1_E_HWHM, dTA1_q_HWHM}));
			lst.push_back(std::vector<t_real>({vecQTA2[0]+m_vecBragg[0], vecQTA2[1]+m_vecBragg[1], vecQTA2[2]+m_vecBragg[2], dETA2, dTA2_S0, dTA2_E_HWHM, dTA2_q_HWHM}));
		}
		else
		{
			const t_real dArcMax = std::abs(tl::d2r(m_dArcMax));
			for(t_real dph=-dArcMax; dph<=dArcMax; dph+=1./t_real(m_iNumArc))
			for(t_real dth=-dArcMax; dth<=dArcMax; dth+=1./t_real(m_iNumArc))
			{
				// ta2
				ublas::vector<t_real> vecArcTA2 = tl::sph_shell(vecQTA2, dph, dth) + m_vecBragg;;
				lst.push_back(std::vector<t_real>({vecArcTA2[0], vecArcTA2[1], vecArcTA2[2], dETA2, dTA2_S0, dTA2_E_HWHM, dTA2_q_HWHM}));

				// ta1
				ublas::vector<t_real> vecArcTA1 = tl::sph_shell(vecQTA1, dph, dth) + m_vecBragg;;
				lst.push_back(std::vector<t_real>({vecArcTA1[0], vecArcTA1[1], vecArcTA1[2], dETA1, dTA1_S0, dTA1_E_HWHM, dTA1_q_HWHM}));

				// la
				ublas::vector<t_real> vecArcLA = tl::sph_shell(vecQLA, dph, dth) + m_vecBragg;;
				lst.push_back(std::vector<t_real>({vecArcLA[0], vecArcLA[1], vecArcLA[2], dELA, dLA_S0, dLA_E_HWHM, dLA_q_HWHM}));
			}
		}
	}

	tl::log_info("Generated ", lst.size(), " S(q,w) points.");
#ifdef USE_RTREE
	m_rt->Load(lst);
	tl::log_info("Generated R* tree.");
#else
	m_kd->Load(lst, 3);
	tl::log_info("Generated k-d tree.");
#endif

	m_bOk = 1;
}


void SqwPhonon::destroy()
{
#ifdef USE_RTREE
	m_rt->Unload();
#else
	m_kd->Unload();
#endif
}


SqwPhonon::SqwPhonon(const ublas::vector<t_real>& vecBragg,
	const ublas::vector<t_real>& vecTA1,
	const ublas::vector<t_real>& vecTA2,
	t_real dLA_amp, t_real dLA_freq, t_real dLA_E_HWHM, t_real dLA_q_HWHM, t_real dLA_S0,
	t_real dTA1_amp, t_real dTA1_freq, t_real dTA1_E_HWHM, t_real dTA1_q_HWHM, t_real dTA1_S0,
	t_real dTA2_amp, t_real dTA2_freq, t_real dTA2_E_HWHM, t_real dTA2_q_HWHM, t_real dTA2_S0,
	t_real dT)
		: m_vecBragg(vecBragg), m_vecLA(vecBragg),
		m_vecTA1(vecTA1), m_vecTA2(vecTA2),
		m_dLA_amp(dLA_amp), m_dLA_freq(dLA_freq),
		m_dLA_E_HWHM(dLA_E_HWHM), m_dLA_q_HWHM(dLA_q_HWHM),
		m_dLA_S0(dLA_S0),

		m_dTA1_amp(dTA1_amp), m_dTA1_freq(dTA1_freq),
		m_dTA1_E_HWHM(dTA1_E_HWHM), m_dTA1_q_HWHM(dTA1_q_HWHM),
		m_dTA1_S0(dTA1_S0),

		m_dTA2_amp(dTA2_amp), m_dTA2_freq(dTA2_freq), 
		m_dTA2_E_HWHM(dTA2_E_HWHM), m_dTA2_q_HWHM(dTA2_q_HWHM),
		m_dTA2_S0(dTA2_S0),

		m_dT(dT)
{
	create();
}


SqwPhonon::SqwPhonon(const char* pcFile)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr)
		tl::log_warn("Cannot open phonon config file \"", pcFile, "\".");

	// if a file is given, load the parameters
	if(!!ifstr)
	{
		std::string strLine;
		while(std::getline(ifstr, strLine))
		{
			std::vector<std::string> vecToks;
			tl::get_tokens<std::string>(strLine, std::string("=,"), vecToks);
			std::for_each(vecToks.begin(), vecToks.end(), [](std::string& str) {tl::trim(str); });

			if(vecToks.size() == 0) continue;

			//for(const auto& tok : vecToks) std::cout << tok << ", ";
			//std::cout << std::endl;

			if(vecToks[0] == "num_qs") m_iNumqs = tl::str_to_var<unsigned int>(vecToks[1]);
			if(vecToks[0] == "num_arc") m_iNumArc = tl::str_to_var<unsigned int>(vecToks[1]);
			if(vecToks[0] == "arc_max") m_dArcMax = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "G") m_vecLA = m_vecBragg = tl::make_vec({tl::str_to_var_parse<t_real>(vecToks[1]), tl::str_to_var_parse<t_real>(vecToks[2]), tl::str_to_var_parse<t_real>(vecToks[3])});
			else if(vecToks[0] == "TA1") m_vecTA1 = tl::make_vec({tl::str_to_var_parse<t_real>(vecToks[1]), tl::str_to_var_parse<t_real>(vecToks[2]), tl::str_to_var_parse<t_real>(vecToks[3])});
			else if(vecToks[0] == "TA2") m_vecTA2 = tl::make_vec({tl::str_to_var_parse<t_real>(vecToks[1]), tl::str_to_var_parse<t_real>(vecToks[2]), tl::str_to_var_parse<t_real>(vecToks[3])});

			else if(vecToks[0] == "LA_amp") m_dLA_amp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "LA_freq") m_dLA_freq = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "LA_E_HWHM") m_dLA_E_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "LA_q_HWHM") m_dLA_q_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "LA_S0") m_dLA_S0 = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "TA1_amp") m_dTA1_amp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA1_freq") m_dTA1_freq = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA1_E_HWHM") m_dTA1_E_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA1_q_HWHM") m_dTA1_q_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA1_S0") m_dTA1_S0 = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "TA2_amp") m_dTA2_amp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA2_freq") m_dTA2_freq = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA2_E_HWHM") m_dTA2_E_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA2_q_HWHM") m_dTA2_q_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "TA2_S0") m_dTA2_S0 = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "inc_amp") m_dIncAmp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "inc_sig") m_dIncSig = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "T") m_dT = tl::str_to_var_parse<t_real>(vecToks[1]);
		}
	}

	create();
}


t_real SqwPhonon::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	std::vector<t_real> vechklE = {dh, dk, dl, dE};
#ifdef USE_RTREE
	if(!m_rt->IsPointInGrid(vechklE)) return 0.;
	std::vector<t_real> vec = m_rt->GetNearestNode(vechklE);
#else
	if(!m_kd->IsPointInGrid(vechklE)) return 0.;
	std::vector<t_real> vec = m_kd->GetNearestNode(vechklE);
#endif

	//std::cout << "query: " << dh << " " << dk << " " << dl << " " << dE << std::endl;
	//std::cout << "nearest: " << vec[0] << " " << vec[1] << " " << vec[2] << " " << vec[3] << std::endl;

	t_real dE0 = vec[3];
	t_real dS = vec[4];
	t_real dT = m_dT;
	t_real dE_HWHM = vec[5];
	t_real dQ_HWHM = vec[6];

	// index, not value
	if(dE_HWHM < 0.)
	{
		if(tl::float_equal<t_real>(dE_HWHM, -1., 0.1))		// TA1
			dE_HWHM = m_dTA1_E_HWHM;
		else if(tl::float_equal<t_real>(dE_HWHM, -2., 0.1))	// TA2
			dE_HWHM = m_dTA2_E_HWHM;
		else if(tl::float_equal<t_real>(dE_HWHM, -3., 0.1))	// LA
			dE_HWHM = m_dLA_E_HWHM;
	}
	if(dQ_HWHM < 0.)
	{
		if(tl::float_equal<t_real>(dQ_HWHM, -1., 0.1))		// TA1
			dQ_HWHM = m_dTA1_q_HWHM;
		else if(tl::float_equal<t_real>(dQ_HWHM, -2., 0.1))	// TA2
			dQ_HWHM = m_dTA2_q_HWHM;
		else if(tl::float_equal<t_real>(dQ_HWHM, -3., 0.1))	// LA
			dQ_HWHM = m_dLA_q_HWHM;
	}
	if(dS < 0.)
	{
		if(tl::float_equal<t_real>(dS, -1., 0.1))		// TA1
			dS = m_dTA1_S0;
		else if(tl::float_equal<t_real>(dS, -2., 0.1))	// TA2
			dS = m_dTA2_S0;
		else if(tl::float_equal<t_real>(dS, -3., 0.1))	// LA
			dS = m_dLA_S0;
	}

	t_real dqDist = std::sqrt(std::pow(vec[0]-vechklE[0], 2.)
		+ std::pow(vec[1]-vechklE[1], 2.)
		+ std::pow(vec[2]-vechklE[2], 2.));

	t_real dInc = 0.;
	if(!tl::float_equal<t_real>(m_dIncAmp, 0.))
		dInc = tl::gauss_model<t_real>(dE, 0., m_dIncSig, m_dIncAmp, 0.);

	return dS * std::abs(tl::DHO_model<t_real>(dE, dT, dE0, dE_HWHM, 1., 0.))
		* tl::gauss_model<t_real>(dqDist, 0., dQ_HWHM*tl::get_HWHM2SIGMA<t_real>(), 1., 0.)
		+ dInc;
}


std::vector<SqwBase::t_var> SqwPhonon::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"num_qs", "uint", tl::var_to_str(m_iNumqs)});
	vecVars.push_back(SqwBase::t_var{"num_arc", "uint", tl::var_to_str(m_iNumArc)});
	vecVars.push_back(SqwBase::t_var{"arc_max", "real", tl::var_to_str(m_dArcMax)});

	vecVars.push_back(SqwBase::t_var{"G", "vector", vec_to_str(m_vecBragg)});
	vecVars.push_back(SqwBase::t_var{"TA1", "vector", vec_to_str(m_vecTA1)});
	vecVars.push_back(SqwBase::t_var{"TA2", "vector", vec_to_str(m_vecTA2)});

	vecVars.push_back(SqwBase::t_var{"LA_amp", "real", tl::var_to_str(m_dLA_amp)});
	vecVars.push_back(SqwBase::t_var{"LA_freq", "real", tl::var_to_str(m_dLA_freq)});
	vecVars.push_back(SqwBase::t_var{"LA_E_HWHM", "real", tl::var_to_str(m_dLA_E_HWHM)});
	vecVars.push_back(SqwBase::t_var{"LA_q_HWHM", "real", tl::var_to_str(m_dLA_q_HWHM)});
	vecVars.push_back(SqwBase::t_var{"LA_S0", "real", tl::var_to_str(m_dLA_S0)});

	vecVars.push_back(SqwBase::t_var{"TA1_amp", "real", tl::var_to_str(m_dTA1_amp)});
	vecVars.push_back(SqwBase::t_var{"TA1_freq", "real", tl::var_to_str(m_dTA1_freq)});
	vecVars.push_back(SqwBase::t_var{"TA1_E_HWHM", "real", tl::var_to_str(m_dTA1_E_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA1_q_HWHM", "real", tl::var_to_str(m_dTA1_q_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA1_S0", "real", tl::var_to_str(m_dTA1_S0)});

	vecVars.push_back(SqwBase::t_var{"TA2_amp", "real", tl::var_to_str(m_dTA2_amp)});
	vecVars.push_back(SqwBase::t_var{"TA2_freq", "real", tl::var_to_str(m_dTA2_freq)});
	vecVars.push_back(SqwBase::t_var{"TA2_E_HWHM", "real", tl::var_to_str(m_dTA2_E_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA2_q_HWHM", "real", tl::var_to_str(m_dTA2_q_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA2_S0", "real", tl::var_to_str(m_dTA2_S0)});

	vecVars.push_back(SqwBase::t_var{"inc_amp", "real", tl::var_to_str(m_dIncAmp)});
	vecVars.push_back(SqwBase::t_var{"inc_sig", "real", tl::var_to_str(m_dIncSig)});

	vecVars.push_back(SqwBase::t_var{"T", "real", tl::var_to_str(m_dT)});

	return vecVars;
}


void SqwPhonon::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(vecVars.size() == 0)
		return;

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strVal = std::get<2>(var);

		if(strVar == "num_qs") m_iNumqs = tl::str_to_var<decltype(m_iNumqs)>(strVal);
		if(strVar == "num_arc") m_iNumArc = tl::str_to_var<decltype(m_iNumArc)>(strVal);
		if(strVar == "arc_max") m_dArcMax = tl::str_to_var<decltype(m_dArcMax)>(strVal);

		else if(strVar == "G") m_vecLA = m_vecBragg = str_to_vec<decltype(m_vecBragg)>(strVal);
		else if(strVar == "TA1") m_vecTA1 = str_to_vec<decltype(m_vecTA1)>(strVal);
		else if(strVar == "TA2") m_vecTA2 = str_to_vec<decltype(m_vecTA2)>(strVal);

		else if(strVar == "LA_amp") m_dLA_amp = tl::str_to_var<decltype(m_dLA_amp)>(strVal);
		else if(strVar == "LA_freq") m_dLA_freq = tl::str_to_var<decltype(m_dLA_freq)>(strVal);
		else if(strVar == "LA_E_HWHM") m_dLA_E_HWHM = tl::str_to_var<decltype(m_dLA_E_HWHM)>(strVal);
		else if(strVar == "LA_q_HWHM") m_dLA_q_HWHM = tl::str_to_var<decltype(m_dLA_q_HWHM)>(strVal);
		else if(strVar == "LA_S0") m_dLA_S0 = tl::str_to_var<decltype(m_dLA_S0)>(strVal);

		else if(strVar == "TA1_amp") m_dTA1_amp = tl::str_to_var<decltype(m_dTA1_amp)>(strVal);
		else if(strVar == "TA1_freq") m_dTA1_freq = tl::str_to_var<decltype(m_dTA1_freq)>(strVal);
		else if(strVar == "TA1_E_HWHM") m_dTA1_E_HWHM = tl::str_to_var<decltype(m_dTA1_E_HWHM)>(strVal);
		else if(strVar == "TA1_q_HWHM") m_dTA1_q_HWHM = tl::str_to_var<decltype(m_dTA1_q_HWHM)>(strVal);
		else if(strVar == "TA1_S0") m_dTA1_S0 = tl::str_to_var<decltype(m_dTA1_S0)>(strVal);

		else if(strVar == "TA2_amp") m_dTA2_amp = tl::str_to_var<decltype(m_dTA2_amp)>(strVal);
		else if(strVar == "TA2_freq") m_dTA2_freq = tl::str_to_var<decltype(m_dTA2_freq)>(strVal);
		else if(strVar == "TA2_E_HWHM") m_dTA2_E_HWHM = tl::str_to_var<decltype(m_dTA2_E_HWHM)>(strVal);
		else if(strVar == "TA2_q_HWHM") m_dTA2_q_HWHM = tl::str_to_var<decltype(m_dTA2_q_HWHM)>(strVal);
		else if(strVar == "TA2_S0") m_dTA2_S0 = tl::str_to_var<decltype(m_dTA2_S0)>(strVal);

		else if(strVar == "inc_amp") m_dIncAmp = tl::str_to_var<decltype(m_dIncAmp)>(strVal);
		else if(strVar == "inc_sig") m_dIncSig = tl::str_to_var<decltype(m_dIncSig)>(strVal);

		else if(strVar == "T") m_dT = tl::str_to_var<decltype(m_dT)>(strVal);
	}

	bool bRecreateTree = 0;

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		if(strVar != "T" && strVar.find("HWHM") == std::string::npos &&
			strVar.find("inc") == std::string::npos &&
			strVar.find("S0") == std::string::npos)
			bRecreateTree = 1;
	}

	if(bRecreateTree)
		create();
}


SqwBase* SqwPhonon::shallow_copy() const
{
	SqwPhonon *pCpy = new SqwPhonon();
	*static_cast<SqwBase*>(pCpy) = *static_cast<const SqwBase*>(this);

#ifdef USE_RTREE
	pCpy->m_rt = m_rt;
#else
	pCpy->m_kd = m_kd;
#endif
	pCpy->m_iNumqs = m_iNumqs;
	pCpy->m_iNumArc = m_iNumArc;
	pCpy->m_dArcMax = m_dArcMax;

	pCpy->m_vecBragg = m_vecBragg;
	pCpy->m_vecLA = m_vecLA;
	pCpy->m_vecTA1 = m_vecTA1;
	pCpy->m_vecTA2 = m_vecTA2;

	pCpy->m_dLA_amp = m_dLA_amp;
	pCpy->m_dLA_freq = m_dLA_freq;
	pCpy->m_dLA_E_HWHM = m_dLA_E_HWHM;
	pCpy->m_dLA_q_HWHM = m_dLA_q_HWHM;
	pCpy->m_dLA_S0 = m_dLA_S0;

	pCpy->m_dTA1_amp = m_dTA1_amp;
	pCpy->m_dTA1_freq = m_dTA1_freq;
	pCpy->m_dTA1_E_HWHM = m_dTA1_E_HWHM;
	pCpy->m_dTA1_q_HWHM = m_dTA1_q_HWHM;
	pCpy->m_dTA1_S0 = m_dTA1_S0;

	pCpy->m_dTA2_amp = m_dTA2_amp;
	pCpy->m_dTA2_freq = m_dTA2_freq;
	pCpy->m_dTA2_E_HWHM = m_dTA2_E_HWHM;
	pCpy->m_dTA2_q_HWHM = m_dTA2_q_HWHM;
	pCpy->m_dTA2_S0 = m_dTA2_S0;

	pCpy->m_dIncAmp = m_dIncAmp;
	pCpy->m_dIncSig = m_dIncSig;

	pCpy->m_dT = m_dT;
	return pCpy;
}



//------------------------------------------------------------------------------



// for model compatibility testing
#define BRANCH_PREFIX ""
//#define BRANCH_PREFIX "TA2_"

t_real SqwPhononSingleBranch::phonon_disp(t_real dq, t_real da, t_real df)
{
	return std::abs(da*std::sin(dq*df));
}


SqwPhononSingleBranch::SqwPhononSingleBranch(const char* pcFile)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr)
		tl::log_warn("Cannot open phonon config file \"", pcFile, "\".");

	// if a file is given, load the parameters
	if(!!ifstr)
	{
		std::string strLine;
		while(std::getline(ifstr, strLine))
		{
			std::vector<std::string> vecToks;
			tl::get_tokens<std::string>(strLine, std::string("=,"), vecToks);
			std::for_each(vecToks.begin(), vecToks.end(), [](std::string& str) {tl::trim(str); });

			if(vecToks.size() == 0) continue;

			if(vecToks[0] == "G") m_vecBragg = tl::make_vec({tl::str_to_var_parse<t_real>(vecToks[1]), tl::str_to_var_parse<t_real>(vecToks[2]), tl::str_to_var_parse<t_real>(vecToks[3])});

			else if(vecToks[0] == BRANCH_PREFIX"amp") m_damp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == BRANCH_PREFIX"freq") m_dfreq = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == BRANCH_PREFIX"E_HWHM") m_dHWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == BRANCH_PREFIX"S0") m_dS0 = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "inc_amp") m_dIncAmp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "inc_sig") m_dIncSig = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "T") m_dT = tl::str_to_var_parse<t_real>(vecToks[1]);
		}
	}

	// just use 100 if nothing else is defined
	if(m_vecBragg.size() < 3)
		m_vecBragg = tl::make_vec({1., 0., 0.});

	m_bOk = 1;
}


/**
 * dispersion E(Q)
 */
std::tuple<std::vector<t_real>, std::vector<t_real>>
SqwPhononSingleBranch::disp(t_real dh, t_real dk, t_real dl) const
{
	dh -= m_vecBragg[0];
	dk -= m_vecBragg[1];
	dl -= m_vecBragg[2];

	t_real dq = std::sqrt(dh*dh + dk*dk + dl*dl);
	t_real dE0 = phonon_disp(dq, m_damp, m_dfreq);
	t_real dWeight = t_real(1);

	return std::make_tuple(std::vector<t_real>({dE0, -dE0}),
		std::vector<t_real>({dWeight, dWeight}));
}


/**
 * dynamical structure factor S(Q,E)
 */
t_real SqwPhononSingleBranch::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	std::vector<t_real> vecE0, vecW;
	std::tie(vecE0, vecW) = disp(dh, dk, dl);

	t_real dInc = 0.;
	if(!tl::float_equal<t_real>(m_dIncAmp, 0.))
		dInc = tl::gauss_model<t_real>(dE, 0., m_dIncSig, m_dIncAmp, 0.);

	return std::abs(tl::DHO_model<t_real>(dE, m_dT, vecE0[0], m_dHWHM, m_dS0*vecW[0], 0.)) + dInc;
}


std::vector<SqwBase::t_var> SqwPhononSingleBranch::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"G", "vector", vec_to_str(m_vecBragg)});

	vecVars.push_back(SqwBase::t_var{BRANCH_PREFIX"amp", "real", tl::var_to_str(m_damp)});
	vecVars.push_back(SqwBase::t_var{BRANCH_PREFIX"freq", "real", tl::var_to_str(m_dfreq)});
	vecVars.push_back(SqwBase::t_var{BRANCH_PREFIX"E_HWHM", "real", tl::var_to_str(m_dHWHM)});
	vecVars.push_back(SqwBase::t_var{BRANCH_PREFIX"S0", "real", tl::var_to_str(m_dS0)});

	vecVars.push_back(SqwBase::t_var{"inc_amp", "real", tl::var_to_str(m_dIncAmp)});
	vecVars.push_back(SqwBase::t_var{"inc_sig", "real", tl::var_to_str(m_dIncSig)});

	vecVars.push_back(SqwBase::t_var{"T", "real", tl::var_to_str(m_dT)});

	return vecVars;
}


void SqwPhononSingleBranch::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(vecVars.size() == 0)
		return;

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strVal = std::get<2>(var);

		if(strVar == "G") m_vecBragg = str_to_vec<decltype(m_vecBragg)>(strVal);

		else if(strVar == BRANCH_PREFIX"amp") m_damp = tl::str_to_var<decltype(m_damp)>(strVal);
		else if(strVar == BRANCH_PREFIX"freq") m_dfreq = tl::str_to_var<decltype(m_dfreq)>(strVal);
		else if(strVar == BRANCH_PREFIX"E_HWHM") m_dHWHM = tl::str_to_var<decltype(m_dHWHM)>(strVal);
		else if(strVar == BRANCH_PREFIX"S0") m_dS0 = tl::str_to_var<decltype(m_dS0)>(strVal);

		else if(strVar == "inc_amp") m_dIncAmp = tl::str_to_var<decltype(m_dIncAmp)>(strVal);
		else if(strVar == "inc_sig") m_dIncSig = tl::str_to_var<decltype(m_dIncSig)>(strVal);

		else if(strVar == "T") m_dT = tl::str_to_var<decltype(m_dT)>(strVal);
	}
}


SqwBase* SqwPhononSingleBranch::shallow_copy() const
{
	SqwPhononSingleBranch *pCpy = new SqwPhononSingleBranch();
	*static_cast<SqwBase*>(pCpy) = *static_cast<const SqwBase*>(this);

	pCpy->m_vecBragg = m_vecBragg;

	pCpy->m_damp = m_damp;
	pCpy->m_dfreq = m_dfreq;
	pCpy->m_dHWHM = m_dHWHM;
	pCpy->m_dS0 = m_dS0;

	pCpy->m_dIncAmp = m_dIncAmp;
	pCpy->m_dIncSig = m_dIncSig;

	pCpy->m_dT = m_dT;
	return pCpy;
}



//------------------------------------------------------------------------------



t_real SqwMagnon::ferro_disp(t_real dq, t_real dD, t_real doffs)
{
	return dq*dq * dD + doffs;
}


t_real SqwMagnon::antiferro_disp(t_real dq, t_real dD, t_real doffs)
{
	return std::abs(dq)*dD + doffs;
}


SqwMagnon::SqwMagnon(const char* pcFile)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr)
		tl::log_warn("Cannot open magnon config file \"", pcFile, "\".");

	// if a file is given, load the parameters
	if(!!ifstr)
	{
		std::string strLine;
		while(std::getline(ifstr, strLine))
		{
			std::vector<std::string> vecToks;
			tl::get_tokens<std::string>(strLine, std::string("=,"), vecToks);
			std::for_each(vecToks.begin(), vecToks.end(), [](std::string& str) {tl::trim(str); });

			if(vecToks.size() == 0) continue;

			if(vecToks[0] == "G") m_vecBragg = tl::make_vec({tl::str_to_var_parse<t_real>(vecToks[1]), tl::str_to_var_parse<t_real>(vecToks[2]), tl::str_to_var_parse<t_real>(vecToks[3])});
			else if(vecToks[0] == "disp") m_iWhichDisp = tl::str_to_var<decltype(m_iWhichDisp)>(vecToks[1]);

			else if(vecToks[0] == "D") m_dD = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "offs") m_dOffs = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "E_HWHM") m_dE_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "S0") m_dS0 = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "inc_amp") m_dIncAmp = tl::str_to_var_parse<t_real>(vecToks[1]);
			else if(vecToks[0] == "inc_sig") m_dIncSig = tl::str_to_var_parse<t_real>(vecToks[1]);

			else if(vecToks[0] == "T") m_dT = tl::str_to_var_parse<t_real>(vecToks[1]);
		}
	}

	// just use 100 if nothing else is defined
	if(m_vecBragg.size() < 3)
		m_vecBragg = tl::make_vec({1., 0., 0.});

	m_bOk = 1;
}


/**
 * dispersion E(Q)
 */
std::tuple<std::vector<t_real>, std::vector<t_real>>
SqwMagnon::disp(t_real dh, t_real dk, t_real dl) const
{
	dh -= m_vecBragg[0];
	dk -= m_vecBragg[1];
	dl -= m_vecBragg[2];

	t_real (*pDisp)(t_real, t_real, t_real) = nullptr;
	switch(m_iWhichDisp)
	{
		case 0: pDisp = &ferro_disp; break;
		case 1: pDisp = &antiferro_disp; break;
	}

	if(!pDisp)
		return std::make_tuple(std::vector<t_real>(), std::vector<t_real>());

	t_real dq = std::sqrt(dh*dh + dk*dk + dl*dl);
	t_real dE = pDisp(dq, m_dD, m_dOffs);
	t_real dW = t_real(1);

	return std::make_tuple(std::vector<t_real>({dE, -dE}),
		std::vector<t_real>({dW, dW}));
}


/**
 * dynamical structure factor S(Q,E)
 */
t_real SqwMagnon::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
{
	std::vector<t_real> vecE0, vecW;
	std::tie(vecE0, vecW) = disp(dh, dk, dl);

	t_real dInc = 0.;
	if(!tl::float_equal<t_real>(m_dIncAmp, 0.))
		dInc = tl::gauss_model<t_real>(dE, 0., m_dIncSig, m_dIncAmp, 0.);

	t_real dS = 0;
	if(vecE0.size())
	{
		for(std::size_t i=0; i<vecE0.size(); ++i)
			dS += std::abs(tl::DHO_model<t_real>(dE, m_dT, vecE0[i], m_dE_HWHM, vecW[i], 0.));
		dS *= m_dS0;
	}

	return dS + dInc;
}


std::vector<SqwBase::t_var> SqwMagnon::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"G", "vector", vec_to_str(m_vecBragg)});
	vecVars.push_back(SqwBase::t_var{"disp", "uint", tl::var_to_str(m_iWhichDisp)});

	vecVars.push_back(SqwBase::t_var{"D", "real", tl::var_to_str(m_dD)});
	vecVars.push_back(SqwBase::t_var{"offs", "real", tl::var_to_str(m_dOffs)});
	vecVars.push_back(SqwBase::t_var{"E_HWHM", "real", tl::var_to_str(m_dE_HWHM)});
	vecVars.push_back(SqwBase::t_var{"S0", "real", tl::var_to_str(m_dS0)});

	vecVars.push_back(SqwBase::t_var{"inc_amp", "real", tl::var_to_str(m_dIncAmp)});
	vecVars.push_back(SqwBase::t_var{"inc_sig", "real", tl::var_to_str(m_dIncSig)});

	vecVars.push_back(SqwBase::t_var{"T", "real", tl::var_to_str(m_dT)});

	return vecVars;
}


void SqwMagnon::SetVars(const std::vector<SqwBase::t_var>& vecVars)
{
	if(vecVars.size() == 0)
		return;

	for(const SqwBase::t_var& var : vecVars)
	{
		const std::string& strVar = std::get<0>(var);
		const std::string& strVal = std::get<2>(var);

		if(strVar == "G") m_vecBragg = str_to_vec<decltype(m_vecBragg)>(strVal);
		else if(strVar == "disp") m_iWhichDisp = tl::str_to_var<decltype(m_iWhichDisp)>(strVal);

		else if(strVar == "D") m_dD = tl::str_to_var<decltype(m_dD)>(strVal);
		else if(strVar == "offs") m_dOffs = tl::str_to_var<decltype(m_dOffs)>(strVal);
		else if(strVar == "E_HWHM") m_dE_HWHM = tl::str_to_var<decltype(m_dE_HWHM)>(strVal);
		else if(strVar == "S0") m_dS0 = tl::str_to_var<decltype(m_dS0)>(strVal);

		else if(strVar == "inc_amp") m_dIncAmp = tl::str_to_var<decltype(m_dIncAmp)>(strVal);
		else if(strVar == "inc_sig") m_dIncSig = tl::str_to_var<decltype(m_dIncSig)>(strVal);

		else if(strVar == "T") m_dT = tl::str_to_var<decltype(m_dT)>(strVal);
	}
}


SqwBase* SqwMagnon::shallow_copy() const
{
	SqwMagnon *pCpy = new SqwMagnon();
	*static_cast<SqwBase*>(pCpy) = *static_cast<const SqwBase*>(this);

	pCpy->m_vecBragg = m_vecBragg;
	pCpy->m_iWhichDisp = m_iWhichDisp;

	pCpy->m_dD = m_dD;
	pCpy->m_dOffs = m_dOffs;
	pCpy->m_dS0 = m_dS0;
	pCpy->m_dE_HWHM = m_dE_HWHM;

	pCpy->m_dIncAmp = m_dIncAmp;
	pCpy->m_dIncSig = m_dIncSig;

	pCpy->m_dT = m_dT;

	return pCpy;
}
