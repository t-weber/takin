/*
 * TOF Calculations
 * @author tweber
 * @date feb-2017
 * @license GPLv2
 */

#ifndef __TOFCALC_DLG_H__
#define __TOFCALC_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_tof.h"

#include "libs/globals.h"


class TOFDlg : public QDialog, Ui::TofCalcDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = nullptr;

	public:
		TOFDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~TOFDlg() = default;

	protected slots:
		void CalcChopper();
		void EnableChopperEdits();

		void CalcDiv();
		void EnableDivEdits();

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void accept() override;
};


#endif
