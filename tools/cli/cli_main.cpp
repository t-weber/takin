/**
 * Minimalistic takin command line client
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date apr-2016
 * @license GPLv2
 */

#include "tlibs/log/log.h"
#include "tlibs/string/string.h"
#include "libs/version.h"
#include "tools/monteconvo/TASReso.h"

#include <map>
#include <future>

namespace ublas = boost::numeric::ublas;

std::istream& istr = std::cin;
std::ostream& ostr = std::cout;

using t_real = t_real_reso;
template<class KEY, class VAL> using t_map = std::/*unordered_*/map<KEY, VAL>;
using t_mat = ublas::matrix<t_real>;
using t_vec = ublas::vector<t_real>;


// ----------------------------------------------------------------------------
// client function declarations
void show_help(const std::vector<std::string>& vecArgs);
void load_sample(const std::vector<std::string>& vecArgs);
void load_instr(const std::vector<std::string>& vecArgs);
void fix(const std::vector<std::string>& vecArgs);
void calc(const std::vector<std::string>& vecArgs);
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// globals
TASReso g_tas;

using t_func = void(*)(const std::vector<std::string>&);
using t_funcmap = t_map<std::string, t_func>;

t_funcmap g_funcmap =
{
	{"help", &show_help},
	{"load_sample", &load_sample},
	{"load_instr", &load_instr},
	{"fix", &fix},
	{"calc", &calc},
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// client functions
void show_help(const std::vector<std::string>& vecArgs)
{
	std::string strHelp = "Available client functions: ";

	for(t_funcmap::const_iterator iter=g_funcmap.begin(); iter!=g_funcmap.end(); ++iter)
	{
		const t_funcmap::value_type& pair = *iter;
		strHelp += pair.first;
		if(std::next(iter) != g_funcmap.end())
			strHelp += ", ";
	}

	ostr << strHelp << ".\n";
}

void load_sample(const std::vector<std::string>& vecArgs)
{
	if(vecArgs.size() < 2)
	{
		ostr << "Error: No filename given.\n";
		return;
	}

	if(g_tas.LoadLattice(vecArgs[1].c_str()))
		ostr << "OK.\n";
	else
		ostr << "Error: Unable to load " << vecArgs[1] << ".\n";
}

void load_instr(const std::vector<std::string>& vecArgs)
{
	if(vecArgs.size() < 2)
	{
		ostr << "Error: No filename given.\n";
		return;
	}

	if(g_tas.LoadRes(vecArgs[1].c_str()))
		ostr << "OK.\n";
	else
		ostr << "Error: Unable to load " << vecArgs[1] << ".\n";
}

void fix(const std::vector<std::string>& vecArgs)
{
	if(vecArgs.size() < 3)
	{
		ostr << "Error: No variable or value given.\n";
		return;
	}

	if(vecArgs[1] == "ki")
		g_tas.SetKiFix(1);
	else if(vecArgs[1] == "kf")
		g_tas.SetKiFix(0);
	else
	{
		ostr << "Error: Unknown variable " << vecArgs[1] << ".\n";
		return;
	}

	const t_real dVal = tl::str_to_var<t_real>(vecArgs[2]);
	g_tas.SetKFix(dVal);
	ostr << "OK.\n";
}

std::ostream& operator<<(std::ostream& ostr, const t_mat& m)
{
	for(std::size_t i=0; i<m.size1(); ++i)
	{
		for(std::size_t j=0; j<m.size2(); ++j)
		{
			ostr << m(i,j) << " ";
		}
		ostr << " ";
	}

	return ostr;
}

std::ostream& operator<<(std::ostream& ostr, const t_vec& v)
{
	for(std::size_t i=0; i<v.size(); ++i)
		ostr << v(i) << " ";
	return ostr;
}

void calc(const std::vector<std::string>& vecArgs)
{
	if(vecArgs.size() < 5)
	{
		ostr << "Error: No hkl and E position given.\n";
		return;
	}

	const t_real dH = tl::str_to_var<t_real>(vecArgs[1]);
	const t_real dK = tl::str_to_var<t_real>(vecArgs[2]);
	const t_real dL = tl::str_to_var<t_real>(vecArgs[3]);
	const t_real dE = tl::str_to_var<t_real>(vecArgs[4]);

	const ResoResults& res = g_tas.GetResoResults();

	g_tas.GetResoParams().flags |= CALC_R0;
	//g_tas.GetTofResoParams().bCalcR0 = 1;

	if(!g_tas.SetHKLE(dH, dK, dL, dE))
	{
		ostr << "Error: At postion Q=("
			<< dH << "," << dK << "," << dL
			<< "), E=" << dE << ": " << res.strErr
			<< ".\n";
		return;
	}

	//Ellipsoid4d<t_real> ell4d = calc_res_ellipsoid4d(res.reso, res.Q_avg);

	int iParams[2][4][5] =
	{
		{	// projected
			{0, 3, 1, 2, -1},
			{1, 3, 0, 2, -1},
			{2, 3, 0, 1, -1},
			{0, 1, 3, 2, -1}
		},
		{	// sliced
			{0, 3, -1, 2, 1},
			{1, 3, -1, 2, 0},
			{2, 3, -1, 1, 0},
			{0, 1, -1, 2, 3}
		}
	};


	ostr << "OK.\n";

	ostr << "Reso: " << res.reso << "\n";
	ostr << "R0: " << res.dR0 << "\n";
	ostr << "Vol: " << res.dResVol << "\n";
	ostr << "Q_avg: " << res.Q_avg << "\n";
	ostr << "Bragg_FWHMs: " << res.dBraggFWHMs[0] << " "
		<< res.dBraggFWHMs[1] << " "
		<< res.dBraggFWHMs[2] << " "
		<< res.dBraggFWHMs[3] << "\n";


	std::vector<std::future<Ellipse2d<t_real>>> tasks_ell_proj, tasks_ell_slice;

	for(unsigned int iEll=0; iEll<4; ++iEll)
	{
		const int *iP = iParams[0][iEll];
		const int *iS = iParams[1][iEll];

		const t_vec& Q_avg = res.Q_avg;
		const t_mat& reso = res.reso;
		const t_vec& reso_v = res.reso_v;
		const t_real& reso_s = res.reso_s;

		std::future<Ellipse2d<t_real>> ell_proj =
			std::async(std::launch::deferred|std::launch::async,
			[=, &reso, &Q_avg]()
			{ return ::calc_res_ellipse<t_real>(
				reso, reso_v, reso_s,
				Q_avg, iP[0], iP[1], iP[2], iP[3], iP[4]); });
		std::future<Ellipse2d<t_real>> ell_slice =
			std::async(std::launch::deferred|std::launch::async,
			[=, &reso, &Q_avg]()
			{ return ::calc_res_ellipse<t_real>(
				reso, reso_v, reso_s,
				Q_avg, iS[0], iS[1], iS[2], iS[3], iS[4]); });

		tasks_ell_proj.push_back(std::move(ell_proj));
		tasks_ell_slice.push_back(std::move(ell_slice));
	}
	for(unsigned int iEll=0; iEll<4; ++iEll)
	{
		Ellipse2d<t_real> elliProj = tasks_ell_proj[iEll].get();
		Ellipse2d<t_real> elliSlice = tasks_ell_slice[iEll].get();
		const std::string& strLabX = ::ellipse_labels(iParams[0][iEll][0], EllipseCoordSys::Q_AVG);
		const std::string& strLabY = ::ellipse_labels(iParams[0][iEll][1], EllipseCoordSys::Q_AVG);

		ostr << "Ellipse_" << iEll << "_labels: " << strLabX << ", " << strLabY << "\n";

		ostr << "Ellipse_" << iEll << "_proj_angle: " << elliProj.phi << "\n";
		ostr << "Ellipse_" << iEll << "_proj_HWHMs: " << elliProj.x_hwhm << " " << elliProj.y_hwhm << "\n";
		ostr << "Ellipse_" << iEll << "_proj_offs: " << elliProj.x_offs << " " << elliProj.y_offs << "\n";
		//ostr << "Ellipse_" << iEll << "_proj_area: " << elliProj.area << "\n";

		ostr << "Ellipse_" << iEll << "_slice_angle: " << elliSlice.phi << "\n";
		ostr << "Ellipse_" << iEll << "_slice_HWHMs: " << elliSlice.x_hwhm << " " << elliSlice.y_hwhm << "\n";
		ostr << "Ellipse_" << iEll << "_slice_offs: " << elliSlice.x_offs << " " << elliSlice.y_offs << "\n";
		//ostr << "Ellipse_" << iEll << "_slice_area: " << elliSlice.area << "\n";
	}

	ostr.flush();
}
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------
int main()
{
	tl::log_info("This is Takin-CLI, version " TAKIN_VER
		" (built on " __DATE__ ").");
	tl::log_info("Please report bugs to tobias.weber@tum.de.");
	tl::log_info(TAKIN_LICENSE("Takin-CLI"));

	std::string strLine;
	while(std::getline(istr, strLine))
	{
		std::vector<std::string> vecToks;
		tl::get_tokens<std::string, std::string, decltype(vecToks)>
			(strLine, " \t", vecToks);

		for(std::string& strTok : vecToks)
			tl::trim(strTok);

		if(!vecToks.size()) continue;

		if(vecToks[0] == "exit")
			break;

		t_funcmap::const_iterator iter = g_funcmap.find(vecToks[0]);
		if(iter == g_funcmap.end())
		{
			ostr << "Error: No such function: "
				<< vecToks[0] << ".\n" << std::endl;
			continue;
		}

		(*iter->second)(vecToks);
		ostr << "\n";
	}

	return 0;
}
// ----------------------------------------------------------------------------
