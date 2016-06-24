/*
 * Crystal Systems
 * @author Tobias Weber
 * @date oct-2015
 * @license GPLv2
 */

#ifndef __TAKIN_CRYSSYS_H__
#define __TAKIN_CRYSSYS_H__

enum CrystalSystem
{
	CRYS_NOT_SET,

	CRYS_TRICLINIC,		// all params free
	CRYS_MONOCLINIC,	// beta=gamma=90
	CRYS_ORTHORHOMBIC,	// alpha=beta=gamma=90
	CRYS_TETRAGONAL,	// a=b, alpha=beta=gamma=90
	CRYS_TRIGONAL,		// a=b=c, alpha=beta=gamma
	CRYS_HEXAGONAL,		// a=b, gamma=120, alpha=beta=90
	CRYS_CUBIC			// a=b=c, alpha=beta=gamma=90
};

extern CrystalSystem get_crystal_system_from_laue_group(const char* pcLaue);
extern const char* get_crystal_system_name(CrystalSystem ty);

#ifndef NO_QT
#include <QLineEdit>

extern void set_crystal_system_edits(CrystalSystem crystalsys,
	QLineEdit* pA, QLineEdit* pB, QLineEdit* pC,
	QLineEdit* pAlpha, QLineEdit* pBeta, QLineEdit* pGamma,
	QLineEdit* pAReci=nullptr, QLineEdit* pBReci=nullptr, QLineEdit* pCReci=nullptr,
	QLineEdit* pAlphaReci=nullptr, QLineEdit* pBetaReci=nullptr, QLineEdit* pGammaReci=nullptr);
#endif

#endif
