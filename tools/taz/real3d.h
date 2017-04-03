/**
 * 3d unit cell drawing
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date oct-2016
 * @license GPLv2
 */

#ifndef __TAZ_REAL_3D__
#define __TAZ_REAL_3D__

#include <QDialog>
#include <QStatusBar>

#include "libs/plotgl.h"
#include "tlibs/math/linalg.h"
#include "libs/spacegroups/latticehelper.h"
#include "libs/globals.h"


class Real3DDlg : public QDialog
{Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;
	QStatusBar *m_pStatus = nullptr;
	PlotGl *m_pPlot = nullptr;

public:
	Real3DDlg(QWidget* pParent, QSettings* = 0);
	virtual ~Real3DDlg();

	void CalcPeaks(const LatticeCommon<t_real_glob>& realcommon);

protected:
	virtual void hideEvent(QHideEvent*) override;
	virtual void showEvent(QShowEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;
};

#endif
