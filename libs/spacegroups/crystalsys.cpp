/**
 * Crystal Systems
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date oct-2015
 * @license GPLv2
 */

#include "crystalsys.h"
#include "tlibs/string/string.h"


const unsigned int* get_crystal_system_start_indices()
{
	static const unsigned int iStartNr[7] = { 1, 3, 16, 75, 143, 168, 195 };
	return iStartNr;
}

const char** get_crystal_system_names(bool bCapital)
{
	static const char* pcNames[] = { "triclinic", "monoclinic", "orthorhombic", 
	"tetragonal", "trigonal", "hexagonal", "cubic" };
	static const char* pcNamesCap[] = { "Triclinic", "Monoclinic", "Orthorhombic", 
	"Tetragonal", "Trigonal", "Hexagonal", "Cubic" };

	return bCapital ? pcNamesCap : pcNames;
}


CrystalSystem get_crystal_system_from_space_group(unsigned int iSGNr)
{
	const unsigned int* pIdx = get_crystal_system_start_indices();

	if(iSGNr>=pIdx[0] && iSGNr<=pIdx[1]-1)
		return CRYS_TRICLINIC;
	else if(iSGNr>=pIdx[1] && iSGNr<=pIdx[2]-1)
		return CRYS_MONOCLINIC;
	else if(iSGNr>=pIdx[2] && iSGNr<=pIdx[3]-1)
		return CRYS_ORTHORHOMBIC;
	else if(iSGNr>=pIdx[3] && iSGNr<=pIdx[4]-1)
		return CRYS_TETRAGONAL;
	else if(iSGNr>=pIdx[4] && iSGNr<=pIdx[5]-1)
		return CRYS_TRIGONAL;
	else if(iSGNr>=pIdx[5] && iSGNr<=pIdx[6]-1)
		return CRYS_HEXAGONAL;
	else if(iSGNr>=pIdx[6] && iSGNr<=230)
		return CRYS_CUBIC;

	return CRYS_NOT_SET;
}


CrystalSystem get_crystal_system_from_laue_group(const char* pcLaue)
{
	std::string strLaue = pcLaue;
	tl::trim(strLaue);

	if(strLaue == "-1")
		return CRYS_TRICLINIC;
	else if(strLaue == "2/m")
		return CRYS_MONOCLINIC;
	else if(strLaue == "mmm")
		return CRYS_ORTHORHOMBIC;
	else if(strLaue == "4/m" || strLaue == "4/mmm")
		return CRYS_TETRAGONAL;
	else if(strLaue == "-3" || strLaue == "-3m")
		return CRYS_TRIGONAL;
	else if(strLaue == "6/m" || strLaue == "6/mmm")
		return CRYS_HEXAGONAL;
	else if(strLaue == "m-3" || strLaue == "m-3m")
		return CRYS_CUBIC;

	return CRYS_NOT_SET;
}


const char* get_crystal_system_name(CrystalSystem ty)
{
	const char **pcNames = get_crystal_system_names();

	switch(ty)
	{
		case CRYS_NOT_SET: return "<not set>";
		case CRYS_TRICLINIC: return pcNames[0];
		case CRYS_MONOCLINIC: return pcNames[1];
		case CRYS_ORTHORHOMBIC: return pcNames[2];
		case CRYS_TETRAGONAL: return pcNames[3];
		case CRYS_TRIGONAL: return pcNames[4];
		case CRYS_HEXAGONAL: return pcNames[5];
		case CRYS_CUBIC: return pcNames[6];
	}

	return "<unknown>";
}


#ifndef NO_QT


/**
 * allowed lattice definitions for crystal systems
 * see e.g.: https://en.wikipedia.org/wiki/Bravais_lattice
 */
void set_crystal_system_edits(CrystalSystem crystalsys,
	QLineEdit* editA, QLineEdit* editB, QLineEdit* editC,
	QLineEdit* editAlpha, QLineEdit* editBeta, QLineEdit* editGamma,
	QLineEdit* editARecip, QLineEdit* editBRecip, QLineEdit* editCRecip,
	QLineEdit* editAlphaRecip, QLineEdit* editBetaRecip, QLineEdit* editGammaRecip)
{
	switch(crystalsys)
	{
		case CRYS_CUBIC:
			if(editA) editA->setEnabled(1);
			if(editB) editB->setEnabled(0);
			if(editC) editC->setEnabled(0);
			if(editAlpha) editAlpha->setEnabled(0);
			if(editBeta) editBeta->setEnabled(0);
			if(editGamma) editGamma->setEnabled(0);

			if(editARecip) editARecip->setEnabled(1);
			if(editBRecip) editBRecip->setEnabled(0);
			if(editCRecip) editCRecip->setEnabled(0);
			if(editAlphaRecip) editAlphaRecip->setEnabled(0);
			if(editBetaRecip) editBetaRecip->setEnabled(0);
			if(editGammaRecip) editGammaRecip->setEnabled(0);

			if(editB) editB->setText(editA->text());
			if(editC) editC->setText(editA->text());
			if(editBRecip) editBRecip->setText(editARecip->text());
			if(editCRecip) editCRecip->setText(editARecip->text());
			if(editAlpha) editAlpha->setText("90");
			if(editBeta) editBeta->setText("90");
			if(editGamma) editGamma->setText("90");
			if(editAlphaRecip) editAlphaRecip->setText("90");
			if(editBetaRecip) editBetaRecip->setText("90");
			if(editGammaRecip) editGammaRecip->setText("90");
			break;

		case CRYS_MONOCLINIC:
			if(editA) editA->setEnabled(1);
			if(editB) editB->setEnabled(1);
			if(editC) editC->setEnabled(1);
			if(editAlpha) editAlpha->setEnabled(0);
			if(editBeta) editBeta->setEnabled(1);
			if(editGamma) editGamma->setEnabled(0);

			if(editARecip) editARecip->setEnabled(1);
			if(editBRecip) editBRecip->setEnabled(1);
			if(editCRecip) editCRecip->setEnabled(1);
			if(editAlphaRecip) editAlphaRecip->setEnabled(0);
			if(editBetaRecip) editBetaRecip->setEnabled(1);
			if(editGammaRecip) editGammaRecip->setEnabled(0);

			if(editAlpha) editAlpha->setText("90");
			if(editGamma) editGamma->setText("90");
			if(editAlphaRecip) editAlphaRecip->setText("90");
			if(editGammaRecip) editGammaRecip->setText("90");
			break;

		case CRYS_ORTHORHOMBIC:
			if(editA) editA->setEnabled(1);
			if(editB) editB->setEnabled(1);
			if(editC) editC->setEnabled(1);
			if(editAlpha) editAlpha->setEnabled(0);
			if(editBeta) editBeta->setEnabled(0);
			if(editGamma) editGamma->setEnabled(0);

			if(editARecip) editARecip->setEnabled(1);
			if(editBRecip) editBRecip->setEnabled(1);
			if(editCRecip) editCRecip->setEnabled(1);
			if(editAlphaRecip) editAlphaRecip->setEnabled(0);
			if(editBetaRecip) editBetaRecip->setEnabled(0);
			if(editGammaRecip) editGammaRecip->setEnabled(0);

			if(editAlpha) editAlpha->setText("90");
			if(editBeta) editBeta->setText("90");
			if(editGamma) editGamma->setText("90");
			if(editAlphaRecip) editAlphaRecip->setText("90");
			if(editBetaRecip) editBetaRecip->setText("90");
			if(editGammaRecip) editGammaRecip->setText("90");
			break;

		case CRYS_TETRAGONAL:
			if(editA) editA->setEnabled(1);
			if(editB) editB->setEnabled(0);
			if(editC) editC->setEnabled(1);
			if(editAlpha) editAlpha->setEnabled(0);
			if(editBeta) editBeta->setEnabled(0);
			if(editGamma) editGamma->setEnabled(0);

			if(editARecip) editARecip->setEnabled(1);
			if(editBRecip) editBRecip->setEnabled(0);
			if(editCRecip) editCRecip->setEnabled(1);
			if(editAlphaRecip) editAlphaRecip->setEnabled(0);
			if(editBetaRecip) editBetaRecip->setEnabled(0);
			if(editGammaRecip) editGammaRecip->setEnabled(0);

			if(editB) editB->setText(editA->text());
			if(editBRecip) editBRecip->setText(editARecip->text());
			if(editAlpha) editAlpha->setText("90");
			if(editBeta) editBeta->setText("90");
			if(editGamma) editGamma->setText("90");
			if(editAlphaRecip) editAlphaRecip->setText("90");
			if(editBetaRecip) editBetaRecip->setText("90");
			if(editGammaRecip) editGammaRecip->setText("90");
			break;

		case CRYS_HEXAGONAL:
		case CRYS_TRIGONAL:
			if(editA) editA->setEnabled(1);
			if(editB) editB->setEnabled(0);
			if(editC) editC->setEnabled(1);
			if(editAlpha) editAlpha->setEnabled(0);
			if(editBeta) editBeta->setEnabled(0);
			if(editGamma) editGamma->setEnabled(0);

			if(editARecip) editARecip->setEnabled(1);
			if(editBRecip) editBRecip->setEnabled(0);
			if(editCRecip) editCRecip->setEnabled(1);
			if(editAlphaRecip) editAlphaRecip->setEnabled(0);
			if(editBetaRecip) editBetaRecip->setEnabled(0);
			if(editGammaRecip) editGammaRecip->setEnabled(0);

			if(editB) editB->setText(editA->text());
			if(editBRecip) editBRecip->setText(editARecip->text());
			if(editAlpha) editAlpha->setText("90");
			if(editBeta) editBeta->setText("90");
			if(editGamma) editGamma->setText("120");
			if(editAlphaRecip) editAlphaRecip->setText("90");
			if(editBetaRecip) editBetaRecip->setText("90");
			if(editGammaRecip) editGammaRecip->setText("60");
			break;

		case CRYS_TRICLINIC:
		case CRYS_NOT_SET:
		default:
			if(editA) editA->setEnabled(1);
			if(editB) editB->setEnabled(1);
			if(editC) editC->setEnabled(1);
			if(editAlpha) editAlpha->setEnabled(1);
			if(editBeta) editBeta->setEnabled(1);
			if(editGamma) editGamma->setEnabled(1);

			if(editARecip) editARecip->setEnabled(1);
			if(editBRecip) editBRecip->setEnabled(1);
			if(editCRecip) editCRecip->setEnabled(1);
			if(editAlphaRecip) editAlphaRecip->setEnabled(1);
			if(editBetaRecip) editBetaRecip->setEnabled(1);
			if(editGammaRecip) editGammaRecip->setEnabled(1);
			break;
	}
}

#endif
