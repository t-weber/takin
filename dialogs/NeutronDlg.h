/**
 * Neutron Properties Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date jul-2013, 28-may-2014
 * @license GPLv2
 */

#ifndef __NEUTRON_DLG_H__
#define __NEUTRON_DLG_H__

#include <QDialog>
#include <QSettings>
#include "ui/ui_neutrons.h"

#include "libs/globals.h"
#include "RecipParamDlg.h"


class NeutronDlg : public QDialog, Ui::NeutronDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;

		t_real_glob m_dExtKi = 0.;
		t_real_glob m_dExtKf = 0.;

	protected:
		void setupConstants();

	public:
		NeutronDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~NeutronDlg();

	protected slots:
		void CalcNeutronLam();
		void CalcNeutronk();
		void CalcNeutronv();
		void CalcNeutronE();
		void CalcNeutronOm();
		void CalcNeutronF();
		void CalcNeutronT();

		void RecipThetaEdited();
		void RecipTwoThetaEdited();
		void RecipKEdited();
		void RecipLamEdited();
		void RecipGEdited();
		void RecipDEdited();

		void CalcBraggRecip();

		void EnableRecipEdits();

		void Eval(const QString&);

		void paramsChanged(const RecipParams& parms);
		void SetExtKi();
		void SetExtKf();

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void accept() override;

		static void SetEditTT(QLineEdit *pEditT, QLineEdit *pEditTT);
		static void SetEditT(QLineEdit *pEditT, QLineEdit *pEditTT);

		static void SetEditK(QLineEdit *pEditLam, QLineEdit *pEditK);
		static void SetEditLam(QLineEdit *pEditLam, QLineEdit *pEditK);

		static void SetEditG(QLineEdit *pEditG, QLineEdit *pEditD);
		static void SetEditD(QLineEdit *pEditG, QLineEdit *pEditD);
};


#endif
