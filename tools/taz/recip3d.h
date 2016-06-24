/*
 * Scattering Triangle Tool
 * @author tweber
 * @date mar-2014
 * @license GPLv2
 */

#ifndef __TAZ_RECIP_3D__
#define __TAZ_RECIP_3D__

#include <QDialog>
#include "libs/plotgl.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/geo.h"
#include "tlibs/math/lattice.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/spacegroups/latticehelper.h"
#include "libs/globals.h"


class Recip3DDlg : public QDialog
{Q_OBJECT
protected:
	PlotGl* m_pPlot;
	t_real_glob m_dMaxPeaks = 5.;
	t_real_glob m_dPlaneDistTolerance = 0.01;

public:
	Recip3DDlg(QWidget* pParent, QSettings* = 0);
	virtual ~Recip3DDlg();

	void CalcPeaks(const LatticeCommon<t_real_glob>& recipcommon);

	void SetPlaneDistTolerance(t_real_glob dTol) { m_dPlaneDistTolerance = dTol; }
	void SetMaxPeaks(t_real_glob dMax) { m_dMaxPeaks = dMax; }

protected:
	void hideEvent(QHideEvent*);
	void showEvent(QShowEvent*);
};


#endif
