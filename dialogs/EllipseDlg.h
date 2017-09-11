/**
 * Ellipse Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2013 - 2016
 * @license GPLv2
 */

#ifndef __RESO_ELLI_DLG__
#define __RESO_ELLI_DLG__

#include <QDialog>
#include <QSettings>
#if QT_VER>=5
	#include <QtWidgets>
#endif

#include <vector>
#include <memory>

#include "tlibs/math/linalg.h"
#include "tools/res/ellipse.h"
#include "tools/res/pop.h"
#include "ui/ui_ellipses.h"
#include "libs/qthelper.h"
#include "libs/qwthelper.h"


struct EllipseDlgParams
{
	const ublas::matrix<t_real_reso>* reso = nullptr;
	const ublas::vector<t_real_reso>* reso_v = nullptr;
	t_real_reso reso_s = 0;
	const ublas::vector<t_real_reso>* Q_avg = nullptr;

	const ublas::matrix<t_real_reso>* resoHKL = nullptr;
	const ublas::vector<t_real_reso>* reso_vHKL = nullptr;
	const ublas::vector<t_real_reso>* Q_avgHKL = nullptr;

	const ublas::matrix<t_real_reso>* resoOrient = nullptr;
	const ublas::vector<t_real_reso>* reso_vOrient = nullptr;
	const ublas::vector<t_real_reso>* Q_avgOrient = nullptr;

	const std::vector<ublas::vector<t_real_reso>>* vecMC_direct = nullptr;
	const std::vector<ublas::vector<t_real_reso>>* vecMC_HKL = nullptr;

	ResoAlgo algo = ResoAlgo::UNKNOWN;
};


class EllipseDlg : public QDialog, Ui::EllipseDlg
{ Q_OBJECT
	private:
		const char* m_pcTitle = "Resolution Ellipses";
		bool m_bExitOnAccept = 0;

	protected:
		bool m_bReady = 0;
		bool m_bCenterOn0 = 1;
		std::vector<std::unique_ptr<QwtPlotWrapper>> m_vecplotwrap;

		std::vector<struct Ellipse2d<t_real_reso>> m_elliProj;
		std::vector<struct Ellipse2d<t_real_reso>> m_elliSlice;

		std::vector<std::vector<t_real_reso>> m_vecXCurvePoints;
		std::vector<std::vector<t_real_reso>> m_vecYCurvePoints;

		std::vector<std::vector<t_real_reso>> m_vecMCXCurvePoints;
		std::vector<std::vector<t_real_reso>> m_vecMCYCurvePoints;

		QSettings *m_pSettings = 0;

	protected:
		// pointers to original parameters
		EllipseDlgParams m_params;

		// copied or modified parameters
		ublas::matrix<t_real_reso> m_reso, m_resoHKL, m_resoOrient;
		ublas::vector<t_real_reso> m_reso_v = ublas::zero_vector<t_real_reso>(4),
			m_reso_vHKL = ublas::zero_vector<t_real_reso>(4),
			m_reso_vOrient = ublas::zero_vector<t_real_reso>(4);
		t_real_reso m_reso_s = 0;
		ublas::vector<t_real_reso> m_Q_avg, m_Q_avgHKL, m_Q_avgOrient;

		ResoAlgo m_algo = ResoAlgo::UNKNOWN;

	public:
		EllipseDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~EllipseDlg();

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void closeEvent(QCloseEvent *pEvt) override;
		virtual void accept() override;

	protected slots:
		void cursorMoved(const QPointF& pt);

	public slots:
		void SetParams(const EllipseDlgParams& params);
		void Calc();
		void SetCenterOn0(bool bCenter);

	public:
		void SetTitle(const char* pcTitle);
		void SetExitOnAccept(bool b) { m_bExitOnAccept = b; }
};

#endif
