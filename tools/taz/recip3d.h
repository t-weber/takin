/**
 * Scattering Triangle Tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date mar-2014
 * @license GPLv2
 */

#ifndef __TAZ_RECIP_3D__
#define __TAZ_RECIP_3D__

#include <QDialog>
#include <QStatusBar>

#include "libs/plotgl.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/geo.h"
#include "tlibs/phys/lattice.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/spacegroups/latticehelper.h"
#include "libs/globals.h"


class Recip3DDlg : public QDialog
{Q_OBJECT
protected:
	QSettings* m_pSettings = nullptr;
	QStatusBar *m_pStatus = nullptr;
	PlotGl* m_pPlot = nullptr;
	t_real_glob m_dMaxPeaks = 5.;
	t_real_glob m_dPlaneDistTolerance = 0.01;

public:
	Recip3DDlg(QWidget* pParent, QSettings* = 0);
	virtual ~Recip3DDlg();

	void CalcPeaks(const LatticeCommon<t_real_glob>& recipcommon);

	void SetPlaneDistTolerance(t_real_glob dTol) { m_dPlaneDistTolerance = dTol; }
	void SetMaxPeaks(t_real_glob dMax) { m_dMaxPeaks = dMax; }

protected:
	virtual void hideEvent(QHideEvent*) override;
	virtual void showEvent(QShowEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;
};


#endif
