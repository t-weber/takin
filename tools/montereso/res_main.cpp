/**
 * Montereso
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2012, 22-sep-2014
 * @license GPLv2
 */

#include "res.h"
#include "tlibs/log/log.h"
#include "tlibs/string/string.h"
#include "dialogs/EllipseDlg.h"

#include <clocale>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>

using namespace ublas;
using t_real = t_real_reso;

static void add_param(std::unordered_map<std::string, std::string>& map, const std::string& strLine)
{
	std::string str = strLine.substr(1);

	std::pair<std::string, std::string> pair = tl::split_first<std::string>(str, ":", true);
	if(pair.first == "Param")
	{
		std::pair<std::string, std::string> pairParam = tl::split_first<std::string>(pair.second, "=", true);
		pair.first = pair.first + "_" + pairParam.first;
		pair.second = pairParam.second;
	}

	map.insert(pair);
}

template<class t_map>
static void print_map(std::ostream& ostr, const t_map& map)
{
	for(typename t_map::const_iterator iter=map.begin(); iter!=map.end(); ++iter)
		ostr << (*iter).first << ": " << (*iter).second << "\n";
}

enum class FileType
{
	NEUTRON_Q_LIST,
	NEUTRON_KIKF_LIST,

	RESOLUTION_MATRIX,
	COVARIANCE_MATRIX,

	UNKNOWN
};

static bool load_mat(const char* pcFile, Resolution& reso, FileType ft)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr.is_open())
	{
		tl::log_err("Cannot open \"", pcFile, "\".");
		return 0;
	}

	matrix<t_real>& res = reso.res;
	matrix<t_real>& cov = reso.cov;
	res.resize(4,4,0);
	cov.resize(4,4,0);

	if(ft == FileType::COVARIANCE_MATRIX)
	{
		for(unsigned int i=0; i<4; ++i)
			for(unsigned int j=0; j<4; ++j)
				ifstr >> cov(i,j);

		reso.bHasRes = tl::inverse(cov, res);
	}
	else if(ft == FileType::RESOLUTION_MATRIX)
	{
		for(unsigned int i=0; i<4; ++i)
			for(unsigned int j=0; j<4; ++j)
				ifstr >> res(i,j);

		reso.bHasRes = tl::inverse(res, cov);
	}

	tl::log_info("Covariance matrix: ", cov);
	tl::log_info("Resolution matrix: ", res);
	tl::log_info("Matrix valid: ", reso.bHasRes);

	if(reso.bHasRes)
	{
		vector<t_real>& dQ = reso.dQ;

		dQ.resize(4, 0);
		reso.Q_avg.resize(4, 0);
		for(int iQ=0; iQ<4; ++iQ)
			dQ[iQ] = tl::get_SIGMA2HWHM<t_real>()/sqrt(res(iQ,iQ));

		std::ostringstream ostrVals;
		ostrVals << "Gaussian HWHM values: ";
		std::copy(dQ.begin(), dQ.end(), std::ostream_iterator<t_real>(ostrVals, ", "));

		tl::log_info(ostrVals.str());
	}

	return reso.bHasRes;
}

static bool load_mc_list(const char* pcFile, Resolution& res)
{
	FileType ft = FileType::NEUTRON_Q_LIST;

	std::ifstream ifstr(pcFile);
	if(!ifstr.is_open())
	{
		tl::log_err("Cannot open \"", pcFile, "\".");
		return 0;
	}

	// neutron Q,E list
	std::vector<vector<t_real>> vecQ;

	// neutron ki, kf list
	std::vector<vector<t_real>> vecKi, vecKf, vecPos;
	std::vector<t_real> vecPi, vecPf;
	std::string strLine;

	std::unordered_map<std::string, std::string> mapParams;
	bool bEndOfHeader = 0;

	std::size_t uiNumNeutr = 0;
	while(std::getline(ifstr, strLine))
	{
		tl::trim(strLine);
		if(strLine.length()==0)
			continue;
		else if(strLine[0]=='#')
		{
			add_param(mapParams, strLine);
			continue;
		}

		if(!bEndOfHeader)
		{
			bEndOfHeader = 1;

			try
			{
				if(mapParams.at("variables") == "ki_x ki_y ki_z kf_x kf_y kf_z x y z p_i p_f")
				{
					tl::log_info("File is a ki, kf list.");
					ft = FileType::NEUTRON_KIKF_LIST;
				}
			}
			catch(const std::out_of_range& ex)
			{
				tl::log_info("File is a Q list.");
				ft = FileType::NEUTRON_Q_LIST;
			}
		}

		std::istringstream istr(strLine);

		if(ft == FileType::NEUTRON_Q_LIST)
		{
			t_real dQx=0., dQy=0., dQz=0., dE=0.;
			istr >> dQx >> dQy >> dQz >> dE;
			vector<t_real> _vec = tl::make_vec<vector<t_real>>({dQx, dQy, dQz, dE});

			vecQ.push_back(std::move(_vec));
		}
		else if(ft == FileType::NEUTRON_KIKF_LIST)
		{
			t_real dKi[3], dKf[3], dPos[3], dPi=0., dPf=0.;

			istr >> dKi[0] >> dKi[2] >> dKi[1];
			istr >> dKf[0] >> dKf[2] >> dKf[1];
			istr >> dPos[0] >> dPos[2] >> dPos[1];
			istr >> dPi >> dPf;

			dKi[1] = -dKi[1];
			dKf[1] = -dKf[1];
			dPos[1] = -dPos[1];

			vecKi.push_back(tl::make_vec<vector<t_real>>({dKi[0], dKi[1], dKi[2]}));
			vecKf.push_back(tl::make_vec<vector<t_real>>({dKf[0], dKf[1], dKf[2]}));
			vecPos.push_back(tl::make_vec<vector<t_real>>({dPos[0], dPos[1], dPos[2]}));

			vecPi.push_back(dPi);
			vecPf.push_back(dPf);
		}

		++uiNumNeutr;
	}

	tl::log_info("Number of neutrons in file: ", uiNumNeutr);
	//print_map(std::cout, mapParams);

	if(ft == FileType::NEUTRON_Q_LIST)
		res = calc_res(std::forward<decltype(vecQ)&&>(vecQ));
	else if(ft == FileType::NEUTRON_KIKF_LIST)
		res = calc_res(vecKi, vecKf, &vecPi, &vecPf);

	for(vector<t_real>& vecCurQ : res.vecQ)
		vecCurQ -= res.Q_avg_notrafo;

	if(!res.bHasRes)
	{
		tl::log_err("Cannot calculate resolution matrix.");
		return 0;
	}

	return 1;
}

static EllipseDlg* show_ellipses(const Resolution& res)
{
	EllipseDlg* pdlg = new EllipseDlg(0);
	pdlg->show();

	matrix<t_real> matDummy;
	vector<t_real> vecDummy;
	vector<t_real> vecZero = zero_vector<t_real>(4);

	EllipseDlgParams params;
	params.reso = &res.res;
	params.vecMC_direct = &res.vecQ;
	pdlg->SetParams(params);

	return pdlg;
}


int main(int argc, char **argv)
{
	std::setlocale(LC_ALL, "C");
	if(argc <= 1)
	{
		std::ostringstream ostr;
		ostr << "Usage: " << argv[0] << " [-r,-c] <file>\n" 
			<< "\t-r\t<file> contains resolution matrix\n"
			<< "\t-c\t<file> contains covariance matrix\n"
			<< "\t<n/a>\t<file> contains Q or ki,kf list";

		tl::log_err("No input file given.\n", ostr.str());
		return -1;
	}

	const char* pcFile = argv[argc-1];

	FileType ft = FileType::UNKNOWN;
	for(int iArg=1; iArg<argc; ++iArg)
	{
		if(strcmp(argv[iArg], "-r") == 0)
			ft = FileType::RESOLUTION_MATRIX;
		else if(strcmp(argv[iArg], "-c") == 0)
			ft = FileType::COVARIANCE_MATRIX;
	}


	Resolution res;

	if(ft==FileType::RESOLUTION_MATRIX || ft==FileType::COVARIANCE_MATRIX)
	{
		tl::log_info("Loading covariance/resolution matrix from \"", pcFile, "\".");
		if(!load_mat(pcFile, res, ft))
			return -1;
	}
	else
	{
		tl::log_info("Loading neutron list from \"", pcFile, "\".");
		if(!load_mc_list(pcFile, res))
			return -1;
	}


	QLocale::setDefault(QLocale::English);
	QApplication app(argc, argv);

	EllipseDlg* pElliDlg = show_ellipses(res);
	int iRet = app.exec();

	if(pElliDlg) delete pElliDlg;
	return iRet;
}
