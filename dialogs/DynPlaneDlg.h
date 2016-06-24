/*
 * Dynamic Plane Dialog
 * @author tweber
 * @date 2013, jan-2015
 * @license GPLv2
 */

#ifndef __DYN_PLANE_DLG_H__
#define __DYN_PLANE_DLG_H__

#include <QDialog>
#include <QSettings>
#include <vector>
#include <memory>
#include "ui/ui_dyn_plane.h"
#include "RecipParamDlg.h"
#include "libs/qthelper.h"
#include "libs/globals.h"


class DynPlaneDlg : public QDialog, Ui::DynPlaneDlg
{ Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;
	std::vector<t_real_glob> m_vecQ, m_vecE;
	std::unique_ptr<QwtPlotWrapper> m_plotwrap;

	t_real_glob m_d2Theta = 0.;
	t_real_glob m_dEi = 5., m_dEf = 5.;

protected:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void accept() override;

protected slots:
	void cursorMoved(const QPointF& pt);
	void Calc();
	void FixedKiKfToggled();

public slots:
	void RecipParamsChanged(const RecipParams&);


public:
	DynPlaneDlg(QWidget* pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~DynPlaneDlg();
};

#endif
