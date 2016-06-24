/**
 * Real Space Parameters
 * @author tweber
 * @date 2014-2016
 * @license GPLv2
 */

#ifndef __REAL_PARAMS_H__
#define __REAL_PARAMS_H__

#include <QDialog>
#include <QSettings>
#include <vector>

#include "ui/ui_real_params.h"
#include "libs/globals.h"
#include "libs/spacegroups/spacegroup.h"
#include "tlibs/math/lattice.h"
#include "AtomsDlg.h"


struct RealParams
{
	t_real_glob dMonoTT, dSampleTT, dAnaTT;
	t_real_glob dMonoT, dSampleT, dAnaT;
	t_real_glob dLenMonoSample, dLenSampleAna, dLenAnaDet;
};


class RealParamDlg : public QDialog, Ui::RealParamDlg
{ Q_OBJECT
	protected:
		QSettings *m_pSettings = 0;

	public:
		RealParamDlg(QWidget* pParent=0, QSettings* pSett=0);
		virtual ~RealParamDlg();

	public slots:
		void paramsChanged(const RealParams& parms);
		void CrystalChanged(const tl::Lattice<t_real_glob>& latt,
			const tl::Lattice<t_real_glob>& recip,
			const SpaceGroup<t_real_glob>* pSG,
			const std::vector<AtomPos<t_real_glob>>* pAtoms);

	protected:
		virtual void closeEvent(QCloseEvent *pEvt) override;
		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void accept() override;
};

#endif
