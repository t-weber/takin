/*
 * Form factor and scattering length tables
 * @author Tobias Weber
 * @date nov-2015
 * @license GPLv2
 */

#ifndef __TAKIN_FFACT_H__
#define __TAKIN_FFACT_H__

#include <string>
#include <vector>
#include <complex>
#include <mutex>

#include "tlibs/helper/array.h"
#include "tlibs/math/atoms.h"
#include "tlibs/math/mag.h"
#include "libs/globals.h"


template<typename T=double>
class Formfact
{
	template<typename> friend class FormfactList;

	public:
		typedef T value_type;

	protected:
		std::string strAtom;

		std::vector<T> a;
		std::vector<T> b;
		T c;

	public:
		const std::string& GetAtomIdent() const { return strAtom; }

		T GetFormfact(T G) const
		{
			return tl::formfact<T, std::vector>(G, a, b, c);
		}
};

template<typename T/*=double*/>
class FormfactList
{
	public:
		typedef Formfact<T> elem_type;
		typedef typename elem_type::value_type value_type;

	private:
		static std::shared_ptr<FormfactList> s_inst;
		static std::mutex s_mutex;

		FormfactList();

	protected:
		std::vector<elem_type> s_vecAtoms, s_vecIons;
		std::string s_strSrc, s_strSrcUrl;

	public:
		virtual ~FormfactList();
		static std::shared_ptr<const FormfactList> GetInstance();

		std::size_t GetNumAtoms() const { return s_vecAtoms.size(); }
		const elem_type& GetAtom(std::size_t iFormfact) const
		{ return s_vecAtoms[iFormfact]; }

		std::size_t GetNumIons() const { return s_vecIons.size(); }
		const elem_type& GetIon(std::size_t iFormfact) const
		{ return s_vecIons[iFormfact]; }

		const elem_type* Find(const std::string& strElem) const;

		const std::string& GetSource() const { return s_strSrc; }
		const std::string& GetSourceUrl() const { return s_strSrcUrl; }
};


// ----------------------------------------------------------------------------


template<typename T=double>
class MagFormfact
{
	template<typename> friend class MagFormfactList;

	public:
		typedef T value_type;

	protected:
		std::string strAtom;
		std::vector<T> A0, a0;
		std::vector<T> A2, a2;

	public:
		const std::string& GetAtomIdent() const { return strAtom; }

		T GetFormfact(T Q, T L, T S, T J) const
		{
			return tl::mag_formfact<T, std::vector>
				(Q, L,S,J, A0,a0, A2,a2);
		}
};


template<typename T/*=double*/>
class MagFormfactList
{
	public:
		typedef MagFormfact<T> elem_type;
		typedef typename elem_type::value_type value_type;

	private:
		static std::shared_ptr<MagFormfactList> s_inst;
		static std::mutex s_mutex;

		MagFormfactList();

	protected:
		std::vector<elem_type> s_vecAtoms;
		std::string s_strSrc, s_strSrcUrl;

	public:
		virtual ~MagFormfactList();
		static std::shared_ptr<const MagFormfactList> GetInstance();

		std::size_t GetNumAtoms() const { return s_vecAtoms.size(); }
		const elem_type& GetAtom(std::size_t iFormfact) const
		{ return s_vecAtoms[iFormfact]; }

		const elem_type* Find(const std::string& strElem) const;

		const std::string& GetSource() const { return s_strSrc; }
		const std::string& GetSourceUrl() const { return s_strSrcUrl; }
};


// ----------------------------------------------------------------------------


template<typename T=std::complex<double>>
class Scatlen
{
	template<typename> friend class ScatlenList;

	public:
		typedef T value_type;

	protected:
		std::string strAtom;
		value_type coh;
		value_type incoh;

		value_type xsec_coh;
		value_type xsec_incoh;
		value_type xsec_scat;
		value_type xsec_abs;

	public:
		const std::string& GetAtomIdent() const { return strAtom; }

		const value_type& GetCoherent() const { return coh; }
		const value_type& GetIncoherent() const { return incoh; }

		const value_type& GetXSecCoherent() const { return xsec_coh; }
		const value_type& GetXSecIncoherent() const { return xsec_incoh; }
		const value_type& GetXSecScatter() const { return xsec_scat; }
		const value_type& GetXSecAbsorption() const { return xsec_abs; }
};


template<typename T/*=double*/>
class ScatlenList
{
	public:
		typedef Scatlen<std::complex<T>> elem_type;
		typedef typename elem_type::value_type value_type;

	private:
		static std::shared_ptr<ScatlenList> s_inst;
		static std::mutex s_mutex;

		ScatlenList();

	protected:
		std::vector<elem_type> s_vecElems, s_vecIsotopes;
		std::string s_strSrc, s_strSrcUrl;

	public:
		virtual ~ScatlenList();
		static std::shared_ptr<const ScatlenList> GetInstance();

		std::size_t GetNumElems() const { return s_vecElems.size(); }
		const elem_type& GetElem(std::size_t i) const
		{ return s_vecElems[i]; }

		std::size_t GetNumIsotopes() const { return s_vecIsotopes.size(); }
		const elem_type& GetIsotope(std::size_t i) const
		{ return s_vecIsotopes[i]; }

		const elem_type* Find(const std::string& strElem) const;

		const std::string& GetSource() const { return s_strSrc; }
		const std::string& GetSourceUrl() const { return s_strSrcUrl; }
};


#endif
