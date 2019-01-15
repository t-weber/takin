/**
 * 3d Brillouin zone drawing
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date apr-2017
 * @license GPLv2
 */

#ifndef __TAZ_BZ_3D__
#define __TAZ_BZ_3D__

#include <QDialog>
#include <QStatusBar>

#include <memory>
#include <boost/optional.hpp>

#include "libs/globals.h"
#include "libs/plotgl.h"
#include "libs/plotgl2.h"
#include "libs/spacegroups/latticehelper.h"
#include "tlibs/phys/bz.h"

#include "dialogs/RecipParamDlg.h"


#ifdef USE_THREADED_GL
	using t_PlotGl = PlotGl;
#else
	using t_PlotGl = PlotGl2;
#endif


class BZ3DDlg : public QDialog
{Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;
	QStatusBar *m_pStatus = nullptr;
	std::unique_ptr<t_PlotGl> m_pPlot;

	boost::optional<std::size_t> m_iqIdx;
	ublas::vector<t_real_glob> m_vecq, m_vecq_rlu;

public:
	BZ3DDlg(QWidget* pParent, QSettings* = 0);
	virtual ~BZ3DDlg() = default;

	void RenderBZ(const tl::Brillouin3D<t_real_glob>& bz,
		const xtl::LatticeCommon<t_real_glob>& lattice,
		const std::vector<ublas::vector<t_real_glob>>* pScatPlaneVerts = nullptr,
		const std::vector<ublas::vector<t_real_glob>>* pvecSymmPts = nullptr);

protected:
	virtual void hideEvent(QHideEvent*) override;
	virtual void showEvent(QShowEvent*) override;
	virtual void closeEvent(QCloseEvent*) override;

	virtual void keyPressEvent(QKeyEvent*) override;

public slots:
	void RecipParamsChanged(const RecipParams&);
};

#endif
