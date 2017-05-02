/**
 * Takin / TAS tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2014
 * @license GPLv2
 */

#ifndef __TAZ_H__
#define __TAZ_H__

#if !defined NO_NET
	#include "dialogs/SrvDlg.h"
	#include "dialogs/NetCacheDlg.h"
	#include "dialogs/ScanMonDlg.h"
	#include "nicos.h"
	#include "sics.h"
#endif

//#include <QDialog>
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QVariant>
#include <QSignalMapper>

#include <string>
#include <vector>

#include "ui/ui_taz.h"
#include "scattering_triangle.h"
#include "real_lattice.h"
#include "proj_lattice.h"
#include "tas_layout.h"
#include "tof_layout.h"

#include "dialogs/RecipParamDlg.h"
#include "dialogs/RealParamDlg.h"
#include "dialogs/EllipseDlg.h"
#include "tools/res/ResoDlg.h"
#include "tools/monteconvo/ConvoDlg.h"
#include "tools/scanviewer/scanviewer.h"
#include "tools/scanpos/ScanPosDlg.h"
#include "dialogs/SpurionDlg.h"
#include "dialogs/NeutronDlg.h"
#include "dialogs/GotoDlg.h"
#include "dialogs/PowderDlg.h"
#include "dialogs/DispDlg.h"
#include "dialogs/SettingsDlg.h"
#include "dialogs/DWDlg.h"
#include "dialogs/DynPlaneDlg.h"
#include "dialogs/FormfactorDlg.h"
#include "dialogs/AtomsDlg.h"
#include "dialogs/AboutDlg.h"

#if !defined NO_3D
	#include "recip3d.h"
	#include "real3d.h"
	#include "bz3d.h"
	#include "dialogs/EllipseDlg3D.h"
#endif

#include "tools/sglist/SgListDlg.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/spacegroups/latticehelper.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "tlibs/phys/lattice.h"


class TazDlg : public QMainWindow, Ui::TazDlg
{ Q_OBJECT
	private:
		void DeleteDialogs();

	private:
		bool m_bUpdateRecipEdits = 1;

		QAction *m_pSmallq = nullptr, *m_pSnapSmallq = nullptr;
		QAction *m_pGoto = nullptr;
		QAction *m_pBZ = nullptr, *m_pWS = nullptr;
		QAction *m_pEwaldSphereNone = nullptr,
			*m_pEwaldSphereKi = nullptr, *m_pEwaldSphereKf = nullptr;
		QAction *m_pShowRealQDir = nullptr;
		QAction *m_pProjGnom = nullptr, *m_pProjStereo = nullptr,
			*m_pProjPara = nullptr, *m_pProjPersp = nullptr;

		std::vector<QLineEdit*> m_vecEdits_real;
		std::vector<QLineEdit*> m_vecEdits_recip;
		std::vector<QLineEdit*> m_vecEdits_plane;
		std::vector<QLineEdit*> m_vecEdits_monoana;

		//std::vector<QDoubleSpinBox*> m_vecSpinBoxesSample;
		std::vector<QCheckBox*> m_vecCheckBoxesSenses;

		std::vector<std::string> m_vecEditNames_real;
		std::vector<std::string> m_vecEditNames_recip;
		std::vector<std::string> m_vecEditNames_plane;
		std::vector<std::string> m_vecEditNames_monoana;

		std::vector<std::string> m_vecSpinBoxNamesSample;
		std::vector<std::string> m_vecCheckBoxNamesSenses;

	protected:
		static const t_real_glob s_dPlaneDistTolerance;

		bool m_bReady = false;
		QSettings m_settings;
		SettingsDlg *m_pSettingsDlg = nullptr;

		QLabel* m_pStatusMsg = nullptr;
		QLabel* m_pCoordQStatusMsg = nullptr;
		QLabel* m_pCoordCursorStatusMsg = nullptr;

		QMenu *m_pMenuViewRecip = nullptr;
		QMenu *m_pMenuViewReal = nullptr;

		QSignalMapper *m_pMapperRecent = nullptr;
		QSignalMapper *m_pMapperRecentImport = nullptr;
		QMenu *m_pMenuRecent = nullptr;
		QMenu *m_pMenuRecentImport = nullptr;

		// reciprocal lattice
		LatticeCommon<t_real_glob> m_latticecommon;
		ScatteringTriangleView *m_pviewRecip = nullptr;
		ScatteringTriangleScene m_sceneRecip;
		ProjLatticeView *m_pviewProjRecip = nullptr;
		ProjLatticeScene m_sceneProjRecip;

		// real lattice
		TasLayoutView *m_pviewReal = nullptr;
		TasLayoutScene m_sceneReal;
		TofLayoutView *m_pviewTof = nullptr;
		TofLayoutScene m_sceneTof;
		LatticeView *m_pviewRealLattice = nullptr;
		LatticeScene m_sceneRealLattice;

		std::string m_strCurFile;
		static const std::string s_strTitle;

		std::vector<AtomPos<t_real_glob>> m_vecAtoms;
		CrystalSystem m_crystalsys = CRYS_NOT_SET;

		RecipParamDlg m_dlgRecipParam;
		RealParamDlg m_dlgRealParam;

		ResoDlg *m_pReso = nullptr;
		EllipseDlg *m_pEllipseDlg = nullptr;
		ConvoDlg *m_pConvoDlg = nullptr;

		SpurionDlg *m_pSpuri = nullptr;
		NeutronDlg *m_pNeutronDlg = nullptr;
		GotoDlg *m_pGotoDlg = nullptr;
		PowderDlg *m_pPowderDlg = nullptr;
		DispDlg *m_pDispDlg = nullptr;
		DWDlg *m_pDWDlg = nullptr;
		DynPlaneDlg* m_pDynPlaneDlg = nullptr;
		FormfactorDlg* m_pFormfactorDlg = nullptr;
		AtomsDlg *m_pAtomsDlg = nullptr;
		AboutDlg *m_pAboutDlg = nullptr;

		ScanViewerDlg *m_pScanViewer = nullptr;
		ScanPosDlg *m_pScanPos = nullptr;

#if !defined NO_NET
		SrvDlg *m_pSrvDlg = nullptr;
		NetCache *m_pNetCache = nullptr;
		NetCacheDlg *m_pNetCacheDlg = nullptr;
		ScanMonDlg *m_pScanMonDlg = nullptr;
#endif

#if !defined NO_3D
		Recip3DDlg *m_pRecip3d = nullptr;
		Real3DDlg *m_pReal3d = nullptr;
		BZ3DDlg *m_pBZ3d = nullptr;
		EllipseDlg3D *m_pEllipseDlg3D = nullptr;
#endif

		SgListDlg *m_pSgListDlg = nullptr;

	protected:
		void InitReso();
		void InitGoto();
		void InitResoConv();

		void RotatePlane(unsigned iAxis, t_real_glob dAngle);

		virtual void showEvent(QShowEvent *pEvt) override;
		virtual void closeEvent(QCloseEvent* pEvt) override;

		virtual void dragEnterEvent(QDragEnterEvent *pEvt) override;
		virtual void dropEvent(QDropEvent *pEvt) override;

		virtual void keyPressEvent(QKeyEvent *pEvt) override;
		virtual void keyReleaseEvent(QKeyEvent *pEvt) override;

	public:
		TazDlg(QWidget *pParent);
		TazDlg() : TazDlg(0) { }
		virtual ~TazDlg();

		bool Load(const char* pcFile);
		bool Import(const char* pcFile);

	protected:
		void ExportSceneSVG(QGraphicsScene& scene);
		void emitSampleParams();

	protected slots:
		void CalcPeaks();
		void CalcPeaksRecip();
		void UpdateDs();

		void SetCrystalType();
		void CheckCrystalType();

		void UpdateSampleSense();
		void UpdateMonoSense();
		void UpdateAnaSense();
		void EnableSmallq(bool bEnable);
		void EnableBZ(bool bEnable);
		void EnableWS(bool bEnable);
		void EnableRealQDir(bool bEnable);
		void ShowEwaldSphere();
		void RecipProjChanged();

		void RecipContextMenu(const QPoint&);
		void RealContextMenu(const QPoint&);

		void ShowHelp();
		void ShowDevelDoc();
		void ShowAbout();

		void New();
		bool Save();
		bool SaveAs();
		bool Load();
		bool Import();

		void ShowScanViewer();
		void ShowScanPos();

		bool LoadFile(const QString& strFile);
		bool ImportFile(const QString& strFile);

		void ExportReal();
		void ExportTof();
		void ExportRealLattice();
		void ExportRecip();
		void ExportProj();
		void ExportBZImage();
		void ExportBZ3DModel();
		void ExportWSImage();
		void ExportUCModel();

		void RepopulateSpaceGroups();

		void ShowRecipParams();
		void ShowRealParams();

		void ShowResoParams();
		void ShowResoEllipses();
		void ShowResoConv();

		void ShowNeutronDlg();
		void ShowGotoDlg();
		void ShowPowderDlg();
		void ShowDispDlg();
		void ShowSettingsDlg();
		void ShowDWDlg();
		void ShowDynPlaneDlg();

		void Show3D();
		void Show3DBZ();
		void Show3DReal();
		void ShowResoEllipses3D();

		void ShowSgListDlg();
		void ShowFormfactorDlg();

		void ShowAtomsDlg();
		void ApplyAtoms(const std::vector<AtomPos<t_real_glob>>& vecAtoms);


		void ShowSpurions();
		void spurionInfo(const tl::ElasticSpurion& spuris,
			const std::vector<tl::InelasticSpurion<t_real_glob>>& vecInelCKI,
			const std::vector<tl::InelasticSpurion<t_real_glob>>& vecInelCKF);
		void recipParamsChanged(const RecipParams&);

		void ShowConnectDlg();

		void ConnectTo(int iSys, const QString& strHost, const QString& strPort,
			const QString& strUser, const QString& strPass);
		void Disconnect();
		void NetRefresh();
		void ShowNetCache();
		void ShowNetScanMonitor();

		void Connected(const QString& strHost, const QString& strSrv);
		void Disconnected();
		void VarsChanged(const CrystalOptions& crys, const TriangleOptions& triag);

		void RecipCoordsChanged(t_real_glob dh, t_real_glob dk, t_real_glob dl,
			bool bHasNearest, t_real_glob dNearestH, t_real_glob dNearestK, t_real_glob dNearestL);
		void RealCoordsChanged(t_real_glob dh, t_real_glob dk, t_real_glob dl,
			bool bHasNearest, t_real_glob dNearestH, t_real_glob dNearestK, t_real_glob dNearestL);

		void SettingsChanged();

		void RecipNodeEvent(bool bStarted);
		void RealNodeEvent(bool bStarted);
		void TofNodeEvent(bool bStarted);

	signals:
		void ResoParamsChanged(const ResoParams& resoparams);
		void SampleParamsChanged(const SampleParams& parms);
};

#endif
