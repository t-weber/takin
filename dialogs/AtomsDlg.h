/**
 * Atom Positions Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#ifndef __TAKIN_ATOMS_DLG_H__
#define __TAKIN_ATOMS_DLG_H__

#include <QDialog>
#include <QSettings>
#include "tlibs/helper/boost_hacks.h"
#include <boost/numeric/ublas/matrix.hpp>
#include <vector>
#include <string>

#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "libs/spacegroups/latticehelper.h"

#include "ui/ui_atoms.h"
namespace ublas = boost::numeric::ublas;



class AtomsDlg : public QDialog, Ui::AtomsDlg
{ Q_OBJECT
protected:
	QSettings *m_pSettings = nullptr;
	bool m_bEnableSpin = 0;
	std::string m_strErr;

protected:
	virtual void closeEvent(QCloseEvent*) override;
	void SendApplyAtoms();
	void CheckAtoms();

protected slots:
	void ButtonBoxClicked(QAbstractButton* pBtn);
	void RemoveAtom();
	void AddAtom();
	void AtomCellChanged(int iRow, int iCol);

public:
	AtomsDlg(QWidget* pParent = nullptr, QSettings *pSettings = nullptr,
		bool bEnableSpin=0);
	virtual ~AtomsDlg();

	void SetAtoms(const std::vector<xtl::AtomPos<t_real_glob>>& vecAtoms);
	std::string GetErrorString() const { return m_strErr; }
	bool ShowPossibleErrorDlg();

signals:
	void ApplyAtoms(const std::vector<xtl::AtomPos<t_real_glob>>& vecAtoms);
};


#endif
