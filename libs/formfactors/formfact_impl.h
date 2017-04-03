/**
 * Form factor and scattering length tables
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#ifndef __TAKIN_FFACT_IMPL_H__
#define __TAKIN_FFACT_IMPL_H__

#include "formfact.h"
#include "libs/globals.h"
#include "tlibs/math/math.h"
#include "tlibs/log/log.h"
#include "tlibs/file/prop.h"
#include "tlibs/string/string.h"


template<typename T>
std::shared_ptr<FormfactList<T>> FormfactList<T>::s_inst = nullptr;

template<typename T>
std::mutex FormfactList<T>::s_mutex;


template<typename T>
FormfactList<T>::FormfactList()
{
	std::string strTabFile = find_resource("res/data/ffacts.xml");
	tl::log_debug("Loading atomic form factors from file \"", strTabFile, "\".");

	tl::Prop<std::string> xml;
	if(!xml.Load(strTabFile.c_str(), tl::PropType::XML))
		return;

	unsigned int iNumDat = xml.Query<unsigned int>("ffacts/num_atoms", 0);
	if(!iNumDat)
	{
		tl::log_err("No data in atomic form factor list.");
		return;
	}

	bool bIonStart = 0;
	for(unsigned int iSf=0; iSf<iNumDat; ++iSf)
	{
		elem_type ffact;
		std::string strAtom = "ffacts/atom_" + tl::var_to_str(iSf);

		ffact.strAtom = xml.Query<std::string>((strAtom + "/name").c_str(), "");
		tl::get_tokens<value_type, std::string, std::vector<value_type>>
			(xml.Query<std::string>((strAtom + "/a").c_str(), ""), " \t", ffact.a);
		tl::get_tokens<value_type, std::string, std::vector<value_type>>
			(xml.Query<std::string>((strAtom + "/b").c_str(), ""), " \t", ffact.b);
		ffact.c = xml.Query<value_type>((strAtom + "/c").c_str(), 0.);

		if(!bIonStart && ffact.strAtom.find_first_of("+-") != std::string::npos)
			bIonStart = 1;

		if(!bIonStart)
			s_vecAtoms.push_back(std::move(ffact));
		else
			s_vecIons.push_back(std::move(ffact));
	}

	s_strSrc = xml.Query<std::string>("ffacts/source", "");
	s_strSrcUrl = xml.Query<std::string>("ffacts/source_url", "");
}

template<typename T>
FormfactList<T>::~FormfactList()
{}

template<typename T>
std::shared_ptr<const FormfactList<T>> FormfactList<T>::GetInstance()
{
	std::lock_guard<std::mutex> _guard(s_mutex);

	if(!s_inst)
		s_inst = std::shared_ptr<FormfactList<T>>(new FormfactList<T>());

	return s_inst;
}

template<typename T>
const typename FormfactList<T>::elem_type* FormfactList<T>::Find(const std::string& strElem) const
{
	typedef typename decltype(s_vecAtoms)::const_iterator t_iter;

	// atoms
	t_iter iter = std::find_if(s_vecAtoms.begin(), s_vecAtoms.end(),
		[&strElem](const elem_type& elem)->bool
		{
			//std::cout << elem.GetAtomIdent() << std::endl;
			return elem.GetAtomIdent() == strElem;
		});
	if(iter != s_vecAtoms.end())
		return &*iter;

	// ions
	iter = std::find_if(s_vecIons.begin(), s_vecIons.end(),
		[&strElem](const elem_type& elem)->bool
		{
			//std::cout << elem.GetAtomIdent() << std::endl;
			return elem.GetAtomIdent() == strElem;
		});
	if(iter != s_vecIons.end())
		return &*iter;

	return nullptr;
}




// =============================================================================

template<typename T>
std::shared_ptr<MagFormfactList<T>> MagFormfactList<T>::s_inst = nullptr;

template<typename T>
std::mutex MagFormfactList<T>::s_mutex;


template<typename T>
MagFormfactList<T>::MagFormfactList()
{
	std::string strTabFile = find_resource("res/data/magffacts.xml");
	tl::log_debug("Loading magnetic form factors from file \"", strTabFile, "\".");

	tl::Prop<std::string, true> xml;
	if(!xml.Load(strTabFile.c_str(), tl::PropType::XML))
		return;

	unsigned int iNumDat = xml.Query<unsigned int>("magffacts/num_atoms", 0);
	if(!iNumDat)
	{
		tl::log_err("No data in magnetic form factor list.");
		return;
	}
	for(unsigned int iSf=0; iSf<iNumDat; ++iSf)
	{
		elem_type ffact;
		std::string strAtom = "magffacts/j0/atom_" + tl::var_to_str(iSf);

		ffact.strAtom = xml.Query<std::string>((strAtom + "/name").c_str(), "");
		for(const std::string& strA : {"/A", "/B", "/C", "/D"})
			ffact.A0.push_back(xml.Query<value_type>((strAtom + strA).c_str(), 0.));
		for(const std::string& stra : {"/a", "/b", "/c"})
			ffact.a0.push_back(xml.Query<value_type>((strAtom + stra).c_str(), 0.));

		s_vecAtoms.push_back(std::move(ffact));
	}

	for(unsigned int iSf=0; iSf<iNumDat; ++iSf)
	{
		std::string strAtom = "magffacts/j2/atom_" + tl::var_to_str(iSf);
		std::string strAtomName = xml.Query<std::string>((strAtom + "/name").c_str(), "");

		MagFormfactList<T>::elem_type* pElem =
			const_cast<MagFormfactList<T>::elem_type*>(Find(strAtomName));
		if(!pElem)
		{
			tl::log_err("Mismatch in j0 and j2 form factor tables.");
			continue;
		}

		for(const std::string& strA : {"/A", "/B", "/C", "/D"})
			pElem->A2.push_back(xml.Query<value_type>((strAtom + strA).c_str(), 0.));
		for(const std::string& stra : {"/a", "/b", "/c"})
			pElem->a2.push_back(xml.Query<value_type>((strAtom + stra).c_str(), 0.));
	}

	s_strSrc = xml.Query<std::string>("magffacts/source", "");
	s_strSrcUrl = xml.Query<std::string>("magffacts/source_url", "");
}

template<typename T>
MagFormfactList<T>::~MagFormfactList()
{}

template<typename T>
std::shared_ptr<const MagFormfactList<T>> MagFormfactList<T>::GetInstance()
{
	std::lock_guard<std::mutex> _guard(s_mutex);

	if(!s_inst)
		s_inst = std::shared_ptr<MagFormfactList<T>>(new MagFormfactList<T>());

	return s_inst;
}

template<typename T>
const typename MagFormfactList<T>::elem_type* MagFormfactList<T>::Find(const std::string& strElem) const
{
	typedef typename decltype(s_vecAtoms)::const_iterator t_iter;

	t_iter iter = std::find_if(s_vecAtoms.begin(), s_vecAtoms.end(),
		[&strElem](const elem_type& elem) -> bool
		{
			return elem.GetAtomIdent() == strElem;
		});
	if(iter != s_vecAtoms.end())
		return &*iter;

	return nullptr;
}



// =============================================================================


template<typename T>
std::shared_ptr<ScatlenList<T>> ScatlenList<T>::s_inst = nullptr;

template<typename T>
std::mutex ScatlenList<T>::s_mutex;


template<typename T>
ScatlenList<T>::ScatlenList()
{
	std::string strTabFile = find_resource("res/data/scatlens.xml");
	tl::log_debug("Loading neutron scattering lengths from file \"", strTabFile, "\".");

	tl::Prop<std::string> xml;
	if(!xml.Load(strTabFile.c_str(), tl::PropType::XML))
		return;

	const unsigned int iNumDat = xml.Query<unsigned int>("scatlens/num_atoms", 0);
	if(!iNumDat)
	{
		tl::log_err("No data in scattering length list.");
		return;
	}

	for(unsigned int iSl=0; iSl<iNumDat; ++iSl)
	{
		ScatlenList<T>::elem_type slen;
		std::string strAtom = "scatlens/atom_" + tl::var_to_str(iSl);

		slen.strAtom = xml.Query<std::string>((strAtom + "/name").c_str(), "");
		slen.coh = xml.Query<ScatlenList<T>::value_type>((strAtom + "/coh").c_str(), 0.);
		slen.incoh = xml.Query<ScatlenList<T>::value_type>((strAtom + "/incoh").c_str(), 0.);

		slen.xsec_coh = xml.Query<ScatlenList<T>::value_type>((strAtom + "/xsec_coh").c_str(), 0.);
		slen.xsec_incoh = xml.Query<ScatlenList<T>::value_type>((strAtom + "/xsec_incoh").c_str(), 0.);
		slen.xsec_scat = xml.Query<ScatlenList<T>::value_type>((strAtom + "/xsec_scat").c_str(), 0.);
		slen.xsec_abs = xml.Query<ScatlenList<T>::value_type>((strAtom + "/xsec_abs").c_str(), 0.);
		slen.abund = xml.QueryOpt<ScatlenList<T>::real_type>((strAtom + "/abund").c_str());
		slen.hl = xml.QueryOpt<ScatlenList<T>::real_type>((strAtom + "/hl").c_str());

		if(std::isdigit(slen.strAtom[0]))
			s_vecIsotopes.push_back(std::move(slen));	// pure isotopes
		else
			s_vecElems.push_back(std::move(slen));		// isotope mixtures
	}

	// link pure isotopes to isotope mixtures
	for(const auto& isotope : s_vecIsotopes)
	{
		const std::string& strIso = isotope.GetAtomIdent();
		std::string strElem = tl::remove_chars(strIso, std::string("0123456789"));
		tl::trim(strElem);

		auto iterElem = std::find_if(s_vecElems.begin(), s_vecElems.end(),
			[&strElem](const elem_type& elem) { return (elem.GetAtomIdent() == strElem); });
		if(iterElem == s_vecElems.end())
		{
			tl::log_err("Mixture for isotope \"", strElem, "\" was not found in scattering lengths list.");
			continue;
		}

		iterElem->m_vecIsotopes.push_back(&isotope);
	}

	s_strSrc = xml.Query<std::string>("scatlens/source", "");
	s_strSrcUrl = xml.Query<std::string>("scatlens/source_url", "");

#ifndef NDEBUG
	// testing scattering lengths
	for(const auto& elem : s_vecElems)
	{
		const auto& vecIsotopes = elem.GetIsotopes();
		if(!vecIsotopes.size()) continue;

		std::vector<ScatlenList<T>::value_type> vecbcoh, vecbincoh;
		std::vector<ScatlenList<T>::real_type> vecAbund;

		for(const auto* isotope : vecIsotopes)
		{
			const auto& abund = isotope->GetAbundance();
			if(abund)
			{
				vecAbund.push_back(*abund);
				vecbcoh.push_back(isotope->GetCoherent());
				vecbincoh.push_back(isotope->GetIncoherent());
			}
		}
		
		auto themean = tl::mean_value<decltype(vecAbund), decltype(vecbcoh)>
			(vecAbund, vecbcoh);
		auto thedev = tl::std_dev<decltype(vecAbund), decltype(vecbcoh)>
			(vecAbund, vecbcoh);

		tl::log_debug(elem.GetAtomIdent(), ": mean b: ", themean,
			": stddev b: ", thedev,
			", orig coh: ", elem.GetCoherent(),
			", orig inc: ", elem.GetIncoherent());
	}
#endif
}


template<typename T>
ScatlenList<T>::~ScatlenList()
{}

template<typename T>
std::shared_ptr<const ScatlenList<T>> ScatlenList<T>::GetInstance()
{
	std::lock_guard<std::mutex> _guard(s_mutex);

	if(!s_inst)
		s_inst = std::shared_ptr<ScatlenList<T>>(new ScatlenList<T>());

	return s_inst;
}

template<typename T>
const typename ScatlenList<T>::elem_type* ScatlenList<T>::Find(const std::string& strElem) const
{
	typedef typename decltype(s_vecElems)::const_iterator t_iter;

	// elements
	t_iter iter = std::find_if(s_vecElems.begin(), s_vecElems.end(),
		[&strElem](const elem_type& elem)->bool
		{
			//std::cout << elem.GetAtomIdent() << std::endl;
			return elem.GetAtomIdent() == strElem;
		});
	if(iter != s_vecElems.end())
		return &*iter;

	// isotopes
	iter = std::find_if(s_vecIsotopes.begin(), s_vecIsotopes.end(),
		[&strElem](const elem_type& elem)->bool
		{
			//std::cout << elem.GetAtomIdent() << std::endl;
			return elem.GetAtomIdent() == strElem;
		});
	if(iter != s_vecIsotopes.end())
		return &*iter;

	return nullptr;
}

#endif
