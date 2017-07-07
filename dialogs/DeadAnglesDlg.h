/**
 * Dead Angles Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date jun-2017
 * @license GPLv2
 */

#ifndef __DEAD_ANGLES_H__
#define __DEAD_ANGLES_H__

#include <QDialog>
#include <QSettings>

#include "libs/globals.h"
#include "libs/globals_qt.h"

#include "ui/ui_deadangles.h"


template<class T = double>
struct DeadAngle
{
	T dAngleStart;
	T dAngleEnd;
	T dAngleOffs;

	int iCentreOn;		// 0=mono, 1=sample, 2=ana
	int iRelativeTo;	// 0=crystal angle, 1=in axis, 2=out axis
};


class DeadAnglesDlg : public QDialog, Ui::DeadAnglesDlg
{ Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;

protected:
	virtual void closeEvent(QCloseEvent*) override;
	void SendApplyDeadAngles();

protected slots:
	void ButtonBoxClicked(QAbstractButton* pBtn);
	void RemoveAngle();
	void AddAngle();

public:
	DeadAnglesDlg(QWidget* pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~DeadAnglesDlg() = default;

	void SetDeadAngles(const std::vector<DeadAngle<t_real_glob>>& vecAngle);

signals:
	void ApplyDeadAngles(const std::vector<DeadAngle<t_real_glob>>& vecAngle);
};

#endif
