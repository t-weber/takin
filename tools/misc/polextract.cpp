/**
 * Extract polarisation matrices from given data files
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 30-sep-18
 * @license GPLv2
 *
 * g++ -std=c++11 -I../.. -o polextract polextract.cpp ../../tlibs/file/loadinstr.cpp ../../tlibs/string/eval.cpp ../../tlibs/log/log.cpp -lboost_iostreams -lboost_system
 */

#include "tlibs/file/loadinstr.h"
#include "tlibs/phys/neutrons.h"

#include <tuple>
#include <vector>
#include <unordered_set>

using t_real = double;
using t_vec = std::vector<t_real>;

static const t_real g_dEps = 1e-6;
static const int g_iPrec = 6;


void calcpol(const std::string& strFileId, const tl::FileInstrBase<t_real>* pInstr, std::ostream& ostr)
{
	if(!pInstr)
	{
		tl::log_err("Invalid file: ", strFileId, ".");
		return;
	}

	const std::vector<std::array<t_real, 6>>& vecPolStates = pInstr->GetPolStates();
	const std::size_t iNumPolStates = vecPolStates.size();
	if(iNumPolStates == 0)
	{
		tl::log_err("No polarisation data found in ", strFileId, ".");
		return;
	}


	// get the SF state to a given NSF state
	auto find_spinflip_state_idx = [&vecPolStates, iNumPolStates]
	(const std::array<t_real, 6>& state) -> std::size_t
	{
		t_real dPix1 = state[0], dPiy1 = state[1], dPiz1 = state[2];
		t_real dPfx1 = state[3], dPfy1 = state[4], dPfz1 = state[5];

		for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol)
		{
			t_real dPix2 = vecPolStates[iPol][0];
			t_real dPiy2 = vecPolStates[iPol][1];
			t_real dPiz2 = vecPolStates[iPol][2];
			t_real dPfx2 = vecPolStates[iPol][3];
			t_real dPfy2 = vecPolStates[iPol][4];
			t_real dPfz2 = vecPolStates[iPol][5];

			if(tl::float_equal(dPix1, dPix2, g_dEps) &&
				tl::float_equal(dPiy1, dPiy2, g_dEps) &&
				tl::float_equal(dPiz1, dPiz2, g_dEps) &&
				tl::float_equal(dPfx1, -dPfx2, g_dEps) &&
				tl::float_equal(dPfy1, -dPfy2, g_dEps) &&
				tl::float_equal(dPfz1, -dPfz2, g_dEps))
			{
				return iPol;
			}
		}

		// none found
		return iNumPolStates;
	};


	auto propagate_err = [](t_real x, t_real y, t_real dx, t_real dy) -> t_real
	{
		// d((x-y)/(x+y)) = dx * 2*y/(x+y)^2 - dy * 2*x/(x+y)^2
		return std::sqrt(dx*2.*y/((x+y)*(x+y))*dx*2.*y/((x+y)*(x+y))
			+ dy*2.*x/((x+y)*(x+y))*dy*2.*x/((x+y)*(x+y)));
	};


	// convert polarisation vector to string representation
	auto polvec_str = [](t_real x, t_real y, t_real z) -> std::string
	{
		std::ostringstream ostr;
		ostr.precision(g_iPrec);

		if(tl::float_equal<t_real>(x, 1., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "x";
		else if(tl::float_equal<t_real>(x, -1., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "-x";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, 1., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "y";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, -1., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "-y";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, 1., g_dEps))
			ostr << "z";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, -1., g_dEps))
			ostr << "-z";
		else
			ostr << "[" << x << " " << y << " " << z << "]";

		return ostr.str();
	};


	std::vector<bool> vecHasSFPartner;
	// indices to spin-flipped states
	std::vector<std::size_t> vecSFIdx;

	for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol)
	{
		const std::array<t_real, 6>& state = vecPolStates[iPol];
		std::size_t iIdx = find_spinflip_state_idx(state);

		vecHasSFPartner.push_back(iIdx < iNumPolStates);
		vecSFIdx.push_back(iIdx);
	}


	const std::vector<t_real>& vecCnts = pInstr->GetCol(pInstr->GetCountVar().c_str());


	// polarisation matrix elements
	ostr.precision(g_iPrec);
	bool bHasAnyData = false;

	ostr << "# "
		 << std::setw(g_iPrec*2-2) << std::right << "file" << " "	// file name
		 << std::setw(g_iPrec) << std::right << "point" << " "			// scan pos
		 << std::setw(g_iPrec*2) << std::right << "h" << " "		// h
		 << std::setw(g_iPrec*2) << std::right << "k" << " "		// k
		 << std::setw(g_iPrec*2) << std::right << "l" << " "		// l
		 << std::setw(g_iPrec*2) << std::right << "E" << " "		// E
		 << std::setw(g_iPrec) << std::right << "init." << " "
		 << std::setw(g_iPrec) << std::right << "fin." << " "
		 << std::setw(g_iPrec*2) << std::right << "pol" << " "
		 << std::setw(g_iPrec*2) << std::right << "pol_err" << " "
		 << "\n";

	// iterate over scan points
	for(std::size_t iPt=0; iPt<vecCnts.size()/iNumPolStates; ++iPt)
	{
		// iterate over all polarisation states which have a SF partner
		std::unordered_set<std::size_t> setPolAlreadySeen;
		for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol)
		{
			if(!vecHasSFPartner[iPol]) continue;
			if(setPolAlreadySeen.find(iPol) != setPolAlreadySeen.end())
				continue;

			// scan position
			auto hklKiKf = pInstr->GetScanHKLKiKf(iPt*iNumPolStates + iPol);
			t_real dE = t_real(tl::get_energy_transfer(hklKiKf[3]/tl::get_one_angstrom<t_real>(),
				hklKiKf[4]/tl::get_one_angstrom<t_real>()) / tl::get_one_meV<t_real>());
			const std::size_t iSF = vecSFIdx[iPol];
			const std::array<t_real, 6>& state = vecPolStates[iPol];
			//const std::array<t_real, 6>& stateSF = vecPolStates[iSF];

			setPolAlreadySeen.insert(iPol);
			setPolAlreadySeen.insert(iSF);

			const t_real dCntsNSF = vecCnts[iPt*iNumPolStates + iPol];
			const t_real dCntsSF = vecCnts[iPt*iNumPolStates + iSF];
			t_real dNSFErr = std::sqrt(dCntsNSF);
			t_real dSFErr = std::sqrt(dCntsSF);
			if(tl::float_equal(dCntsNSF, t_real(0), g_dEps))
				dNSFErr = 1.;
			if(tl::float_equal(dCntsSF, t_real(0), g_dEps))
				dSFErr = 1.;

			bool bInvalid = tl::float_equal(dCntsNSF+dCntsSF, t_real(0), g_dEps);
			t_real dPolElem = 0., dPolErr = 1.;
			if(!bInvalid)
			{
				dPolElem = /*std::abs*/((dCntsSF-dCntsNSF) / (dCntsSF+dCntsNSF));
				dPolErr = propagate_err(dCntsNSF, dCntsSF, dNSFErr, dSFErr);
			}

			// polarisation matrix elements, e.g. <[100] | P | [010]> = <x|P|y>
			ostr << std::setw(g_iPrec*2) << std::right << strFileId << " "	// file name
				<< std::setw(g_iPrec) << std::right << (iPt+1) << " "		// scan pos
				<< std::setw(g_iPrec*2) << std::right << hklKiKf[0] << " "	// h
				<< std::setw(g_iPrec*2) << std::right << hklKiKf[1] << " "	// k
				<< std::setw(g_iPrec*2) << std::right << hklKiKf[2] << " "	// l
				<< std::setw(g_iPrec*2) << std::right << dE << " "	// E
				<< std::setw(g_iPrec) << std::right << polvec_str(state[0], state[1], state[2]) << " "
				<< std::setw(g_iPrec) << std::right << polvec_str(state[3], state[4], state[5]) << " "
				<< std::setw(g_iPrec*2) << std::right << (bInvalid ? "--- ": tl::var_to_str(dPolElem, g_iPrec)) << " "
				<< std::setw(g_iPrec*2) << std::right << (bInvalid ? "--- ": tl::var_to_str(dPolErr, g_iPrec)) << " "
				<< "\n";

			bHasAnyData = true;
		}
	}
}


int main(int argc, char** argv)
{
	auto pInstr = std::unique_ptr<tl::FileInstrBase<t_real>>(tl::FileInstrBase<t_real>::LoadInstr("/home/tw/tmp/poltst/tst.dat"));
	pInstr->SetPolNames("p1", "p2", "i0", "i1");
	pInstr->ParsePolData();
	calcpol("0123", pInstr.get(), std::cout);

	return 0;
}
