/**
 * Scan Position Plotter Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date Feb-2017
 * @license GPLv2
 */

#ifndef __SCANPOS_DLG_H__
#define __SCANPOS_DLG_H__

#include <QDialog>
#include <QSettings>

#include "ui/ui_scanpos.h"
#include "tlibs/helper/proc.h"


class ScanPosDlg : public QDialog, Ui::ScanPosDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;
		std::unique_ptr<tl::PipeProc<char>> m_pPlotProc;

	public:
		ScanPosDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~ScanPosDlg() = default;

	protected:
		std::vector<std::string> GetFiles(bool bMultiple);
		virtual void accept() override;

	protected slots:
		void LoadPlaneFromFile();
		void LoadPosFromFile();

		void Plot();
		void UpdatePlot();
		void SavePlot();

		void ButtonBoxClicked(QAbstractButton* pBtn);
};

#endif
