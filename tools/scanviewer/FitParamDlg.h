/**
 * Fit Parameters Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date sep-2016
 * @license GPLv2
 */

#ifndef __FITPARAMS_DLG_H__
#define __FITPARAMS_DLG_H__

#include <QDialog>
#include <QSettings>
#include <QLabel>

#include "tlibs/string/string.h"
#include "libs/globals.h"
#include "ui/ui_fitparams.h"


class FitParamDlg : public QDialog, Ui::FitParamDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = nullptr;

	private:
		void SetBold(QLabel* pLab, bool bBold = 1);


	public:
		FitParamDlg(QWidget* pParent=0, QSettings *pSett=0);
		virtual ~FitParamDlg();


		t_real_glob GetAmp() const { return tl::str_to_var_parse<t_real_glob>(editAmp->text().toStdString()); }
		t_real_glob GetAmpErr() const { return tl::str_to_var_parse<t_real_glob>(editAmpErr->text().toStdString()); }
		bool GetAmpFixed() const { return checkAmp->isChecked(); }

		t_real_glob GetSig() const { return tl::str_to_var_parse<t_real_glob>(editSig->text().toStdString()); }
		t_real_glob GetSigErr() const { return tl::str_to_var_parse<t_real_glob>(editSigErr->text().toStdString()); }
		bool GetSigFixed() const { return checkSig->isChecked(); }

		t_real_glob GetHWHM() const { return tl::str_to_var_parse<t_real_glob>(editHWHM->text().toStdString()); }
		t_real_glob GetHWHMErr() const { return tl::str_to_var_parse<t_real_glob>(editHWHMErr->text().toStdString()); }
		bool GetHWHMFixed() const { return checkHWHM->isChecked(); }

		t_real_glob GetX0() const { return tl::str_to_var_parse<t_real_glob>(editX0->text().toStdString()); }
		t_real_glob GetX0Err() const { return tl::str_to_var_parse<t_real_glob>(editX0Err->text().toStdString()); }
		bool GetX0Fixed() const { return checkX0->isChecked(); }

		t_real_glob GetOffs() const { return tl::str_to_var_parse<t_real_glob>(editOffs->text().toStdString()); }
		t_real_glob GetOffsErr() const { return tl::str_to_var_parse<t_real_glob>(editOffsErr->text().toStdString()); }
		bool GetOffsFixed() const { return checkOffs->isChecked(); }

		t_real_glob GetSlope() const { return tl::str_to_var_parse<t_real_glob>(editSlope->text().toStdString()); }
		t_real_glob GetSlopeErr() const { return tl::str_to_var_parse<t_real_glob>(editSlopeErr->text().toStdString()); }
		bool GetSlopeFixed() const { return checkSlope->isChecked(); }

		t_real_glob GetFreq() const { return tl::str_to_var_parse<t_real_glob>(editFreq->text().toStdString()); }
		t_real_glob GetFreqErr() const { return tl::str_to_var_parse<t_real_glob>(editFreqErr->text().toStdString()); }
		bool GetFreqFixed() const { return checkFreq->isChecked(); }

		t_real_glob GetPhase() const { return tl::str_to_var_parse<t_real_glob>(editPhase->text().toStdString()); }
		t_real_glob GetPhaseErr() const { return tl::str_to_var_parse<t_real_glob>(editPhaseErr->text().toStdString()); }
		bool GetPhaseFixed() const { return checkPhase->isChecked(); }


		void SetAmp(t_real_glob d) { editAmp->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelAmp, 1); }
		void SetAmpErr(t_real_glob d) { editAmpErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetSig(t_real_glob d) { editSig->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelSig, 1); }
		void SetSigErr(t_real_glob d) { editSigErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetHWHM(t_real_glob d) { editHWHM->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelHWHM, 1); }
		void SetHWHMErr(t_real_glob d) { editHWHMErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetX0(t_real_glob d) { editX0->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelX0, 1); }
		void SetX0Err(t_real_glob d) { editX0Err->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetOffs(t_real_glob d) { editOffs->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelOffs, 1); }
		void SetOffsErr(t_real_glob d) { editOffsErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetSlope(t_real_glob d) { editSlope->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelSlope, 1); }
		void SetSlopeErr(t_real_glob d) { editSlopeErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetFreq(t_real_glob d) { editFreq->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelFreq, 1); }
		void SetFreqErr(t_real_glob d) { editFreqErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void SetPhase(t_real_glob d) { editPhase->setText(tl::var_to_str(d, g_iPrec).c_str()); SetBold(labelPhase, 1); }
		void SetPhaseErr(t_real_glob d) { editPhaseErr->setText(tl::var_to_str(d, g_iPrec).c_str()); }

		void UnsetAllBold();

		bool WantParams() const { return checkUseParams->isChecked(); }

	protected:
		virtual void accept() override;
};

#endif
