/**
 * Form Factor & Scattering Length Dialog
 * @author tweber
 * @date nov-2015
 * @license GPLv2
 */

#ifndef __TAKIN_FF_DLG_H__
#define __TAKIN_FF_DLG_H__

#include <QDialog>
#include <QSettings>
#include <vector>
#include <memory>
#include "ui/ui_formfactors.h"
#include "libs/qthelper.h"
#include "libs/globals.h"


class FormfactorDlg : public QDialog, Ui::FormFactorDlg
{ Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;

	// form factors
	std::vector<t_real_glob> m_vecQ, m_vecFF;
	std::unique_ptr<QwtPlotWrapper> m_plotwrap;

	// mag form factors
	std::vector<t_real_glob> m_vecQ_m, m_vecFF_m;
	std::unique_ptr<QwtPlotWrapper> m_plotwrap_m;

	// scattering lengths
	std::vector<t_real_glob> m_vecElem, m_vecSc;
	std::unique_ptr<QwtPlotWrapper> m_plotwrapSc;


protected:
	virtual void closeEvent(QCloseEvent* pEvt) override;

	void SetupAtoms();
	void SetupMagAtoms();

protected slots:
	void SearchAtom(const QString& qstr);
	void AtomSelected(QListWidgetItem *pItem, QListWidgetItem *pItemPrev);

	void SearchMagAtom(const QString& qstr);
	void MagAtomSelected(QListWidgetItem *pItem, QListWidgetItem *pItemPrev);
	void RefreshMagAtom();
	void CalcTermSymbol(const QString& qstr);

	void SearchSLAtom(const QString& qstr);
	void PlotScatteringLengths();
	void SetupScatteringLengths();

	void cursorMoved(const QPointF& pt);

public:
	FormfactorDlg(QWidget* pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~FormfactorDlg();
};

#endif
