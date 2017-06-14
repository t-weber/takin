/**
 * 3D Ellipsoid Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date may-2013, 29-apr-2014
 * @license GPLv2
 */

#ifndef __RESO_ELLI_DLG_3D__
#define __RESO_ELLI_DLG_3D__

#include <QDialog>
#include <QSettings>
#include <QComboBox>

#include <vector>

#include "EllipseDlg.h"
#include "libs/plotgl.h"
#include "tlibs/math/linalg.h"
#include "tools/res/ellipse.h"
#include "tools/res/defs.h"


class EllipseDlg3D : public QDialog
{Q_OBJECT
	protected:
		std::vector<PlotGl*> m_pPlots;
		std::vector<Ellipsoid3d<t_real_reso>> m_elliProj;
		std::vector<Ellipsoid3d<t_real_reso>> m_elliSlice;

		QComboBox *m_pComboCoord = nullptr;
		QSettings *m_pSettings = nullptr;

		ublas::matrix<t_real_reso> m_reso, m_resoHKL, m_resoOrient;
		ublas::vector<t_real_reso> m_reso_v = ublas::zero_vector<t_real_reso>(4),
			m_reso_vHKL = ublas::zero_vector<t_real_reso>(4),
			m_reso_vOrient = ublas::zero_vector<t_real_reso>(4);
		t_real_reso m_reso_s = 0;
		ublas::vector<t_real_reso> m_Q_avg, m_Q_avgHKL, m_Q_avgOrient;
		ResoAlgo m_algo = ResoAlgo::UNKNOWN;

	protected:
		ublas::vector<t_real_reso>
		ProjRotatedVec(const ublas::matrix<t_real_reso>& rot,
			const ublas::vector<t_real_reso>& vec);

	public:
		EllipseDlg3D(QWidget* pParent, QSettings *pSett=0);
		virtual ~EllipseDlg3D();

	protected:
		virtual void hideEvent(QHideEvent*) override;
		virtual void showEvent(QShowEvent*) override;
		virtual void closeEvent(QCloseEvent*) override;
		virtual void keyPressEvent(QKeyEvent*) override;
		virtual void accept() override;

	public slots:
		void SetParams(const EllipseDlgParams& params);
		void Calc();
};

#endif
