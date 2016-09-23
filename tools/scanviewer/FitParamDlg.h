/**
 * Fit Parameters Dialog
 * @author Tobias Weber
 * @date sep-2016
 * @license GPLv2
 */

#ifndef __FITPARAMS_DLG_H__
#define __FITPARAMS_DLG_H__

#include <QDialog>
#include <QSettings>

#include "tlibs/string/string.h"
#include "libs/globals.h"
#include "ui/ui_fitparams.h"


class FitParamDlg : public QDialog, Ui::FitParamDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = nullptr;

	public:
		FitParamDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~FitParamDlg();


		t_real_glob GetAmp() const { return tl::str_to_var<t_real_glob>(editAmp->text().toStdString()); }
		t_real_glob GetAmpErr() const { return tl::str_to_var<t_real_glob>(editAmpErr->text().toStdString()); }
		bool GetAmpFixed() const { return checkAmp->isChecked(); }

		t_real_glob GetSig() const { return tl::str_to_var<t_real_glob>(editSig->text().toStdString()); }
		t_real_glob GetSigErr() const { return tl::str_to_var<t_real_glob>(editSigErr->text().toStdString()); }
		bool GetSigFixed() const { return checkSig->isChecked(); }

		t_real_glob GetHWHM() const { return tl::str_to_var<t_real_glob>(editHWHM->text().toStdString()); }
		t_real_glob GetHWHMErr() const { return tl::str_to_var<t_real_glob>(editHWHMErr->text().toStdString()); }
		bool GetHWHMFixed() const { return checkHWHM->isChecked(); }

		t_real_glob GetX0() const { return tl::str_to_var<t_real_glob>(editX0->text().toStdString()); }
		t_real_glob GetX0Err() const { return tl::str_to_var<t_real_glob>(editX0Err->text().toStdString()); }
		bool GetX0Fixed() const { return checkX0->isChecked(); }

		t_real_glob GetOffs() const { return tl::str_to_var<t_real_glob>(editOffs->text().toStdString()); }
		t_real_glob GetOffsErr() const { return tl::str_to_var<t_real_glob>(editOffsErr->text().toStdString()); }
		bool GetOffsFixed() const { return checkOffs->isChecked(); }


		void SetAmp(t_real_glob d) { editAmp->setText(tl::var_to_str(d, g_iPrec).c_str()); }
		void SetAmpErr(t_real_glob d) { editAmpErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetSig(t_real_glob d) { editSig->setText(tl::var_to_str(d, g_iPrec).c_str()); }
		void SetSigErr(t_real_glob d) { editSigErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetHWHM(t_real_glob d) { editHWHM->setText(tl::var_to_str(d, g_iPrec).c_str()); }
		void SetHWHMErr(t_real_glob d) { editHWHMErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetX0(t_real_glob d) { editX0->setText(tl::var_to_str(d, g_iPrec).c_str()); }
		void SetX0Err(t_real_glob d) { editX0Err->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetOffs(t_real_glob d) { editOffs->setText(tl::var_to_str(d, g_iPrec).c_str()); }
		void SetOffsErr(t_real_glob d) { editOffsErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }


		bool WantParams() const { return checkUseParams->isChecked(); }

	protected:
		virtual void accept() override;
};

#endif
