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

#include <memory>

#include "libs/plotgl.h"
#include "tlibs/math/linalg.h"
#include "libs/spacegroups/latticehelper.h"
#include "tlibs/phys/bz.h"
#include "libs/globals.h"


class Real3DDlg : public QDialog
{Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;
	QStatusBar *m_pStatus = nullptr;
	std::unique_ptr<PlotGl> m_pPlot;

public:
	Real3DDlg(QWidget* pParent, QSettings* = 0);
	virtual ~Real3DDlg() = default;

	void CalcPeaks(const tl::Brillouin3D<t_real_glob>& ws,
		const xtl::LatticeCommon<t_real_glob>& realcommon);

protected:
	virtual void hideEvent(QHideEvent*) override;
	virtual void showEvent(QShowEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;

	virtual void keyPressEvent(QKeyEvent*) override;
};

#endif
