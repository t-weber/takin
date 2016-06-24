/**
 * Powder Line Dialog
 * @author Tobias Weber
 * @date 2013, 2-dec-2014
 * @license GPLv2
 */

#ifndef __POWDER_DLG_H__
#define __POWDER_DLG_H__

#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QSettings>
#include "ui/ui_powder.h"

#include <map>
#include <string>
#include <vector>
#include <memory>

#include "libs/spacegroups/spacegroup.h"
#include "libs/qthelper.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "tlibs/file/prop.h"
#include "AtomsDlg.h"


struct PowderLine
{
	int h, k, l;

	t_real_glob dAngle;
	std::string strAngle;

	t_real_glob dQ;
	std::string strQ;

	std::string strPeaks;

	unsigned int iMult;
	t_real_glob dFn, dFx;	// neutron/xray structure factors
	t_real_glob dIn, dIx;	// neutron/xray intensities
};


class PowderDlg : public QDialog, Ui::PowderDlg
{ Q_OBJECT
	protected:
		std::vector<t_real_glob> m_vecTT, m_vecInt;
		std::vector<t_real_glob> m_vecTTx, m_vecIntx;

		std::unique_ptr<QwtPlotWrapper> m_plotwrapN;
		std::unique_ptr<QwtPlotWrapper> m_plotwrapX;

	protected:
		bool m_bDontCalc = 1;
		QSettings *m_pSettings = 0;

		CrystalSystem m_crystalsys = CRYS_NOT_SET;
		const SpaceGroups<t_real_glob>::t_mapSpaceGroups* m_pmapSpaceGroups;

		AtomsDlg *m_pAtomsDlg = nullptr;
		std::vector<AtomPos<t_real_glob>> m_vecAtoms;

	public:
		PowderDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~PowderDlg();

	protected:
		void PlotPowderLines(const std::vector<const PowderLine*>& vecLines);

	protected slots:
		void CalcPeaks();

		void CheckCrystalType();
		void SpaceGroupChanged();
		void RepopulateSpaceGroups();

		void SaveTable();
		void SavePowder();
		void LoadPowder();

		void ShowAtomDlg();
		void ApplyAtoms(const std::vector<AtomPos<t_real_glob>>&);

		void cursorMoved(const QPointF& pt);

		virtual void dragEnterEvent(QDragEnterEvent *pEvt) override;
		virtual void dropEvent(QDropEvent *pEvt) override;

	protected:
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void accept() override;

		const SpaceGroup<t_real_glob>* GetCurSpaceGroup() const;

		void Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot);
		void Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot);
};

#endif
