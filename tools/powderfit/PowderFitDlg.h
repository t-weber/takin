/**
 * Powder Fit Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date Mar-2018
 * @license GPLv2
 */

#ifndef __POWDERFIT_DLG_H__
#define __POWDERFIT_DLG_H__

#include <QDialog>
#include <QSettings>

#include "ui/ui_powderfit.h"
#include "tlibs/helper/proc.h"


class PowderFitDlg : public QDialog, Ui::PowderFitDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;
		std::unique_ptr<tl::PipeProc<char>> m_pPlotProc;

	public:
		PowderFitDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~PowderFitDlg() = default;

	protected:
		virtual void accept() override;

	protected slots:
		void Calc();
		void Plot();
		void SavePlot();

		void ButtonBoxClicked(QAbstractButton* pBtn);
};

#endif
