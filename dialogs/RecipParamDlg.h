/**
 * Reciprocal Space Parameters
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 26-mar-2014
 * @license GPLv2
 */

#ifndef __RECIP_PARAMS_H__
#define __RECIP_PARAMS_H__

#include <QDialog>
#include <QSettings>
#include "libs/globals.h"
#include "ui/ui_recip_params.h"


struct RecipParams
{
	t_real_glob dki, dkf;
	t_real_glob dE, dQ, dq;
	t_real_glob d2Theta, dTheta, dKiQ, dKfQ;
	t_real_glob dAngleQVec0;

	t_real_glob Q[3], Q_rlu[3];
	t_real_glob G[3], G_rlu[3], G_rlu_accurate[3];
	t_real_glob q[3], q_rlu[3];

	t_real_glob orient_0[3], orient_1[3], orient_up[3];
};


class RecipParamDlg : public QDialog, Ui::RecipParamDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;
		RecipParams m_params;

	public:
		RecipParamDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~RecipParamDlg();

	public slots:
		void paramsChanged(const RecipParams& parms);

	protected slots:
		void KiChanged();
		void KfChanged();

		void SetGOrigin();
		void OriginChanged();

	protected:
		virtual void closeEvent(QCloseEvent *pEvt) override;
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void accept() override;
};

#endif
