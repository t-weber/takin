/*
 * spacegroup helpers
 * @author Tobias Weber
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

/*template<class t_mat = ublas::matrix<double>>
bool is_inverting(const t_mat& mat)
{
	for(typename t_mat::size_type i=0; i<3; ++i)
		for(typename t_mat::size_type j=0; j<3; ++j)
			if(mat(i,j) > 0.)
				return false;

	return true;
}*/

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

// convert e.g.: "P 21 3"  ->  "P2_13"
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


// get PG from SG, eg.: P2_13 -> 23
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
 * check allowed Bragg reflections
 * algorithm based on Clipper's HKL_class
 *    constructor in clipper/core/coords.cpp by K. Cowtan
 */
template<template<class...> class t_cont = std::vector,
	class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>>
bool is_reflection_allowed(int h, int k, int l, const t_cont<t_mat>& vecTrafos)
{
	using t_real = typename t_mat::value_type;
	const t_real dEps = t_real(1e-6);
	t_vec vecHKL = tl::make_vec({t_real(h), t_real(k), t_real(l)});

	for(const t_mat& mat : vecTrafos)
	{
		t_mat matRot = ublas::trans(mat);	// recip -> transpose
		matRot.resize(3,3,1);
		t_vec vecHKL2 = ublas::prod(matRot, vecHKL);

		if(tl::vec_equal(vecHKL, vecHKL2, dEps))
		{
			t_vec vecTrans = tl::make_vec({mat(0,3), mat(1,3), mat(2,3)});
			t_real dInner = ublas::inner_prod(vecTrans, vecHKL2);
			t_real dMod = std::abs(std::fmod(dInner, t_real(1)));	// map into [0,1]

			// not allowed if vecTrans and vecHKL2 not perpendicular
			if(!tl::float_equal<t_real>(dMod, t_real(0), dEps) &&
				!tl::float_equal<t_real>(dMod, t_real(1), dEps))
				return false;
		}
	}
	return true;
}


#endif
