/**
 * Component calculations (formerly only TOF-specific)
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2017
 * @license GPLv2
 */

#ifndef __TOFCALC_DLG_H__
#define __TOFCALC_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_tof.h"

#include "libs/globals.h"

#include <vector>
#include <string>


class TOFDlg : public QDialog, Ui::TofCalcDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = nullptr;

		std::vector<QRadioButton*> m_vecRadioBoxes;
		std::vector<std::string> m_vecRadioNames;

		std::vector<QCheckBox*> m_vecCheckBoxes;
		std::vector<std::string> m_vecCheckNames;

		std::vector<QLineEdit*> m_vecEditBoxes;
		std::vector<std::string> m_vecEditNames;

	public:
		TOFDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~TOFDlg() = default;

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void accept() override;
		void ReadLastConfig();
		void WriteLastConfig();

	protected slots:
		void CalcChopper();
		void EnableChopperEdits();

		void CalcDiv();
		void EnableDivEdits();

		void CalcSel();
		void EnableSelEdits();

		void CalcFoc();
};


#endif
