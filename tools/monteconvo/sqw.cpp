/**
 * monte carlo convolution tool
 * @author tweber
 * @date 2015, 2016
 * @license GPLv2
 */

#include "sqw.h"
#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/math/math.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/neutrons.h"
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

template<class t_vec>
static std::string vec_to_str(const t_vec& vec)
{
	std::ostringstream ostr;
	for(const typename t_vec::value_type& t : vec)
		ostr << t << " ";
	return ostr.str();
}

template<class t_vec>
static t_vec str_to_vec(const std::string& str)
{
	typedef typename t_vec::value_type T;

	std::vector<T> vec0;
	tl::get_tokens<T, std::string, std::vector<T>>(str, " \t", vec0);

	t_vec vec(vec0.size());
	for(unsigned int i=0; i<vec0.size(); ++i)
		vec[i] = vec0[i];
	return vec;
}


//------------------------------------------------------------------------------


t_real SqwPhonon::disp(t_real dq, t_real da, t_real df)
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
		m_bOk = 0;
		return;
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

		vecQLA += m_vecBragg;
		vecQTA1 += m_vecBragg;
		vecQTA2 += m_vecBragg;

		t_real dELA = disp(dq, m_dLA_amp, m_dLA_freq);
		t_real dETA1 = disp(dq, m_dTA1_amp, m_dTA1_freq);
		t_real dETA2 = disp(dq, m_dTA2_amp, m_dTA2_freq);


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
			lst.push_back(std::vector<t_real>({vecQLA[0], vecQLA[1], vecQLA[2], dELA, dLA_S0, dLA_E_HWHM, dLA_q_HWHM}));
			lst.push_back(std::vector<t_real>({vecQTA1[0], vecQTA1[1], vecQTA1[2], dETA1, dTA1_S0, dTA1_E_HWHM, dTA1_q_HWHM}));
			lst.push_back(std::vector<t_real>({vecQTA2[0], vecQTA2[1], vecQTA2[2], dETA2, dTA2_S0, dTA2_E_HWHM, dTA2_q_HWHM}));
		}
		else
		{
			const t_real dArcMax = std::abs(tl::d2r(m_dArcMax));
			for(t_real dph=-dArcMax; dph<=dArcMax; dph+=1./t_real(m_iNumArc))
			{
				// ta2
				ublas::vector<t_real> vecArcTA2TA1 = tl::arc(vecQTA2, vecQTA1, dph);
				ublas::vector<t_real> vecArcTA2LA = tl::arc(vecQTA2, vecQLA, dph);
				lst.push_back(std::vector<t_real>({vecArcTA2TA1[0], vecArcTA2TA1[1], vecArcTA2TA1[2], dETA2, dTA2_S0, dTA2_E_HWHM, dTA2_q_HWHM}));
				lst.push_back(std::vector<t_real>({vecArcTA2LA[0], vecArcTA2LA[1], vecArcTA2LA[2], dETA2, dTA2_S0, dTA2_E_HWHM, dTA2_q_HWHM}));

				// ta1
				ublas::vector<t_real> vecArcTA1TA2 = tl::arc(vecQTA1, vecQTA2, dph);
				ublas::vector<t_real> vecArcTA1LA = tl::arc(vecQTA1, vecQLA, dph);
				lst.push_back(std::vector<t_real>({vecArcTA1TA2[0], vecArcTA1TA2[1], vecArcTA1TA2[2], dETA1, dTA1_S0, dTA1_E_HWHM, dTA1_q_HWHM}));
				lst.push_back(std::vector<t_real>({vecArcTA1LA[0], vecArcTA1LA[1], vecArcTA1LA[2], dETA1, dTA1_S0, dTA1_E_HWHM, dTA1_q_HWHM}));

				// la
				ublas::vector<t_real> vecArcLATA1 = tl::arc(vecQLA, vecQTA1, dph);
				ublas::vector<t_real> vecArcLATA2 = tl::arc(vecQLA, vecQTA2, dph);
				lst.push_back(std::vector<t_real>({vecArcLATA1[0], vecArcLATA1[1], vecArcLATA1[2], dELA, dLA_S0, dLA_E_HWHM, dLA_q_HWHM}));
				lst.push_back(std::vector<t_real>({vecArcLATA2[0], vecArcLATA2[1], vecArcLATA2[2], dELA, dLA_S0, dLA_E_HWHM, dLA_q_HWHM}));
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
	{
		tl::log_err("Cannot open phonon config file \"", pcFile, "\".");
		return;
	}

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
		* tl::gauss_model<t_real>(dqDist, 0., dQ_HWHM*tl::HWHM2SIGMA, 1., 0.)
		+ dInc;
}

std::vector<SqwBase::t_var> SqwPhonon::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"num_qs", "uint", tl::var_to_str(m_iNumqs)});
	vecVars.push_back(SqwBase::t_var{"num_arc", "uint", tl::var_to_str(m_iNumArc)});
	vecVars.push_back(SqwBase::t_var{"arc_max", "t_real", tl::var_to_str(m_dArcMax)});

	vecVars.push_back(SqwBase::t_var{"G", "vector", vec_to_str(m_vecBragg)});
	vecVars.push_back(SqwBase::t_var{"TA1", "vector", vec_to_str(m_vecTA1)});
	vecVars.push_back(SqwBase::t_var{"TA2", "vector", vec_to_str(m_vecTA2)});

	vecVars.push_back(SqwBase::t_var{"LA_amp", "t_real", tl::var_to_str(m_dLA_amp)});
	vecVars.push_back(SqwBase::t_var{"LA_freq", "t_real", tl::var_to_str(m_dLA_freq)});
	vecVars.push_back(SqwBase::t_var{"LA_E_HWHM", "t_real", tl::var_to_str(m_dLA_E_HWHM)});
	vecVars.push_back(SqwBase::t_var{"LA_q_HWHM", "t_real", tl::var_to_str(m_dLA_q_HWHM)});
	vecVars.push_back(SqwBase::t_var{"LA_S0", "t_real", tl::var_to_str(m_dLA_S0)});

	vecVars.push_back(SqwBase::t_var{"TA1_amp", "t_real", tl::var_to_str(m_dTA1_amp)});
	vecVars.push_back(SqwBase::t_var{"TA1_freq", "t_real", tl::var_to_str(m_dTA1_freq)});
	vecVars.push_back(SqwBase::t_var{"TA1_E_HWHM", "t_real", tl::var_to_str(m_dTA1_E_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA1_q_HWHM", "t_real", tl::var_to_str(m_dTA1_q_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA1_S0", "t_real", tl::var_to_str(m_dTA1_S0)});

	vecVars.push_back(SqwBase::t_var{"TA2_amp", "t_real", tl::var_to_str(m_dTA2_amp)});
	vecVars.push_back(SqwBase::t_var{"TA2_freq", "t_real", tl::var_to_str(m_dTA2_freq)});
	vecVars.push_back(SqwBase::t_var{"TA2_E_HWHM", "t_real", tl::var_to_str(m_dTA2_E_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA2_q_HWHM", "t_real", tl::var_to_str(m_dTA2_q_HWHM)});
	vecVars.push_back(SqwBase::t_var{"TA2_S0", "t_real", tl::var_to_str(m_dTA2_S0)});

	vecVars.push_back(SqwBase::t_var{"inc_amp", "t_real", tl::var_to_str(m_dIncAmp)});
	vecVars.push_back(SqwBase::t_var{"inc_sig", "t_real", tl::var_to_str(m_dIncSig)});

	vecVars.push_back(SqwBase::t_var{"T", "t_real", tl::var_to_str(m_dT)});

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

	//std::cout << "hwhm = " << m_dTA2_E_HWHM << std::endl;
	//std::cout << "T = " << m_dT << std::endl;
	//std::cout << "amp = " << m_dTA2_amp << std::endl;
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


t_real SqwMagnon::ferro_disp(t_real dq, t_real dD, t_real doffs)
{
	return dq*dq * dD + doffs;
}

t_real SqwMagnon::antiferro_disp(t_real dq, t_real dD, t_real doffs)
{
	return std::abs(dq)*dD + doffs;
}

void SqwMagnon::create()
{
#ifdef USE_RTREE
	m_rt = std::make_shared<tl::Rt<t_real,3,RT_ELEMS>>();
#else
	m_kd = std::make_shared<tl::Kd<t_real>>();
#endif

	destroy();

	if(m_vecBragg.size() == 0)
	{
		m_bOk = 0;
		return;
	}

	std::list<std::vector<t_real>> lst;

	const t_real MAX_Q = 1.;

	for(t_real q=0.; q<MAX_Q; q += MAX_Q/t_real(m_iNumPoints))
	{
		t_real dE = 0.;
		switch(m_iWhichDisp)
		{
			case 0: dE = ferro_disp(q, m_dD, m_dOffs); break;
			case 1: dE = antiferro_disp(q, m_dD, m_dOffs); break;
		}

		for(unsigned int iPhi=0; iPhi<m_iNumPoints; ++iPhi)
		{
			for(unsigned int iTh=0; iTh<m_iNumPoints; ++iTh)
			{
				t_real phi = tl::lerp(0., 2.*tl::get_pi<t_real>(), t_real(iPhi)/t_real(m_iNumPoints));
				t_real theta = tl::lerp(0., tl::get_pi<t_real>(), t_real(iTh)/t_real(m_iNumPoints));

				t_real qx, qy, qz;
				std::tie(qx, qy, qz) = tl::sph_to_cart(q, phi, theta);
				qx += m_vecBragg[0];
				qy += m_vecBragg[1];
				qz += m_vecBragg[2];

				lst.push_back(std::vector<t_real>({qx, qy, qz, dE, m_dS0, m_dE_HWHM, m_dq_HWHM}));
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

void SqwMagnon::destroy()
{
#ifdef USE_RTREE
	m_rt->Unload();
#else
	m_kd->Unload();
#endif
}

SqwMagnon::SqwMagnon(const char* pcFile)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr)
	{
		tl::log_err("Cannot open magnon config file \"", pcFile, "\".");
		return;
	}

	std::string strLine;
	while(std::getline(ifstr, strLine))
	{
		std::vector<std::string> vecToks;
		tl::get_tokens<std::string>(strLine, std::string("=,"), vecToks);
		std::for_each(vecToks.begin(), vecToks.end(), [](std::string& str) {tl::trim(str); });

		if(vecToks.size() == 0) continue;

		//for(const auto& tok : vecToks) std::cout << tok << ", ";
		//std::cout << std::endl;

		if(vecToks[0] == "num_points") m_iNumPoints = tl::str_to_var<unsigned int>(vecToks[1]);

		else if(vecToks[0] == "G") m_vecBragg = tl::make_vec({tl::str_to_var_parse<t_real>(vecToks[1]), tl::str_to_var_parse<t_real>(vecToks[2]), tl::str_to_var_parse<t_real>(vecToks[3])});
		else if(vecToks[0] == "disp") m_iWhichDisp = tl::str_to_var<decltype(m_iWhichDisp)>(vecToks[1]);

		else if(vecToks[0] == "D") m_dD = tl::str_to_var_parse<t_real>(vecToks[1]);
		else if(vecToks[0] == "offs") m_dOffs = tl::str_to_var_parse<t_real>(vecToks[1]);
		else if(vecToks[0] == "E_HWHM") m_dE_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
		else if(vecToks[0] == "q_HWHM") m_dq_HWHM = tl::str_to_var_parse<t_real>(vecToks[1]);
		else if(vecToks[0] == "S0") m_dS0 = tl::str_to_var_parse<t_real>(vecToks[1]);

		else if(vecToks[0] == "inc_amp") m_dIncAmp = tl::str_to_var_parse<t_real>(vecToks[1]);
		else if(vecToks[0] == "inc_sig") m_dIncSig = tl::str_to_var_parse<t_real>(vecToks[1]);

		else if(vecToks[0] == "T") m_dT = tl::str_to_var_parse<t_real>(vecToks[1]);
	}

	create();
}

t_real SqwMagnon::operator()(t_real dh, t_real dk, t_real dl, t_real dE) const
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
	t_real dS0 = vec[4];
	t_real dT = m_dT;
	t_real dE_HWHM = vec[5];
	t_real dQ_HWHM = vec[6];

	t_real dqDist = std::sqrt(std::pow(vec[0]-vechklE[0], 2.)
		+ std::pow(vec[1]-vechklE[1], 2.)
		+ std::pow(vec[2]-vechklE[2], 2.));

	t_real dInc = 0.;
	if(!tl::float_equal<t_real>(m_dIncAmp, 0.))
		dInc = tl::gauss_model<t_real>(dE, 0., m_dIncSig, m_dIncAmp, 0.);

	t_real dS = dS0 * std::abs(tl::DHO_model<t_real>(dE, dT, dE0, dE_HWHM, 1., 0.))
		* tl::gauss_model<t_real>(dqDist, 0., dQ_HWHM*tl::HWHM2SIGMA, 1., 0.)
		+ dInc;

	return dS;
}

std::vector<SqwBase::t_var> SqwMagnon::GetVars() const
{
	std::vector<SqwBase::t_var> vecVars;

	vecVars.push_back(SqwBase::t_var{"num_points", "uint", tl::var_to_str(m_iNumPoints)});

	vecVars.push_back(SqwBase::t_var{"G", "vector", vec_to_str(m_vecBragg)});
	vecVars.push_back(SqwBase::t_var{"disp", "uint", tl::var_to_str(m_iWhichDisp)});

	vecVars.push_back(SqwBase::t_var{"D", "t_real", tl::var_to_str(m_dD)});
	vecVars.push_back(SqwBase::t_var{"offs", "t_real", tl::var_to_str(m_dOffs)});
	vecVars.push_back(SqwBase::t_var{"E_HWHM", "t_real", tl::var_to_str(m_dE_HWHM)});
	vecVars.push_back(SqwBase::t_var{"q_HWHM", "t_real", tl::var_to_str(m_dq_HWHM)});
	vecVars.push_back(SqwBase::t_var{"S0", "t_real", tl::var_to_str(m_dS0)});

	vecVars.push_back(SqwBase::t_var{"inc_amp", "t_real", tl::var_to_str(m_dIncAmp)});
	vecVars.push_back(SqwBase::t_var{"inc_sig", "t_real", tl::var_to_str(m_dIncSig)});

	vecVars.push_back(SqwBase::t_var{"T", "t_real", tl::var_to_str(m_dT)});

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

		if(strVar == "num_points") m_iNumPoints = tl::str_to_var<decltype(m_iNumPoints)>(strVal);

		else if(strVar == "G") m_vecBragg = str_to_vec<decltype(m_vecBragg)>(strVal);
		else if(strVar == "disp") m_iWhichDisp = tl::str_to_var<decltype(m_iWhichDisp)>(strVal);

		else if(strVar == "D") m_dD = tl::str_to_var<decltype(m_dD)>(strVal);
		else if(strVar == "offs") m_dOffs = tl::str_to_var<decltype(m_dOffs)>(strVal);
		else if(strVar == "E_HWHM") m_dE_HWHM = tl::str_to_var<decltype(m_dE_HWHM)>(strVal);
		else if(strVar == "q_HWHM") m_dq_HWHM = tl::str_to_var<decltype(m_dq_HWHM)>(strVal);
		else if(strVar == "S0") m_dS0 = tl::str_to_var<decltype(m_dS0)>(strVal);

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

SqwBase* SqwMagnon::shallow_copy() const
{
	SqwMagnon *pCpy = new SqwMagnon();
	*static_cast<SqwBase*>(pCpy) = *static_cast<const SqwBase*>(this);

#ifdef USE_RTREE
	pCpy->m_rt = m_rt;
#else
	pCpy->m_kd = m_kd;
#endif
	pCpy->m_iNumPoints = m_iNumPoints;

	pCpy->m_vecBragg = m_vecBragg;
	pCpy->m_iWhichDisp = m_iWhichDisp;

	pCpy->m_dD = m_dD;
	pCpy->m_dOffs = m_dOffs;
	pCpy->m_dS0 = m_dS0;
	pCpy->m_dE_HWHM = m_dE_HWHM;
	pCpy->m_dq_HWHM = m_dq_HWHM;

	pCpy->m_dIncAmp = m_dIncAmp;
	pCpy->m_dIncSig = m_dIncSig;

	pCpy->m_dT = m_dT;

	return pCpy;
}
