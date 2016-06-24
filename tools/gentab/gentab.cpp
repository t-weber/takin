/**
 * Creates needed tables
 * @author tweber
 * @date nov-2015
 * @license GPLv2
 */

#include <iostream>
#include <sstream>
#include <set>
#include <limits>

#include "tlibs/string/string.h"
#include "tlibs/file/prop.h"
#include "tlibs/log/log.h"
#include "tlibs/math/linalg.h"
#include "libs/spacegroups/spacegroup_clp.h"

#ifndef USE_BOOST_REX
	#include <regex>
	namespace rex = ::std;
#else
	#include <boost/tr1/regex.hpp>
	namespace rex = ::boost;
#endif

#include <boost/numeric/ublas/io.hpp>
#include <boost/version.hpp>

namespace prop = boost::property_tree;
namespace algo = boost::algorithm;
namespace ublas = boost::numeric::ublas;

using t_real = double;
using t_cplx = std::complex<t_real>;
using t_mat = ublas::matrix<t_real>;


unsigned g_iPrec = std::numeric_limits<t_real>::max_digits10-1;

// ============================================================================

// ----------------------------------------------------------------------------
// ugly, but can't be helped for the moment:
// directly link to the internal clipper coefficients table
// that lives in clipper/clipper/core/atomsf.cpp
namespace clipper { namespace data
{
	extern const struct SFData
	{
		const char atomname[8];
		const t_real a[5], c, b[5], d;  // d is always 0
	} sfdata[];

	const unsigned int numsfdata = 212;
}}
// ----------------------------------------------------------------------------

namespace dat = clipper::data;


bool gen_formfacts()
{
	tl::Prop<std::string> prop;
	prop.SetSeparator('.');

	prop.Add("ffacts.source", "Form factor coefficients extracted from Clipper");
	prop.Add("ffacts.source_url", "http://www.ysbl.york.ac.uk/~cowtan/clipper/");
	prop.Add("ffacts.num_atoms", tl::var_to_str(dat::numsfdata));

	for(unsigned int iFF=0; iFF<dat::numsfdata; ++iFF)
	{
		std::ostringstream ostr;
		ostr << "ffacts.atom_" << iFF;
		std::string strAtom = ostr.str();

		std::string strA, strB;
		for(int i=0; i<5; ++i)
		{
			strA += tl::var_to_str(dat::sfdata[iFF].a[i], g_iPrec) + " ";
			strB += tl::var_to_str(dat::sfdata[iFF].b[i], g_iPrec) + " ";
		}

		prop.Add(strAtom + ".name", std::string(dat::sfdata[iFF].atomname));
		prop.Add(strAtom + ".a", strA);
		prop.Add(strAtom + ".b", strB);
		prop.Add(strAtom + ".c", tl::var_to_str(dat::sfdata[iFF].c, g_iPrec));
	}


	if(!prop.Save("res/ffacts.xml.gz"))
	{
		tl::log_err("Cannot write \"res/ffacts.xml.gz\".");
		return false;
	}
	return true;
}

// ============================================================================

t_cplx get_number(std::string str)
{
	tl::string_rm<std::string>(str, "(", ")");

	if(tl::string_rm<std::string>(str, "<i>", "</i>"))
	{	// complex number
		std::size_t iSign = str.find_last_of("+-");
		str.insert(iSign, ", ");
		str.insert(0, "(");
		str.push_back(')');
	}

	tl::trim(str);
	if(str == "---") str = "";


	t_cplx c;
	std::istringstream istr(str);
	istr >> c;

	t_real dR = c.real(), dI = c.imag();
	tl::set_eps_0(dR); tl::set_eps_0(dI);
	c.real(dR); c.imag(dI);
	return c;
}

bool gen_scatlens()
{
	std::ifstream ifstr("tmp/scatlens.html");
	if(!ifstr)
	{
		tl::log_err("Cannot open \"tmp/scatlens.html\".");
		return false;
	}

	bool bTableStarted = 0;
	std::string strTable;
	while(!ifstr.eof())
	{
		std::string strLine;
		std::getline(ifstr, strLine);

		if(!bTableStarted)
		{
			std::vector<std::string> vecHdr;
			tl::get_tokens_seq<std::string, std::string>(strLine, "<th>", vecHdr, 0);
			if(vecHdr.size() < 9)
				continue;
			bTableStarted = 1;
		}
		else
		{
			// at end of table?
			if(tl::str_to_lower(strLine).find("/table") != std::string::npos)
				break;

			strTable += strLine;
		}
	}
	ifstr.close();



	std::vector<std::string> vecRows;
	tl::get_tokens_seq<std::string, std::string>(strTable, "<tr>", vecRows, 0);


	tl::Prop<std::string> prop;
	prop.SetSeparator('.');
	prop.Add("scatlens.source", "Scattering lengths and cross-sections extracted from NIST table");
	prop.Add("scatlens.source_url", "https://www.ncnr.nist.gov/resources/n-lengths/list.html");

	unsigned int iAtom = 0;
	for(const std::string& strRow : vecRows)
	{
		if(strRow.length() == 0)
			continue;

		std::ostringstream ostr;
		ostr << "scatlens.atom_" << iAtom;
		std::string strAtom = ostr.str();


		std::vector<std::string> vecCol;
		tl::get_tokens_seq<std::string, std::string>(strRow, "<td>", vecCol, 0);
		if(vecCol.size() < 9)
		{
			tl::log_warn("Invalid number of table entries in row \"", strRow, "\".");
			continue;
		}

		std::string strName = vecCol[1];
		tl::trim(strName);
		if(strName == "") continue;

		t_cplx cCoh = get_number(vecCol[3]);
		t_cplx cIncoh = get_number(vecCol[4]);
		t_real dXsecCoh = get_number(vecCol[5]).real();
		t_real dXsecIncoh = get_number(vecCol[6]).real();
		t_real dXsecScat = get_number(vecCol[7]).real();
		t_real dXsecAbsTherm = get_number(vecCol[8]).real();

		prop.Add(strAtom + ".name", strName);
		prop.Add(strAtom + ".coh", tl::var_to_str(cCoh, g_iPrec));
		prop.Add(strAtom + ".incoh", tl::var_to_str(cIncoh, g_iPrec));

		prop.Add(strAtom + ".xsec_coh", tl::var_to_str(dXsecCoh, g_iPrec));
		prop.Add(strAtom + ".xsec_incoh", tl::var_to_str(dXsecIncoh, g_iPrec));
		prop.Add(strAtom + ".xsec_scat", tl::var_to_str(dXsecScat, g_iPrec));
		prop.Add(strAtom + ".xsec_abs", tl::var_to_str(dXsecAbsTherm, g_iPrec));

		++iAtom;
	}

	prop.Add("scatlens.num_atoms", tl::var_to_str(iAtom));


	if(!prop.Save("res/scatlens.xml.gz"))
	{
		tl::log_err("Cannot write \"res/scatlens.xml.gz\".");
		return false;
	}
	return true;
}


// ============================================================================


bool gen_spacegroups()
{
	tl::Prop<std::string> prop;
	prop.SetSeparator('.');

	const unsigned int iNumSGs = 230;
	prop.Add("sgroups.source", "Space group data extracted from Clipper");
	prop.Add("sgroups.source_url", "http://www.ysbl.york.ac.uk/~cowtan/clipper/");
	prop.Add("sgroups.num_groups", tl::var_to_str(iNumSGs));

	for(unsigned int iSG=1; iSG<=iNumSGs; ++iSG)
	{
		SpaceGroupClp sg(iSG);

		std::ostringstream ostr;
		ostr << "sgroups.group_" << (iSG-1);
		std::string strGroup = ostr.str();

		prop.Add(strGroup + ".number", tl::var_to_str(iSG));
		prop.Add(strGroup + ".name", sg.GetName());
		//prop.Add(strGroup + ".pointgroup", get_pointgroup(sg.GetName()));
		prop.Add(strGroup + ".lauegroup", sg.GetLaueGroup());
		//prop.Add(strGroup + ".crystalsys", sg.GetCrystalSystem());
		//prop.Add(strGroup + ".crystalsysname", sg.GetCrystalSystemName());


		std::vector<t_mat> vecTrafos, vecInv, vecPrim, vecCenter;
		sg.GetSymTrafos(vecTrafos);
		sg.GetInvertingSymTrafos(vecInv);
		sg.GetPrimitiveSymTrafos(vecPrim);
		sg.GetCenteringSymTrafos(vecCenter);


		prop.Add(strGroup + ".num_trafos", tl::var_to_str(vecTrafos.size()));
		unsigned int iTrafo = 0;
		for(const t_mat& matTrafo : vecTrafos)
		{
			bool bIsInv = is_mat_in_container(vecInv, matTrafo);
			bool bIsPrim = is_mat_in_container(vecPrim, matTrafo);
			bool bIsCenter = is_mat_in_container(vecCenter, matTrafo);

			std::string strOpts = "; ";
			if(bIsPrim) strOpts += "p";
			if(bIsInv) strOpts += "i";
			if(bIsCenter) strOpts += "c";

			std::ostringstream ostrTrafo;
			ostrTrafo << strGroup << ".trafo_" << iTrafo;
			std::string strTrafo = ostrTrafo.str();

			prop.Add(strTrafo, tl::var_to_str(matTrafo, g_iPrec) + strOpts);

			++iTrafo;
		}
	}


	if(!prop.Save("res/sgroups.xml.gz"))
	{
		tl::log_err("Cannot write \"res/sgroups.xml.gz\".");
		return false;
	}

	return true;
}


// ============================================================================


bool gen_magformfacts()
{
	tl::Prop<std::string> propOut;
	propOut.SetSeparator('.');
	propOut.Add("magffacts.source", "Magnetic form factor coefficients extracted from ILL table");
	propOut.Add("magffacts.source_url", "https://www.ill.eu/sites/ccsl/ffacts/");

	std::size_t iAtom=0;
	std::set<std::string> setAtoms;

	std::vector<std::string> vecFiles =
		{"tmp/j0_1.html", "tmp/j0_2.html",
		"tmp/j0_3.html" , "tmp/j0_4.html",

		"tmp/j2_1.html", "tmp/j2_2.html",
		"tmp/j2_3.html", "tmp/j2_4.html",};

	for(std::size_t iFile=0; iFile<vecFiles.size(); ++iFile)
	{
		const std::string& strFile = vecFiles[iFile];
		std::string strJ = iFile < 4 ? "j0" : "j2";

		// switching to j2 files
		if(iFile==4)
		{
			iAtom = 0;
			setAtoms.clear();
		}

		std::ifstream ifstr(strFile);
		if(!ifstr)
		{
			tl::log_err("Cannot open \"", strFile, "\".");
			return false;
		}

		std::string strTable;
		bool bTableStarted=0;
		while(!ifstr.eof())
		{
			std::string strLine;
			std::getline(ifstr, strLine);
			std::string strLineLower = tl::str_to_lower(strLine);

			if(bTableStarted)
			{
				strTable += strLine + "\n";
				if(strLineLower.find("</table") != std::string::npos)
				break;
			}
			else
			{
				std::size_t iPos = strLineLower.find("<table");
				if(iPos != std::string::npos)
				{
					std::string strSub = strLine.substr(iPos);
					strTable += strSub + "\n";

					bTableStarted = 1;
				}
			}
		}
		if(strTable.length() == 0)
		{
			tl::log_err("Invalid table: \"", strFile, "\".");
			return 0;
		}

		// removing attributes
		rex::basic_regex<char> rex("<([A-Za-z]*)[A-Za-z0-9\\=\\\"\\ ]*>", rex::regex::ECMAScript);
		strTable = rex::regex_replace(strTable, rex, "<$1>");

		tl::find_all_and_replace<std::string>(strTable, "<P>", "");
		tl::find_all_and_replace<std::string>(strTable, "<p>", "");
		//std::cout << strTable << std::endl;


		std::istringstream istrTab(strTable);
		tl::Prop<std::string> prop;
		prop.Load(istrTab, tl::PropType::XML);
		const auto& tab = prop.GetProp().begin()->second;

		auto iter = tab.begin(); ++iter;
		for(; iter!=tab.end(); ++iter)
		{
			auto iterElem = iter->second.begin();
			std::string strElem = iterElem++->second.data();
			tl::trim(strElem);
			if(*strElem.rbegin() == '0')
				strElem.resize(strElem.length()-1);
			else
				strElem += "+";

			if(setAtoms.find(strElem) != setAtoms.end())
			{
				tl::log_warn("Atom ", strElem, " already in set. Ignoring.");
				continue;
			}
			setAtoms.insert(strElem);

			t_real dA = tl::str_to_var<t_real>(iterElem++->second.data());
			t_real da = tl::str_to_var<t_real>(iterElem++->second.data());
			t_real dB = tl::str_to_var<t_real>(iterElem++->second.data());
			t_real db = tl::str_to_var<t_real>(iterElem++->second.data());
			t_real dC = tl::str_to_var<t_real>(iterElem++->second.data());
			t_real dc = tl::str_to_var<t_real>(iterElem++->second.data());
			t_real dD = tl::str_to_var<t_real>(iterElem->second.data());

			std::ostringstream ostrAtom;
			ostrAtom << "magffacts." + strJ + ".atom_" << iAtom;
			propOut.Add(ostrAtom.str() + ".name", strElem);
			propOut.Add(ostrAtom.str() + ".A", tl::var_to_str(dA, g_iPrec));
			propOut.Add(ostrAtom.str() + ".a", tl::var_to_str(da, g_iPrec));
			propOut.Add(ostrAtom.str() + ".B", tl::var_to_str(dB, g_iPrec));
			propOut.Add(ostrAtom.str() + ".b", tl::var_to_str(db, g_iPrec));
			propOut.Add(ostrAtom.str() + ".C", tl::var_to_str(dC, g_iPrec));
			propOut.Add(ostrAtom.str() + ".c", tl::var_to_str(dc, g_iPrec));
			propOut.Add(ostrAtom.str() + ".D", tl::var_to_str(dD, g_iPrec));
			++iAtom;
		}
	}

	propOut.Add("magffacts.num_atoms", tl::var_to_str(iAtom));

	if(!propOut.Save("res/magffacts.xml.gz"))
	{
		tl::log_err("Cannot write \"res/magffacts.xml.gz\".");
		return false;
	}

	return true;
}


// ============================================================================


int main()
{
	std::cout << "Generating atomic form factor coefficient table ... ";
	if(gen_formfacts())
		std::cout << "OK" << std::endl;

	std::cout << "Generating scattering length table ... ";
	if(gen_scatlens())
		std::cout << "OK" << std::endl;

	std::cout << "Generating space group table ... ";
	if(gen_spacegroups())
		std::cout << "OK" << std::endl;

	std::cout << "Generating magnetic form factor coefficient table ... ";
	if(gen_magformfacts())
		std::cout << "OK" << std::endl;

	return 0;
}
