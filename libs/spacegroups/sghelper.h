/**
 * spacegroup helpers
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date oct-2015
 * @license GPLv2
 */

#ifndef __SG_HELPERS_H__
#define __SG_HELPERS_H__

#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include "tlibs/math/linalg.h"
#include "tlibs/phys/atoms.h"
#include "tlibs/string/string.h"

namespace ublas = tl::ublas;


template<class T=double>
std::string print_matrix(const ublas::matrix<T>& mat)
{
	std::ostringstream ostr;
	ostr.precision(4);

	for(int i=0; i<int(mat.size1()); ++i)
	{
		ostr << "(";

		for(int j=0; j<int(mat.size2()); ++j)
			ostr << std::setw(8) << mat(i,j);

		ostr << ")";
		if(i!=int(mat.size1())-1) ostr << "\n";
	}

	return ostr.str();
}


template<class T=double>
std::string print_vector(const ublas::vector<T>& vec)
{
	std::ostringstream ostr;
	ostr.precision(4);

	ostr << "(";
	for(int i=0; i<int(vec.size()); ++i)
	{
		ostr << std::setw(8) << vec[i];
		//if(i!=int(vec.size())-1) ostr << ", ";
	}
	ostr << ")";

	return ostr.str();
}


template<template<class...> class t_cont = std::vector,
	class t_mat = ublas::matrix<double>>
bool is_mat_in_container(const t_cont<t_mat>& cont, const t_mat& mat)
{
	using t_real = typename t_mat::value_type;
	t_real dEps = 1e-4;

	for(const t_mat& matCur : cont)
	{
		if(tl::mat_equal(matCur, mat, dEps))
			return true;
	}
	return false;
}


template<template<class...> class t_cont = std::vector,
	class t_vec = ublas::vector<double>>
bool is_vec_in_container(const t_cont<t_vec>& cont, const t_vec& vec)
{
	using t_real = typename t_vec::value_type;
	t_real dEps = 1e-4;

	for(const t_vec& vecCur : cont)
	{
		if(tl::vec_equal(vecCur, vec, dEps))
			return true;
	}
	return false;
}



template<class T=double>
ublas::vector<T> _get_desc_vec(const std::string& strTok)
{
	if(strTok == "")
		return tl::make_vec<ublas::vector<T>>({0,0,0,0});
	else if(strTok == "x" || strTok == "X")
		return tl::make_vec<ublas::vector<T>>({1,0,0,0});
	else if(strTok == "y" || strTok == "Y")
		return tl::make_vec<ublas::vector<T>>({0,1,0,0});
	else if(strTok == "z" || strTok == "Z")
		return tl::make_vec<ublas::vector<T>>({0,0,1,0});
	else
	{
		bool bOk;
		T dTrans;
		std::tie(bOk, dTrans) = tl::eval_expr<std::string, T>(strTok);
		if(!bOk)
			tl::log_err("Unknown expression: \"", strTok, "\".");
		return tl::make_vec<ublas::vector<T>>({0,0,0,dTrans});
	}

	std::cerr << "Unknown token: \"" << strTok << "\"." << std::endl;
	return tl::make_vec<ublas::vector<T>>({0,0,0,0});
}


/**
 * converts trafo from "xyz" form to matrix form
 */
template<class T=double>
ublas::matrix<T> get_desc_trafo(const std::string& strTrafo)
{
	std::vector<std::string> vecComps;
	tl::get_tokens<std::string, std::string, std::vector<std::string>>
		(strTrafo, std::string(",;"), vecComps);

	ublas::matrix<T> mat = ublas::zero_matrix<T>(4,4);
	mat(3,3) = T(1);

	for(std::size_t iComp=0; iComp<std::min<std::size_t>(vecComps.size(), 3); ++iComp)
	{
		const std::string& strDesc = vecComps[iComp];

		std::vector<std::string> vecPlusComps, vecMinusComps;


		// ---------------------------------------------------------------------
		// split at "+"
		std::vector<std::string> _vecPlusComps;
		tl::get_tokens_seq<std::string, std::string, std::vector>
			(strDesc, std::string("+"), _vecPlusComps);

		for(const std::string& strPlus : _vecPlusComps)
		{
			// split at "-"
			std::vector<std::string> _vecMinusComps;
			tl::get_tokens_seq<std::string, std::string, std::vector>
				(strPlus, std::string("-"), _vecMinusComps);

			// first element in vector is "+", others are "-"
			for(std::size_t iElem=0; iElem<_vecMinusComps.size(); ++iElem)
			{
				std::string strElem = _vecMinusComps[iElem];
				strElem = tl::remove_char(strElem, '#');
				tl::trim(strElem);

				if(iElem == 0)
					vecPlusComps.push_back(strElem);
				else
					vecMinusComps.push_back(strElem);
			}
		}
		// ---------------------------------------------------------------------

		ublas::vector<T> vecComp = ublas::zero_vector<T>(4);
		for(const std::string& str : vecPlusComps)
			vecComp += _get_desc_vec(str);
		for(const std::string& str : vecMinusComps)
			vecComp -= _get_desc_vec(str);

		tl::set_row<decltype(vecComp), decltype(mat)>(mat, iComp, vecComp);
		//std::cout << strDesc << " ->  " << vecComp << std::endl;
	}

	return mat;
}


/**
 * converts matrices into "xyz" form
 */
template<class T=double>
std::string get_trafo_desc(const ublas::matrix<T>& mat)
{
	ublas::vector<T> vecTrans = tl::make_vec({mat(0,3), mat(1,3), mat(2,3)});
	ublas::matrix<T> matRot = mat;
	matRot.resize(3,3, 1);

	ublas::vector<T> vecX = tl::make_vec({1.,0.,0.});
	ublas::vector<T> vecY = tl::make_vec({0.,1.,0.});
	ublas::vector<T> vecZ = tl::make_vec({0.,0.,1.});

	ublas::vector<T> vecXR = ublas::prod(matRot, vecX);
	ublas::vector<T> vecYR = ublas::prod(matRot, vecY);
	ublas::vector<T> vecZR = ublas::prod(matRot, vecZ);

	std::ostringstream ostr;
	ostr << "(";
	for(int i=0; i<3; ++i)
	{
		bool bHasElem = 0;
		if(!tl::float_equal<T>(vecXR[i], 0.))
		{
			double dVal = vecXR[i];

			if(tl::float_equal<T>(dVal, 1.))
				ostr << "x";
			else if(tl::float_equal<T>(dVal, -1.))
				ostr << "-x";
			else
				ostr << dVal << "x";
			bHasElem = 1;
		}
		if(!tl::float_equal<T>(vecYR[i], 0.))
		{
			double dVal = vecYR[i];

			if(bHasElem)
			{
				if(dVal >= 0.) ostr << " + "; else ostr << " - ";

				if(tl::float_equal<T>(std::fabs(dVal), 1.))
					ostr << "y";
				else
					ostr << std::fabs(dVal) << "y";
			}
			else
			{
				if(tl::float_equal<T>(dVal, 1.))
					ostr << "y";
				else if(tl::float_equal<T>(dVal, -1.))
					ostr << "-y";
				else
					ostr << dVal << "y";
			}
			bHasElem = 1;
		}
		if(!tl::float_equal<T>(vecZR[i], 0.))
		{
			double dVal = vecZR[i];

			if(bHasElem)
			{
				if(dVal >= 0.) ostr << " + "; else ostr << " - ";

				if(tl::float_equal<T>(std::fabs(dVal), 1.))
					ostr << "z";
				else
					ostr << std::fabs(dVal) << "z";
			}
			else
			{
				if(tl::float_equal<T>(dVal, 1.))
					ostr << "z";
				else if(tl::float_equal<T>(dVal, -1.))
					ostr << "-z";
				else
					ostr << dVal << "z";
			}
			bHasElem = 1;
		}

		if(!tl::float_equal<T>(vecTrans[i], 0.))
		{
			double dVal = vecTrans[i];

			if(bHasElem)
			{
				if(dVal >= 0.) ostr << " + "; else ostr << " - ";
				ostr << std::fabs(dVal);
			}
			else
				ostr << dVal;
		}

		if(i<2)
			ostr << ", ";
	}
	ostr << ")";
	return ostr.str();
}


/**
 * convert e.g.: "P 21 3"  ->  "P2_13"
 */
template<class t_str=std::string>
void convert_hm_symbol(t_str& strHM)
{
	std::vector<t_str> vecSyms;
	tl::get_tokens<t_str, t_str, decltype(vecSyms)>(strHM, " ", vecSyms);

	for(t_str& str : vecSyms)
	{
		bool bLastWasDigit = 0;
		for(std::size_t iC = 0; iC<str.length(); ++iC)
		{
			typename t_str::value_type c = str[iC];
			bool bCurIsDigit = std::isdigit(c);

			if(bCurIsDigit && bLastWasDigit)
			{
				str.insert(iC, "_");
				bLastWasDigit = 0;
			}
			else
			{
				bLastWasDigit = bCurIsDigit;
			}
		}
	}

	strHM = "";
	for(const t_str& str : vecSyms)
		strHM += str /*+ " "*/;
}


/**
 * get PG from SG, eg.: P2_13 -> 23
 */
template<class t_str=std::string>
t_str get_pointgroup(const t_str& str)
{
	t_str strRet;

	// remove cell centering symbol
	std::remove_copy_if(str.begin(), str.end(),
		std::back_insert_iterator<t_str>(strRet),
		[](typename t_str::value_type c)->bool { return std::isupper(c); });

	// remove screw axes
	while(1)
	{
		std::size_t iPosScrew = strRet.find('_');
		if(iPosScrew == t_str::npos)
			break;
		strRet.erase(iPosScrew, 2);
	}

	// mirror plane
	std::replace_if(strRet.begin(), strRet.end(),
		[](typename t_str::value_type c)->bool
		{ return c=='a'||c=='b'||c=='c'||c=='d'||c=='e'||c=='f'||c=='n'; },
		'm' );
	return strRet;
}


/**
 * check allowed Bragg reflections based on centering
 * see e.g.: http://pd.chem.ucl.ac.uk/pdnn/symm4/centred.htm
 */
template<class t_int=int>
bool is_centering_reflection_allowed(const std::string& strSG, t_int h, t_int k, t_int l)
{
	if(strSG.length() == 0) return true;

	const char cCentr = strSG[0];
	switch(cCentr)
	{
		case 'P': return true;
		case 'I': return tl::is_even(h+k+l);
		case 'F':
		{
			return (tl::is_even(h) && tl::is_even(k) && tl::is_even(l))
				|| (tl::is_odd(h) && tl::is_odd(k) && tl::is_odd(l));
		}
		case 'A': return tl::is_even(k+l);
		case 'B': return tl::is_even(h+l);
		case 'C': return tl::is_even(h+k);
		case 'R': return ((-h+k+l)%3 == 0);
	}

	return true;
}


/**
 * checks for allowed Bragg reflections
 * algorithm based on Clipper's HKL_class
 * constructor in clipper/core/coords.cpp by K. Cowtan
 */
template<template<class...> class t_cont = std::vector,
	class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>>
bool is_reflection_allowed(int h, int k, int l, const t_cont<t_mat>& vecTrafos)
{
	using t_real = typename t_mat::value_type;
	const constexpr t_real dEps = t_real(1e-6);
	t_vec vecHKL = tl::make_vec({t_real(h), t_real(k), t_real(l)});

	for(const t_mat& mat : vecTrafos)
	{
		// rotation part of the symmetry trafo
		t_mat matRot = ublas::trans(mat);	// recip -> transpose
		matRot.resize(3,3,1);

		// rotate hkl coordinates
		t_vec vecHKLrot = ublas::prod(matRot, vecHKL);

		if(tl::vec_equal(vecHKL, vecHKLrot, dEps))
		{
			// translation part of the symmetry trafo
			t_vec vecTrans = tl::make_vec({mat(0,3), mat(1,3), mat(2,3)});

			// inner product of rotated hkl coordiantes and translation vector
			t_real dInner = ublas::inner_prod(vecTrans, vecHKLrot);
			dInner = std::abs(std::fmod(dInner, t_real(1)));	// map into [0,1]

			// not allowed if vecTrans and vecHKLrot not perpendicular or parallel
			if(!tl::float_equal<t_real>(dInner, t_real(0), dEps) &&
				!tl::float_equal<t_real>(dInner, t_real(1), dEps))
			{
				//tl::log_debug("No allowed: t*r = ", dInner, ", mat = ", mat);
				return false;
			}
		}
	}
	return true;
}

#endif
