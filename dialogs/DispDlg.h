/**
 * Dispersion Dialog
 * @author Tobias Weber
 * @date may-2016
 * @license GPLv2
 */

#ifndef __DISP_DLG_H__
#define __DISP_DLG_H__

#include "ui/ui_disp.h"

#include "libs/spacegroups/spacegroup.h"
#include "libs/qthelper.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "tlibs/file/prop.h"
#include "AtomsDlg.h"

#include <map>
#include <string>
#include <vector>
#include <memory>

#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QSettings>


class DispDlg : public QDialog, Ui::DispDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;
		bool m_bDontCalc = 1;

		CrystalSystem m_crystalsys = CRYS_NOT_SET;
		const SpaceGroups<t_real_glob>::t_mapSpaceGroups* m_pmapSpaceGroups;

		AtomsDlg *m_pAtomsDlg = nullptr;
		std::vector<AtomPos<t_real_glob>> m_vecAtoms;

		std::vector<t_real_glob> m_vecFerroQ, m_vecFerroE;
		std::unique_ptr<QwtPlotWrapper> m_plotwrapFerro;

	public:
		DispDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~DispDlg();

	protected slots:
		void Calc();

		void CheckCrystalType();
		void SpaceGroupChanged();
		void RepopulateSpaceGroups();

		void Save();
		void Load();

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
