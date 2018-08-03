/**
 * TAS tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2014
 * @license GPLv2
 */

#include "taz.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QScrollBar>
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>

#include "tlibs/phys/lattice.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/string/string.h"
#include "tlibs/helper/flags.h"
#include "tlibs/log/log.h"
#include "libs/qt/recent.h"

namespace algo = boost::algorithm;
namespace fs = boost::filesystem;

using t_real = t_real_glob;
const std::string TazDlg::s_strTitle = "Takin";
const t_real_glob TazDlg::s_dPlaneDistTolerance = std::cbrt(tl::get_epsilon<t_real_glob>());

//#define NO_HELP_ASSISTANT


TazDlg::TazDlg(QWidget* pParent, const std::string& strLogFile)
	: QMainWindow(pParent), m_settings("tobis_stuff", "takin"),
		m_pSettingsDlg(new SettingsDlg(this, &m_settings)),
		m_strLogFile(strLogFile),
		m_pStatusMsg(new QLabel(this)),
		m_pCoordQStatusMsg(new QLabel(this)),
		m_pCoordCursorStatusMsg(new QLabel(this)),
		m_sceneRecip(this),
		m_dlgRecipParam(this, &m_settings),
		m_dlgRealParam(this, &m_settings),
		m_pGotoDlg(new GotoDlg(this, &m_settings))
{
	//log_debug("In ", __func__, ".");
	const bool bSmallqVisible = 0;
	const bool bCoordAxesVisible = 1;
	const bool bBZVisible = 1;
	const bool bWSVisible = 1;

	this->setupUi(this);
	this->setWindowTitle(s_strTitle.c_str());
	this->setFont(g_fontGen);
	this->setWindowIcon(load_icon("res/icons/takin.svg"));

	btnAtoms->setEnabled(g_bHasScatlens);

	if(m_settings.contains("main/geo"))
	{
		QByteArray geo = m_settings.value("main/geo").toByteArray();
		restoreGeometry(geo);
	}
	/*if(m_settings.contains("main/width") && m_settings.contains("main/height"))
	{
		int iW = m_settings.value("main/width").toInt();
		int iH = m_settings.value("main/height").toInt();
		resize(iW, iH);
	}*/

	m_pStatusMsg->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pCoordQStatusMsg->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pCoordCursorStatusMsg->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	// prevents window resizing by message length
	for(QLabel* pLabel : {m_pStatusMsg/*, m_pCoordQStatusMsg, m_pCoordCursorStatusMsg*/})
		pLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	QStatusBar *pStatusBar = new QStatusBar(this);
	pStatusBar->addWidget(m_pStatusMsg, 1);
	pStatusBar->addPermanentWidget(m_pCoordQStatusMsg, 0);
	pStatusBar->addPermanentWidget(m_pCoordCursorStatusMsg, 0);
	m_pCoordQStatusMsg->setMinimumWidth(350);
	m_pCoordQStatusMsg->setAlignment(Qt::AlignCenter);
	m_pCoordCursorStatusMsg->setMinimumWidth(325);
	m_pCoordCursorStatusMsg->setAlignment(Qt::AlignCenter);
	this->setStatusBar(pStatusBar);

	// --------------------------------------------------------------------------------

	m_vecEdits_real =
	{
		editA, editB, editC,
		editAlpha, editBeta, editGamma
	};
	m_vecEdits_recip =
	{
		editARecip, editBRecip, editCRecip,
		editAlphaRecip, editBetaRecip, editGammaRecip
	};

	m_vecEdits_plane =
	{
		editScatX0, editScatX1, editScatX2,
		editScatY0, editScatY1, editScatY2,

		editRealX0, editRealX1, editRealX2,
		editRealY0, editRealY1, editRealY2,
	};
	m_vecEdits_monoana =
	{
		editMonoD, editAnaD
	};

	//m_vecSpinBoxesSample = { spinRotPhi, spinRotTheta, spinRotPsi };
	m_vecCheckBoxesSenses = { checkSenseM, checkSenseS, checkSenseA };


	m_vecEditNames_real =
	{
		"sample/a", "sample/b", "sample/c",
		"sample/alpha", "sample/beta", "sample/gamma"
	};
	m_vecEditNames_recip =
	{
		"sample/a_recip", "sample/b_recip", "sample/c_recip",
		"sample/alpha_recip", "sample/beta_recip", "sample/gamma_recip"
	};
	m_vecEditNames_plane =
	{
		"plane/x0", "plane/x1", "plane/x2",
		"plane/y0", "plane/y1", "plane/y2",

		"plane/real_x0", "plane/real_x1", "plane/real_x2",
		"plane/real_y0", "plane/real_y1", "plane/real_y2",
	};
	m_vecEditNames_monoana =
	{
		"tas/mono_d", "tas/ana_d"
	};

	m_vecSpinBoxNamesSample = {"sample/phi", "sample/theta", "sample/psi"};
	m_vecCheckBoxNamesSenses = {"tas/sense_m", "tas/sense_s", "tas/sense_a"};


	// recip
	m_pviewRecip = new ScatteringTriangleView(groupRecip);
	groupRecip->addTab(m_pviewRecip, "Reciprocal Lattice");

	m_pviewProjRecip = new ProjLatticeView(groupRecip);
	groupRecip->addTab(m_pviewProjRecip, "Projection");

	m_pviewRecip->setScene(&m_sceneRecip);
	m_pviewProjRecip->setScene(&m_sceneProjRecip);

	if(m_settings.contains("main/recip_tab"))
		groupRecip->setCurrentIndex(m_settings.value("main/recip_tab").value<int>());


	// real
	m_pviewRealLattice = new LatticeView(groupReal);
	groupReal->addTab(m_pviewRealLattice, "Real Lattice");

	m_pviewReal = new TasLayoutView(groupReal);
	groupReal->addTab(m_pviewReal, "TAS Instrument");

	m_pviewTof = new TofLayoutView(groupReal);
	groupReal->addTab(m_pviewTof, "TOF Instrument");

	m_pviewRealLattice->setScene(&m_sceneRealLattice);
	m_pviewReal->setScene(&m_sceneReal);
	m_pviewTof->setScene(&m_sceneTof);

	if(m_settings.contains("main/real_tab"))
		groupReal->setCurrentIndex(m_settings.value("main/real_tab").value<int>());



	QObject::connect(m_pSettingsDlg, SIGNAL(SettingsChanged()), this, SLOT(SettingsChanged()));

	QObject::connect(&m_sceneReal, SIGNAL(nodeEvent(bool)), this, SLOT(RealNodeEvent(bool)));
	QObject::connect(&m_sceneRecip, SIGNAL(nodeEvent(bool)), this, SLOT(RecipNodeEvent(bool)));
	QObject::connect(&m_sceneTof, SIGNAL(nodeEvent(bool)), this, SLOT(TofNodeEvent(bool)));

	// TAS
	QObject::connect(&m_sceneRecip, SIGNAL(triangleChanged(const TriangleOptions&)),
		&m_sceneReal, SLOT(triangleChanged(const TriangleOptions&)));
	QObject::connect(&m_sceneReal, SIGNAL(tasChanged(const TriangleOptions&)),
		&m_sceneRecip, SLOT(tasChanged(const TriangleOptions&)));
	QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
		&m_sceneReal, SLOT(recipParamsChanged(const RecipParams&)));

	// TOF
	QObject::connect(&m_sceneRecip, SIGNAL(triangleChanged(const TriangleOptions&)),
		&m_sceneTof, SLOT(triangleChanged(const TriangleOptions&)));
	QObject::connect(&m_sceneTof, SIGNAL(tasChanged(const TriangleOptions&)),
		&m_sceneRecip, SLOT(tasChanged(const TriangleOptions&)));
	QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
		&m_sceneTof, SLOT(recipParamsChanged(const RecipParams&)));

	// connections between instruments
	QObject::connect(&m_sceneTof, SIGNAL(tasChanged(const TriangleOptions&)),
		&m_sceneReal, SLOT(triangleChanged(const TriangleOptions&)));
	QObject::connect(&m_sceneReal, SIGNAL(tasChanged(const TriangleOptions&)),
		&m_sceneTof, SLOT(triangleChanged(const TriangleOptions&)));

	// scale factor
	if(m_pviewRecip)
		QObject::connect(m_pviewRecip, SIGNAL(scaleChanged(t_real_glob)),
			&m_sceneRecip, SLOT(scaleChanged(t_real_glob)));
	if(m_pviewProjRecip)
		QObject::connect(m_pviewProjRecip, SIGNAL(scaleChanged(t_real_glob)),
			&m_sceneProjRecip, SLOT(scaleChanged(t_real_glob)));
	if(m_pviewRealLattice)
		QObject::connect(m_pviewRealLattice, SIGNAL(scaleChanged(t_real_glob)),
			&m_sceneRealLattice, SLOT(scaleChanged(t_real_glob)));
	if(m_pviewReal)
		QObject::connect(m_pviewReal, SIGNAL(scaleChanged(t_real_glob)),
			&m_sceneReal, SLOT(scaleChanged(t_real_glob)));
	if(m_pviewTof)
		QObject::connect(m_pviewTof, SIGNAL(scaleChanged(t_real_glob)),
			&m_sceneTof, SLOT(scaleChanged(t_real_glob)));

	// parameter dialogs
	QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
		&m_dlgRecipParam, SLOT(paramsChanged(const RecipParams&)));
	QObject::connect(&m_sceneReal, SIGNAL(paramsChanged(const RealParams&)),
		&m_dlgRealParam, SLOT(paramsChanged(const RealParams&)));

	// cursor position
	QObject::connect(&m_sceneRecip, SIGNAL(coordsChanged(t_real_glob, t_real_glob, t_real_glob, bool, t_real_glob, t_real_glob, t_real_glob)),
		this, SLOT(RecipCoordsChanged(t_real_glob, t_real_glob, t_real_glob, bool, t_real_glob, t_real_glob, t_real_glob)));
	QObject::connect(&m_sceneProjRecip, SIGNAL(coordsChanged(t_real_glob, t_real_glob, t_real_glob, bool, t_real_glob, t_real_glob, t_real_glob)),
		this, SLOT(RecipCoordsChanged(t_real_glob, t_real_glob, t_real_glob, bool, t_real_glob, t_real_glob, t_real_glob)));
	QObject::connect(&m_sceneRealLattice, SIGNAL(coordsChanged(t_real_glob, t_real_glob, t_real_glob, bool, t_real_glob, t_real_glob, t_real_glob)),
		this, SLOT(RealCoordsChanged(t_real_glob, t_real_glob, t_real_glob, bool, t_real_glob, t_real_glob, t_real_glob)));

	QObject::connect(&m_sceneRecip, SIGNAL(spurionInfo(const tl::ElasticSpurion&, const std::vector<tl::InelasticSpurion<t_real_glob>>&, const std::vector<tl::InelasticSpurion<t_real_glob>>&)),
		this, SLOT(spurionInfo(const tl::ElasticSpurion&, const std::vector<tl::InelasticSpurion<t_real_glob>>&, const std::vector<tl::InelasticSpurion<t_real_glob>>&)));

	QObject::connect(m_pGotoDlg, SIGNAL(vars_changed(const CrystalOptions&, const TriangleOptions&)),
		this, SLOT(VarsChanged(const CrystalOptions&, const TriangleOptions&)));
	QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
		m_pGotoDlg, SLOT(RecipParamsChanged(const RecipParams&)));

	QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
		this, SLOT(recipParamsChanged(const RecipParams&)));


	for(QLineEdit* pEdit : m_vecEdits_monoana)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(UpdateDs()));

	for(QLineEdit* pEdit : m_vecEdits_real)
	{
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CheckCrystalType()));
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcPeaks()));
	}

	for(QLineEdit* pEdit : m_vecEdits_plane)
	{
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcPeaks()));
	}

	//for(QDoubleSpinBox* pSpin : m_vecSpinBoxesSample)
	//	QObject::connect(pSpin, SIGNAL(valueChanged(t_real)), this, SLOT(CalcPeaks()));

	for(QLineEdit* pEdit : m_vecEdits_recip)
	{
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CheckCrystalType()));
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcPeaksRecip()));
	}

	QObject::connect(checkSenseM, SIGNAL(stateChanged(int)), this, SLOT(UpdateMonoSense()));
	QObject::connect(checkSenseS, SIGNAL(stateChanged(int)), this, SLOT(UpdateSampleSense()));
	QObject::connect(checkSenseA, SIGNAL(stateChanged(int)), this, SLOT(UpdateAnaSense()));

	QObject::connect(editSpaceGroupsFilter, SIGNAL(textEdited(const QString&)), this, SLOT(RepopulateSpaceGroups()));
	QObject::connect(comboSpaceGroups, SIGNAL(currentIndexChanged(int)), this, SLOT(SetCrystalType()));
	QObject::connect(comboSpaceGroups, SIGNAL(currentIndexChanged(int)), this, SLOT(CalcPeaks()));
	QObject::connect(checkPowder, SIGNAL(stateChanged(int)), this, SLOT(CalcPeaks()));

	QObject::connect(btnAtoms, SIGNAL(clicked(bool)), this, SLOT(ShowAtomsDlg()));



	// --------------------------------------------------------------------------------
	// file menu
	QMenu *pMenuFile = new QMenu("File", this);

	QAction *pNew = new QAction("New", this);
	pNew->setIcon(load_icon("res/icons/document-new.svg"));
	pMenuFile->addAction(pNew);

	pMenuFile->addSeparator();

	QAction *pLoad = new QAction("Load...", this);
	pLoad->setIcon(load_icon("res/icons/document-open.svg"));
	pMenuFile->addAction(pLoad);

	m_pMenuRecent = new QMenu("Recently Loaded", this);
	RecentFiles recent(&m_settings, "main/recent");
	m_pMapperRecent = new QSignalMapper(m_pMenuRecent);
	QObject::connect(m_pMapperRecent, SIGNAL(mapped(const QString&)),
		this, SLOT(LoadFile(const QString&)));
	recent.FillMenu(m_pMenuRecent, m_pMapperRecent);
	pMenuFile->addMenu(m_pMenuRecent);

	QAction *pSave = new QAction("Save", this);
	pSave->setIcon(load_icon("res/icons/document-save.svg"));
	pMenuFile->addAction(pSave);

	QAction *pSaveAs = new QAction("Save as...", this);
	pSaveAs->setIcon(load_icon("res/icons/document-save-as.svg"));
	pMenuFile->addAction(pSaveAs);

	pMenuFile->addSeparator();

	QAction *pImport = new QAction("Import...", this);
	pImport->setIcon(load_icon("res/icons/drive-harddisk.svg"));
	pMenuFile->addAction(pImport);

	m_pMenuRecentImport = new QMenu("Recently Imported", this);
	RecentFiles recentimport(&m_settings, "main/recent_import");
	m_pMapperRecentImport = new QSignalMapper(m_pMenuRecentImport);
	QObject::connect(m_pMapperRecentImport, SIGNAL(mapped(const QString&)),
		this, SLOT(ImportFile(const QString&)));
	recentimport.FillMenu(m_pMenuRecentImport, m_pMapperRecentImport);
	pMenuFile->addMenu(m_pMenuRecentImport);

	pMenuFile->addSeparator();

	QAction *pSettings = new QAction("Settings...", this);
	pSettings->setMenuRole(QAction::PreferencesRole);
	pSettings->setIcon(load_icon("res/icons/preferences-system.svg"));
	pMenuFile->addAction(pSettings);

	pMenuFile->addSeparator();

	QAction *pExit = new QAction("Exit", this);
	pExit->setMenuRole(QAction::QuitRole);
	pExit->setIcon(load_icon("res/icons/system-log-out.svg"));
	pMenuFile->addAction(pExit);


	// --------------------------------------------------------------------------------
	// recip menu
	m_pMenuViewRecip = new QMenu("Reciprocal Space", this);

	m_pGoto = new QAction("Go to Position...", this);
	m_pGoto->setIcon(load_icon("res/icons/goto.svg"));
	m_pMenuViewRecip->addAction(m_pGoto);

	QAction *pRecipParams = new QAction("Information...", this);
	m_pMenuViewRecip->addAction(pRecipParams);
	m_pMenuViewRecip->addSeparator();

	m_pCoordAxes = new QAction("Show Coordinate Axes", this);
	m_pCoordAxes->setCheckable(1);
	m_pCoordAxes->setChecked(bCoordAxesVisible);
	m_pMenuViewRecip->addAction(m_pCoordAxes);

	m_pSmallq = new QAction("Show Reduced Scattering Vector q", this);
	m_pSmallq->setIcon(load_icon("res/icons/q.svg"));
	m_pSmallq->setCheckable(1);
	m_pSmallq->setChecked(bSmallqVisible);
	m_pMenuViewRecip->addAction(m_pSmallq);

	m_pSnapSmallq = new QAction("Snap G to Bragg Peak", this);
	m_pSnapSmallq->setCheckable(1);
	m_pSnapSmallq->setChecked(m_sceneRecip.getSnapq());
	m_pMenuViewRecip->addAction(m_pSnapSmallq);

	QAction *pKeepAbsKiKf = new QAction("Keep |ki| and |kf| Fixed", this);
	pKeepAbsKiKf->setCheckable(1);
	pKeepAbsKiKf->setChecked(m_sceneRecip.getKeepAbsKiKf());
	m_pMenuViewRecip->addAction(pKeepAbsKiKf);

	m_pBZ = new QAction("Show First Brillouin Zone", this);
	m_pBZ->setIcon(load_icon("res/icons/brillouin.svg"));
	m_pBZ->setCheckable(1);
	m_pBZ->setChecked(bBZVisible);
	m_pMenuViewRecip->addAction(m_pBZ);


	QMenu *pMenuEwald = new QMenu("Ewald Sphere", this);
	QActionGroup *pGroupEwald = new QActionGroup(this);
	m_pEwaldSphereNone = new QAction("Disabled", this);
	m_pEwaldSphereKi = new QAction("Around ki", this);
	m_pEwaldSphereKf = new QAction("Around kf", this);
	for(QAction* pAct : {m_pEwaldSphereNone, m_pEwaldSphereKi, m_pEwaldSphereKf})
	{
		pAct->setCheckable(1);
		pGroupEwald->addAction(pAct);
	}

	m_pEwaldSphereKf->setChecked(1);
	pMenuEwald->addActions(pGroupEwald->actions());
	m_pMenuViewRecip->addMenu(pMenuEwald);


	m_pMenuViewRecip->addSeparator();

	QMenu *pMenuProj = new QMenu("Projection", this);
	pMenuProj->setTearOffEnabled(1);
	QActionGroup *pGroupProj = new QActionGroup(this);

	m_pProjGnom = new QAction("Gnomonic", this);
	m_pProjStereo = new QAction("Stereographic", this);
	m_pProjPara = new QAction("Parallel", this);
	m_pProjPersp = new QAction("Perspectivic", this);

	for(QAction *pAct : {m_pProjGnom, m_pProjStereo, m_pProjPara, m_pProjPersp})
	{
		pAct->setCheckable(1);
		pGroupProj->addAction(pAct);
	}

	m_pProjStereo->setChecked(1);
	pMenuProj->addActions(pGroupProj->actions());

	m_pMenuViewRecip->addMenu(pMenuProj);
	m_pMenuViewRecip->addSeparator();

#if !defined NO_3D
	QAction *pView3DBZ = new QAction("3D Brillouin Zone...", this);
	pView3DBZ->setIcon(load_icon("res/icons/brillouin3d.svg"));
	m_pMenuViewRecip->addAction(pView3DBZ);

	QAction *pView3D = new QAction("3D Bragg Peaks...", this);
	//pView3D->setIcon(QIcon::fromTheme("applications-graphics"));
	m_pMenuViewRecip->addAction(pView3D);

	m_pMenuViewRecip->addSeparator();
#endif


	QAction *pRecipExport = new QAction("Export Lattice Graphics...", this);
	pRecipExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewRecip->addAction(pRecipExport);

	QAction *pProjExport = new QAction("Export Projection Graphics...", this);
	pProjExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewRecip->addAction(pProjExport);

#ifdef USE_GIL
	QAction *pBZExport = new QAction("Export Brillouin Zone Image...", this);
	pBZExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewRecip->addAction(pBZExport);
#endif

	QAction *pBZ3DExport = new QAction("Export 3D Brillouin Zone Model...", this);
	pBZ3DExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewRecip->addAction(pBZ3DExport);



	// --------------------------------------------------------------------------------
	// real menu
	m_pMenuViewReal = new QMenu("Real Space", this);
	m_pMenuViewReal->addAction(m_pGoto);

	QAction *pRealParams = new QAction("Information...", this);
	m_pMenuViewReal->addAction(pRealParams);

	m_pMenuViewReal->addSeparator();

	m_pShowRealQDir = new QAction("Show Q Direction", this);
	m_pShowRealQDir->setCheckable(1);
	m_pShowRealQDir->setChecked(m_sceneReal.GetTasLayout()->GetRealQVisible());
	m_pMenuViewReal->addAction(m_pShowRealQDir);

	m_pWS = new QAction("Show Unit Cell", this);
	m_pWS->setIcon(load_icon("res/icons/brillouin.svg"));
	m_pWS->setCheckable(1);
	m_pWS->setChecked(bWSVisible);
	m_pMenuViewReal->addAction(m_pWS);

	m_pMenuViewReal->addSeparator();

	QAction *pDeadAngles = new QAction("Dead Angles...", this);
	m_pMenuViewReal->addAction(pDeadAngles);

	m_pMenuViewReal->addSeparator();

#if !defined NO_3D
	QAction *pView3DReal = new QAction("3D Unit Cell...", this);
	pView3DReal->setIcon(load_icon("res/icons/unitcell3d.svg"));
	m_pMenuViewReal->addAction(pView3DReal);

	m_pMenuViewReal->addSeparator();
#endif

	QAction *pRealLatticeExport = new QAction("Export Lattice Graphics...", this);
	pRealLatticeExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewReal->addAction(pRealLatticeExport);

	QAction *pRealExport = new QAction("Export TAS Layout...", this);
	pRealExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewReal->addAction(pRealExport);

	QAction *pTofExport = new QAction("Export TOF Layout...", this);
	pTofExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewReal->addAction(pTofExport);

#ifdef USE_GIL
	QAction *pWSExport = new QAction("Export Unit Cell Image...", this);
	pWSExport->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewReal->addAction(pWSExport);
#endif

	QAction *pExportUC = new QAction("Export 3D Unit Cell Model...", this);
	pExportUC->setIcon(load_icon("res/icons/image-x-generic.svg"));
	m_pMenuViewReal->addAction(pExportUC);


	// --------------------------------------------------------------------------------
	// resolution menu
	QMenu *pMenuReso = new QMenu("Resolution", this);

	QAction *pResoParams = new QAction("Parameters...", this);
	pResoParams->setIcon(load_icon("res/icons/accessories-calculator.svg"));
	pMenuReso->addAction(pResoParams);

	pMenuReso->addSeparator();

	QAction *pResoEllipses = new QAction("Ellipses...", this);
	pResoEllipses->setIcon(load_icon("res/icons/ellipses.svg"));
	pMenuReso->addAction(pResoEllipses);

#if !defined NO_3D
	QAction *pResoEllipses3D = new QAction("3D Ellipsoids...", this);
	pMenuReso->addAction(pResoEllipses3D);
#endif

	pMenuReso->addSeparator();

	QAction *pResoConv = new QAction("Convolution...", this);
	pMenuReso->addAction(pResoConv);


	// --------------------------------------------------------------------------------
	// calc menu
	QMenu *pMenuCalc = new QMenu("Calculation", this);

	QAction *pNeutronProps = new QAction("Neutron Properties...", this);
	pNeutronProps->setIcon(load_icon("res/icons/x-office-spreadsheet-template.svg"));
	pMenuCalc->addAction(pNeutronProps);

	QAction *pCompProps = new QAction("Components...", this);
	//pCompProps->setIcon(load_icon("res/icons/x-office-spreadsheet-template.svg"));
	pMenuCalc->addAction(pCompProps);

	pMenuCalc->addSeparator();

	QAction *pSgList = new QAction("Space Groups...", this);
	pMenuCalc->addAction(pSgList);

	QAction *pFormfactor = nullptr;
	if(g_bHasFormfacts && g_bHasScatlens)
	{
		pFormfactor = new QAction("Elements && Form Factors...", this);
		pMenuCalc->addAction(pFormfactor);
	}

	QAction *pDW = new QAction("Scattering Factors...", this);
	pMenuCalc->addAction(pDW);

	pMenuCalc->addSeparator();

	QAction *pPowder = new QAction("Powder Lines...", this);
	pPowder->setIcon(load_icon("res/icons/weather-snow.svg"));
	pMenuCalc->addAction(pPowder);

	QAction *pDisp = new QAction("Dispersions...", this);
	//pDisp->setIcon(load_icon("disp.svg"));
	pMenuCalc->addAction(pDisp);

	pMenuCalc->addSeparator();

	QAction *pDynPlane = new QAction("Kinematic Plane...", this);
	pMenuCalc->addAction(pDynPlane);

	QAction *pSpuri = new QAction("Spurious Scattering...", this);
	pMenuCalc->addAction(pSpuri);


#if !defined NO_NET
	// --------------------------------------------------------------------------------
	// network menu
	QMenu *pMenuNet = new QMenu("Network", this);

	QAction *pConn = new QAction("Connect to Instrument...", this);
	pConn->setIcon(load_icon("res/icons/network-transmit-receive.svg"));
	pMenuNet->addAction(pConn);

	QAction *pDisconn = new QAction("Disconnect", this);
	pDisconn->setIcon(load_icon("res/icons/network-offline.svg"));
	pMenuNet->addAction(pDisconn);

	pMenuNet->addSeparator();

	QAction *pNetScanMon = new QAction("Scan Monitor...", this);
	pMenuNet->addAction(pNetScanMon);

	QAction *pNetCache = new QAction("Network Cache...", this);
	pMenuNet->addAction(pNetCache);

	QAction *pNetRefresh = new QAction("Refresh", this);
	pNetRefresh->setIcon(load_icon("res/icons/view-refresh.svg"));
	pMenuNet->addSeparator();
	pMenuNet->addAction(pNetRefresh);
#endif


	// --------------------------------------------------------------------------------
	// tools menu
	QMenu *pMenuTools = new QMenu("Tools", this);

	QAction *pScanViewer = new QAction("Scan Viewer...", this);
	pMenuTools->addAction(pScanViewer);

	QAction *pScanPos = new QAction("Scan Positions Plot...", this);
	pMenuTools->addAction(pScanPos);

	pMenuTools->addSeparator();

	QAction *pPowderFit = new QAction("Powder Fitting...", this);
	pMenuTools->addAction(pPowderFit);



	// --------------------------------------------------------------------------------
	// help menu
	QMenu *pMenuHelp = new QMenu("Help", this);

	QAction *pHelp = new QAction("Show Help...", this);
	pHelp->setIcon(load_icon("res/icons/help-browser.svg"));
	pMenuHelp->addAction(pHelp);

	QAction *pDevelDoc = new QAction("Show Developer Help...", this);
	if(find_resource("doc/devel/html/index.html", 0) == "")
		pDevelDoc->setEnabled(0);
	pDevelDoc->setIcon(load_icon("res/icons/help-browser.svg"));
	pMenuHelp->addAction(pDevelDoc);

	pMenuHelp->addSeparator();

	QAction *pLog = new QAction("Log...", this);
	pMenuHelp->addAction(pLog);

	pMenuHelp->addSeparator();

	QAction *pAboutQt = new QAction("About Qt...", this);
	pAboutQt->setMenuRole(QAction::AboutQtRole);
	//pAboutQt->setIcon(QIcon::fromTheme("help-about"));
	pMenuHelp->addAction(pAboutQt);

	//pMenuHelp->addSeparator();
	QAction *pAbout = new QAction("About Takin...", this);
	pAbout->setMenuRole(QAction::AboutRole);
	pAbout->setIcon(load_icon("res/icons/dialog-information.svg"));
	pMenuHelp->addAction(pAbout);



	// --------------------------------------------------------------------------------
	QMenuBar *pMenuBar = new QMenuBar(this);
	pMenuBar->setNativeMenuBar(m_settings.value("main/native_dialogs", 1).toBool());

	pMenuBar->addMenu(pMenuFile);
	pMenuBar->addMenu(m_pMenuViewRecip);
	pMenuBar->addMenu(m_pMenuViewReal);
	pMenuBar->addMenu(pMenuReso);
	pMenuBar->addMenu(pMenuCalc);
	pMenuBar->addMenu(pMenuTools);
#if !defined NO_NET
	pMenuBar->addMenu(pMenuNet);
#endif
	pMenuBar->addMenu(pMenuHelp);


	QObject::connect(pNew, SIGNAL(triggered()), this, SLOT(New()));
	QObject::connect(pLoad, SIGNAL(triggered()), this, SLOT(Load()));
	QObject::connect(pSave, SIGNAL(triggered()), this, SLOT(Save()));
	QObject::connect(pSaveAs, SIGNAL(triggered()), this, SLOT(SaveAs()));
	QObject::connect(pImport, SIGNAL(triggered()), this, SLOT(Import()));
	QObject::connect(pScanViewer, SIGNAL(triggered()), this, SLOT(ShowScanViewer()));
	QObject::connect(pScanPos, SIGNAL(triggered()), this, SLOT(ShowScanPos()));
	QObject::connect(pPowderFit, SIGNAL(triggered()), this, SLOT(ShowPowderFit()));
	QObject::connect(pSettings, SIGNAL(triggered()), this, SLOT(ShowSettingsDlg()));
	QObject::connect(pExit, SIGNAL(triggered()), this, SLOT(close()));

	QObject::connect(m_pSmallq, SIGNAL(toggled(bool)), this, SLOT(EnableSmallq(bool)));
	QObject::connect(m_pCoordAxes, SIGNAL(toggled(bool)), this, SLOT(EnableCoordAxes(bool)));
	QObject::connect(m_pBZ, SIGNAL(toggled(bool)), this, SLOT(EnableBZ(bool)));
	QObject::connect(m_pWS, SIGNAL(toggled(bool)), this, SLOT(EnableWS(bool)));

	for(QAction* pAct : {m_pEwaldSphereNone, m_pEwaldSphereKi, m_pEwaldSphereKf})
		QObject::connect(pAct, SIGNAL(triggered()), this, SLOT(ShowEwaldSphere()));

	QObject::connect(m_pShowRealQDir, SIGNAL(toggled(bool)), this, SLOT(EnableRealQDir(bool)));

	QObject::connect(m_pSnapSmallq, SIGNAL(toggled(bool)), &m_sceneRecip, SLOT(setSnapq(bool)));
	QObject::connect(pKeepAbsKiKf, SIGNAL(toggled(bool)), &m_sceneRecip, SLOT(setKeepAbsKiKf(bool)));

	QObject::connect(pRecipParams, SIGNAL(triggered()), this, SLOT(ShowRecipParams()));
	QObject::connect(pRealParams, SIGNAL(triggered()), this, SLOT(ShowRealParams()));

	for(QAction *pProj : {m_pProjGnom, m_pProjStereo, m_pProjPara, m_pProjPersp})
		QObject::connect(pProj, SIGNAL(triggered()), this, SLOT(RecipProjChanged()));

#if !defined NO_3D
	QObject::connect(pView3D, SIGNAL(triggered()), this, SLOT(Show3D()));
	QObject::connect(pView3DReal, SIGNAL(triggered()), this, SLOT(Show3DReal()));
	QObject::connect(pView3DBZ, SIGNAL(triggered()), this, SLOT(Show3DBZ()));
	QObject::connect(pResoEllipses3D, SIGNAL(triggered()), this, SLOT(ShowResoEllipses3D()));
#endif

	QObject::connect(pRecipExport, SIGNAL(triggered()), this, SLOT(ExportRecip()));
	QObject::connect(pBZ3DExport, SIGNAL(triggered()), this, SLOT(ExportBZ3DModel()));
	QObject::connect(pProjExport, SIGNAL(triggered()), this, SLOT(ExportProj()));
	QObject::connect(pRealExport, SIGNAL(triggered()), this, SLOT(ExportReal()));
	QObject::connect(pTofExport, SIGNAL(triggered()), this, SLOT(ExportTof()));
	QObject::connect(pRealLatticeExport, SIGNAL(triggered()), this, SLOT(ExportRealLattice()));

	QObject::connect(pExportUC, SIGNAL(triggered()), this, SLOT(ExportUCModel()));

#ifdef USE_GIL
	QObject::connect(pBZExport, SIGNAL(triggered()), this, SLOT(ExportBZImage()));
	QObject::connect(pWSExport, SIGNAL(triggered()), this, SLOT(ExportWSImage()));
#endif

	QObject::connect(pResoParams, SIGNAL(triggered()), this, SLOT(ShowResoParams()));
	QObject::connect(pResoEllipses, SIGNAL(triggered()), this, SLOT(ShowResoEllipses()));
	QObject::connect(pResoConv, SIGNAL(triggered()), this, SLOT(ShowResoConv()));

	QObject::connect(pNeutronProps, SIGNAL(triggered()), this, SLOT(ShowNeutronDlg()));
	QObject::connect(pCompProps, SIGNAL(triggered()), this, SLOT(ShowTofDlg()));
	QObject::connect(m_pGoto, SIGNAL(triggered()), this, SLOT(ShowGotoDlg()));
	QObject::connect(pPowder, SIGNAL(triggered()), this, SLOT(ShowPowderDlg()));
	QObject::connect(pDisp, SIGNAL(triggered()), this, SLOT(ShowDispDlg()));
	QObject::connect(pSpuri, SIGNAL(triggered()), this, SLOT(ShowSpurions()));
	QObject::connect(pDW, SIGNAL(triggered()), this, SLOT(ShowDWDlg()));
	QObject::connect(pDynPlane, SIGNAL(triggered()), this, SLOT(ShowDynPlaneDlg()));
	QObject::connect(pDeadAngles, SIGNAL(triggered()), this, SLOT(ShowDeadAnglesDlg()));

#if !defined NO_NET
	QObject::connect(pConn, SIGNAL(triggered()), this, SLOT(ShowConnectDlg()));
	QObject::connect(pDisconn, SIGNAL(triggered()), this, SLOT(Disconnect()));
	QObject::connect(pNetRefresh, SIGNAL(triggered()), this, SLOT(NetRefresh()));
	QObject::connect(pNetCache, SIGNAL(triggered()), this, SLOT(ShowNetCache()));
	QObject::connect(pNetScanMon, SIGNAL(triggered()), this, SLOT(ShowNetScanMonitor()));
#endif

	QObject::connect(pSgList, SIGNAL(triggered()), this, SLOT(ShowSgListDlg()));

	if(pFormfactor)
		QObject::connect(pFormfactor, SIGNAL(triggered()), this, SLOT(ShowFormfactorDlg()));

	QObject::connect(pHelp, SIGNAL(triggered()), this, SLOT(ShowHelp()));
	QObject::connect(pDevelDoc, SIGNAL(triggered()), this, SLOT(ShowDevelDoc()));
	QObject::connect(pLog, SIGNAL(triggered()), this, SLOT(ShowLog()));
	QObject::connect(pAbout, SIGNAL(triggered()), this, SLOT(ShowAbout()));
	QObject::connect(pAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));


	setMenuBar(pMenuBar);
	// --------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------
	// context menus
	if(m_pviewRecip) m_pviewRecip->setContextMenuPolicy(Qt::CustomContextMenu);
	if(m_pviewProjRecip) m_pviewProjRecip->setContextMenuPolicy(Qt::CustomContextMenu);
	if(m_pviewRealLattice) m_pviewRealLattice->setContextMenuPolicy(Qt::CustomContextMenu);
	if(m_pviewReal) m_pviewReal->setContextMenuPolicy(Qt::CustomContextMenu);
	if(m_pviewTof) m_pviewTof->setContextMenuPolicy(Qt::CustomContextMenu);

	if(m_pviewRecip)
		QObject::connect(m_pviewRecip, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(RecipContextMenu(const QPoint&)));
	if(m_pviewProjRecip)
		QObject::connect(m_pviewProjRecip, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(RecipContextMenu(const QPoint&)));
	if(m_pviewRealLattice)
		QObject::connect(m_pviewRealLattice, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(RealContextMenu(const QPoint&)));
	if(m_pviewReal)
		QObject::connect(m_pviewReal, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(RealContextMenu(const QPoint&)));
	if(m_pviewTof)
		QObject::connect(m_pviewTof, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(RealContextMenu(const QPoint&)));


	// --------------------------------------------------------------------------------
	// tool bars
	// file
	QToolBar *pFileTools = new QToolBar(this);
	pFileTools->setWindowTitle("File");
	pFileTools->addAction(pNew);
	pFileTools->addAction(pLoad);
	pFileTools->addAction(pImport);
	pFileTools->addAction(pSave);
	pFileTools->addAction(pSaveAs);
	addToolBar(pFileTools);

	// recip
	QToolBar *pRecipTools = new QToolBar(this);
	pRecipTools->setWindowTitle("Reciprocal Space");
	pRecipTools->addAction(m_pGoto);
	pRecipTools->addAction(m_pSmallq);
	pRecipTools->addAction(m_pBZ);
	//pRecipTools->addAction(m_pEwaldSphere);
	addToolBar(pRecipTools);

	// reso
	QToolBar *pResoTools = new QToolBar(this);
	pResoTools->setWindowTitle("Resolution");
	pResoTools->addAction(pResoParams);
	pResoTools->addAction(pResoEllipses);
	addToolBar(pResoTools);

	// calc
	QToolBar *pCalcTools = new QToolBar(this);
	pCalcTools->setWindowTitle("Calculation");
	pCalcTools->addAction(pNeutronProps);
	pCalcTools->addAction(pPowder);
	addToolBar(pCalcTools);

#if !defined NO_3D
	// 3d tools
	QToolBar *p3dTools = new QToolBar(this);
	p3dTools->setWindowTitle("3D Tools");
	p3dTools->addAction(pView3DBZ);
	p3dTools->addAction(pView3DReal);
	addToolBar(p3dTools);
#endif

#if !defined NO_NET
	// net
	QToolBar *pNetTools = new QToolBar(this);
	pNetTools->setWindowTitle("Network");
	pNetTools->addAction(pConn);
	pNetTools->addAction(pDisconn);
	pNetTools->addAction(pNetRefresh);
	addToolBar(pNetTools);
#endif

	// --------------------------------------------------------------------------------


	RepopulateSpaceGroups();

	unsigned int iMaxPeaks = m_settings.value("main/max_peaks", 10).toUInt();

	m_sceneRecip.GetTriangle()->SetMaxPeaks(iMaxPeaks);
	m_sceneRecip.GetTriangle()->SetPlaneDistTolerance(s_dPlaneDistTolerance);

	m_sceneProjRecip.GetLattice()->SetMaxPeaks(iMaxPeaks/2);

	m_sceneRealLattice.GetLattice()->SetMaxPeaks(iMaxPeaks);
	m_sceneRealLattice.GetLattice()->SetPlaneDistTolerance(s_dPlaneDistTolerance);

#if !defined NO_3D
	if(m_pRecip3d)
	{
		m_pRecip3d->SetMaxPeaks(t_real(iMaxPeaks/2));
		m_pRecip3d->SetPlaneDistTolerance(s_dPlaneDistTolerance);
	}
#endif

	m_bReady = 1;
	UpdateDs();
	CalcPeaks();


	m_sceneRecip.GetTriangle()->SetqVisible(bSmallqVisible);
	m_sceneRecip.GetTriangle()->SetCoordAxesVisible(bCoordAxesVisible);
	m_sceneRecip.GetTriangle()->SetBZVisible(bBZVisible);
	m_sceneRecip.GetTriangle()->SetEwaldSphereVisible(EWALD_KF);
	m_sceneRealLattice.GetLattice()->SetWSVisible(bWSVisible);
	m_sceneRecip.emitUpdate();
	//m_sceneRecip.emitAllParams();

	setAcceptDrops(1);
}

TazDlg::~TazDlg()
{
	//log_debug("In ", __func__, ".");
	Disconnect();
	DeleteDialogs();

	// don't delete non-optional sub-modules in DeleteDialogs()
	if(m_pGotoDlg) { delete m_pGotoDlg; m_pGotoDlg = 0; }
	if(m_pSettingsDlg) { delete m_pSettingsDlg; m_pSettingsDlg = 0; }

	if(m_pviewRecip) { delete m_pviewRecip; m_pviewRecip = 0; }
	if(m_pviewProjRecip) { delete m_pviewProjRecip; m_pviewProjRecip = 0; }
	if(m_pviewRealLattice) { delete m_pviewRealLattice; m_pviewRealLattice = 0; }
	if(m_pviewReal) { delete m_pviewReal; m_pviewReal = 0; }
	if(m_pviewTof) { delete m_pviewTof; m_pviewTof = 0; }

	comboSpaceGroups->clear();
}

void TazDlg::DeleteDialogs()
{
	if(m_pAboutDlg) { delete m_pAboutDlg; m_pAboutDlg = 0; }
	if(m_pLogDlg) { delete m_pLogDlg; m_pLogDlg = 0; }
	if(m_pEllipseDlg) { delete m_pEllipseDlg; m_pEllipseDlg = 0; }
	if(m_pReso) { delete m_pReso; m_pReso = 0; }
	if(m_pConvoDlg) { delete m_pConvoDlg; m_pConvoDlg = 0; }
	if(m_pSpuri) { delete m_pSpuri; m_pSpuri = 0; }
	if(m_pNeutronDlg) { delete m_pNeutronDlg; m_pNeutronDlg = 0; }
	if(m_pTofDlg) { delete m_pTofDlg; m_pTofDlg = 0; }
	if(m_pDWDlg) { delete m_pDWDlg; m_pDWDlg = 0; }
	if(m_pDynPlaneDlg) { delete m_pDynPlaneDlg; m_pDynPlaneDlg = 0; }
	if(m_pScanViewer) { delete m_pScanViewer; m_pScanViewer = nullptr; }
	if(m_pScanPos) { delete m_pScanPos; m_pScanPos = nullptr; }
	if(m_pPowderFit) { delete m_pPowderFit; m_pPowderFit = nullptr; }
	if(m_pAtomsDlg) { delete m_pAtomsDlg; m_pAtomsDlg = nullptr; }
	if(m_pDeadAnglesDlg) { delete m_pDeadAnglesDlg; m_pDeadAnglesDlg = nullptr; }
	if(m_pPowderDlg) { delete m_pPowderDlg; m_pPowderDlg = 0; }
	if(m_pDispDlg) { delete m_pDispDlg; m_pDispDlg = 0; }

#if !defined NO_3D
	if(m_pRecip3d) { delete m_pRecip3d; m_pRecip3d = 0; }
	if(m_pReal3d) { delete m_pReal3d; m_pReal3d = 0; }
	if(m_pBZ3d) { delete m_pBZ3d; m_pBZ3d = 0; }
	if(m_pEllipseDlg3D) { delete m_pEllipseDlg3D; m_pEllipseDlg3D = 0; }
#endif

#if !defined NO_NET
	if(m_pSrvDlg) { delete m_pSrvDlg; m_pSrvDlg = 0; }
	if(m_pScanMonDlg) { delete m_pScanMonDlg; m_pScanMonDlg = 0; }
	if(m_pNetCacheDlg) { delete m_pNetCacheDlg; m_pNetCacheDlg = 0; }
	if(m_pNetCache) { delete m_pNetCache; m_pNetCache = 0; }
#endif

	if(m_pSgListDlg) { delete m_pSgListDlg; m_pSgListDlg = 0; }
	if(m_pFormfactorDlg) { delete m_pFormfactorDlg; m_pFormfactorDlg = 0; }
}

void TazDlg::SettingsChanged()
{
	setFont(g_fontGen);

	m_sceneReal.update();
	m_sceneTof.update();
	m_sceneRealLattice.update();
	m_sceneRecip.update();
}


void TazDlg::keyPressEvent(QKeyEvent *pEvt)
{
	// x rotation
	if(pEvt->key() == Qt::Key_8)
		RotatePlane(0, tl::d2r<t_real>(-5.));
	else if(pEvt->key() == Qt::Key_2)
		RotatePlane(0, tl::d2r<t_real>(5.));

	// y rotation
	else if(pEvt->key() == Qt::Key_4)
		RotatePlane(1, tl::d2r<t_real>(-5.));
	else if(pEvt->key() == Qt::Key_6)
		RotatePlane(1, tl::d2r<t_real>(5.));

	// z rotation
	else if(pEvt->key() == Qt::Key_9)
		RotatePlane(2, tl::d2r<t_real>(-5.));
	else if(pEvt->key() == Qt::Key_7)
		RotatePlane(2, tl::d2r<t_real>(5.));

	QMainWindow::keyPressEvent(pEvt);
}

void TazDlg::keyReleaseEvent(QKeyEvent *pEvt)
{
	QMainWindow::keyReleaseEvent(pEvt);
}


void TazDlg::showEvent(QShowEvent *pEvt)
{
	QMainWindow::showEvent(pEvt);

	static bool bInitialShow = 1;
	if(bInitialShow)
	{
		bInitialShow = 0;

		if(m_pviewRecip) m_pviewRecip->centerOn(m_sceneRecip.GetTriangle()->GetGfxMid());
		if(m_pviewProjRecip) m_pviewProjRecip->centerOn(0.,0.);
		if(m_pviewReal) m_pviewReal->centerOn(m_sceneReal.GetTasLayout());
		if(m_pviewTof) m_pviewTof->centerOn(m_sceneTof.GetTofLayout());
		if(m_pviewRealLattice) m_pviewRealLattice->centerOn(0.,0.);

		/*for(QScrollBar* pSB : {
			m_pviewRealLattice->horizontalScrollBar(),
			m_pviewRealLattice->verticalScrollBar(),
			m_pviewReal->horizontalScrollBar(),
			m_pviewReal->verticalScrollBar(),
			m_pviewRecip->horizontalScrollBar(),
			m_pviewRecip->verticalScrollBar() })
			pSB->setValue(pSB->minimum() + (pSB->maximum()-pSB->minimum())/2);*/
	}
}

void TazDlg::dragEnterEvent(QDragEnterEvent *pEvt)
{
	if(pEvt) pEvt->accept();
}

void TazDlg::dropEvent(QDropEvent *pEvt)
{
	if(!pEvt) return;
	const QMimeData* pMime = pEvt->mimeData();
	if(!pMime) return;

	std::string strFiles = pMime->text().toStdString();
	std::vector<std::string> vecFiles;
	tl::get_tokens<std::string, std::string>(strFiles, "\n", vecFiles);
	if(vecFiles.size() > 1)
		tl::log_warn("More than one file dropped, using first one.");

	if(vecFiles.size() >= 1)
	{
		std::string& strFile = vecFiles[0];
		tl::trim(strFile);

		const std::string strHead = "file://";
		if(algo::starts_with(strFile, strHead))
			algo::replace_head(strFile, strHead.length(), "");
		//tl::log_debug("dropped: ", strFile);

		Load(strFile.c_str());
	}
}

void TazDlg::closeEvent(QCloseEvent* pEvt)
{
	m_bReady = 0;

	//m_settings.setValue("main/width", this->width());
	//m_settings.setValue("main/height", this->height());
	m_settings.setValue("main/geo", saveGeometry());
	m_settings.setValue("main/recip_tab", groupRecip->currentIndex());
	m_settings.setValue("main/real_tab", groupReal->currentIndex());

	QMainWindow::closeEvent(pEvt);
}


void TazDlg::ShowNeutronDlg()
{
	if(!m_pNeutronDlg)
	{
		m_pNeutronDlg = new NeutronDlg(this, &m_settings);
		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pNeutronDlg, SLOT(paramsChanged(const RecipParams&)));
		m_sceneRecip.emitAllParams();
	}

	focus_dlg(m_pNeutronDlg);
}

void TazDlg::ShowTofDlg()
{
	if(!m_pTofDlg)
		m_pTofDlg = new TOFDlg(this, &m_settings);

	focus_dlg(m_pTofDlg);
}


void TazDlg::InitGoto()
{
	if(!m_pGotoDlg)
		m_pGotoDlg = new GotoDlg(this, &m_settings);
}

void TazDlg::ShowGotoDlg()
{
	InitGoto();
	focus_dlg(m_pGotoDlg);
}

void TazDlg::ShowPowderDlg()
{
	if(!m_pPowderDlg)
	{
		m_pPowderDlg = new PowderDlg(this, &m_settings);
		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pPowderDlg, SLOT(paramsChanged(const RecipParams&)));
		m_sceneRecip.emitAllParams();
	}

	focus_dlg(m_pPowderDlg);
}

void TazDlg::ShowDispDlg()
{
	if(!m_pDispDlg)
		m_pDispDlg = new DispDlg(this, &m_settings);

	focus_dlg(m_pDispDlg);
}

void TazDlg::ShowSettingsDlg()
{
	if(!m_pSettingsDlg)
		m_pSettingsDlg = new SettingsDlg(this, &m_settings);

	focus_dlg(m_pSettingsDlg);
}

void TazDlg::ShowDWDlg()
{
	if(!m_pDWDlg)
		m_pDWDlg = new DWDlg(this, &m_settings);

	focus_dlg(m_pDWDlg);
}

void TazDlg::ShowDynPlaneDlg()
{
	if(!m_pDynPlaneDlg)
	{
		m_pDynPlaneDlg = new DynPlaneDlg(this, &m_settings);
		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pDynPlaneDlg, SLOT(RecipParamsChanged(const RecipParams&)));
		m_sceneRecip.emitAllParams();
	}

	focus_dlg(m_pDynPlaneDlg);
}

void TazDlg::UpdateDs()
{
	t_real dMonoD = editMonoD->text().toDouble();
	t_real dAnaD = editAnaD->text().toDouble();

	m_sceneRecip.SetDs(dMonoD, dAnaD);

	ResoParams resoparams;
	resoparams.bMonoDChanged = 1;
	resoparams.bAnaDChanged = 1;
	resoparams.dMonoD = dMonoD;
	resoparams.dAnaD = dAnaD;

	if(m_pGotoDlg)
	{
		m_pGotoDlg->SetD(editMonoD->text().toDouble(), editAnaD->text().toDouble());
		m_pGotoDlg->CalcMonoAna();
		m_pGotoDlg->CalcSample();
	}

	emit ResoParamsChanged(resoparams);
}

void TazDlg::UpdateSampleSense()
{
	const bool bSense = checkSenseS->isChecked();
	m_sceneRecip.SetSampleSense(bSense);

	if(m_pGotoDlg)
	{
		m_pGotoDlg->SetSampleSense(bSense);
		m_pGotoDlg->CalcSample();
	}

	ResoParams params;
	params.bSensesChanged[1] = 1;
	params.bScatterSenses[1] = bSense;
	emit ResoParamsChanged(params);

	m_sceneRecip.emitUpdate();
}

void TazDlg::UpdateMonoSense()
{
	const bool bSense = checkSenseM->isChecked();
	m_sceneRecip.SetMonoSense(bSense);

	if(m_pGotoDlg)
	{
		m_pGotoDlg->SetMonoSense(bSense);
		m_pGotoDlg->CalcMonoAna();
	}

	ResoParams params;
	params.bSensesChanged[0] = 1;
	params.bScatterSenses[0] = bSense;
	emit ResoParamsChanged(params);
}

void TazDlg::UpdateAnaSense()
{
	const bool bSense = checkSenseA->isChecked();
	m_sceneRecip.SetAnaSense(bSense);

	if(m_pGotoDlg)
	{
		m_pGotoDlg->SetAnaSense(bSense);
		m_pGotoDlg->CalcMonoAna();
	}

	ResoParams params;
	params.bSensesChanged[2] = 1;
	params.bScatterSenses[2] = bSense;
	emit ResoParamsChanged(params);
}


void TazDlg::RecipNodeEvent(bool bStarted)
{
	//tl::log_info("recip node movement: ", bStarted);
	// optimises reso dialog update policy
	if(m_pReso)
			m_pReso->SetUpdateOn(!bStarted, 1);
}

void TazDlg::RealNodeEvent(bool bStarted)
{
	//tl::log_info("real node movement: ", bStarted);
	// optimises reso dialog update policy
	if(m_pReso)
		m_pReso->SetUpdateOn(1, !bStarted);
}

void TazDlg::TofNodeEvent(bool bStarted)
{
	//tl::log_info("tof node movement: ", bStarted);
	// optimises reso dialog update policy
	if(m_pReso)
		m_pReso->SetUpdateOn(1, !bStarted);
}


void TazDlg::RecipProjChanged()
{
	LatticeProj proj = LatticeProj::PARALLEL;
	if(m_pProjGnom->isChecked())
		proj = LatticeProj::GNOMONIC;
	else if(m_pProjStereo->isChecked())
		proj = LatticeProj::STEREOGRAPHIC;
	else if(m_pProjPara->isChecked())
		proj = LatticeProj::PARALLEL;
	else if(m_pProjPersp->isChecked())
		proj = LatticeProj::PERSPECTIVE;

	if(m_sceneProjRecip.GetLattice())
	{
		m_sceneProjRecip.GetLattice()->SetProjection(proj);
		m_sceneProjRecip.GetLattice()->CalcPeaks(m_latticecommon, true);
	}
}


// 3d stuff
#if !defined NO_3D

void TazDlg::Show3D()
{
	if(!m_pRecip3d)
	{
		m_pRecip3d = new Recip3DDlg(this, &m_settings);

		t_real dTol = s_dPlaneDistTolerance;
		m_pRecip3d->SetPlaneDistTolerance(dTol);

		// also track current Q position
		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pRecip3d, SLOT(RecipParamsChanged(const RecipParams&)));
		m_sceneRecip.emitAllParams();
	}

	if(!m_pRecip3d->isVisible())
		m_pRecip3d->show();
	m_pRecip3d->activateWindow();

	m_pRecip3d->CalcPeaks(m_latticecommon);
	//CalcPeaks();
}

void TazDlg::Show3DReal()
{
	if(!m_pReal3d)
		m_pReal3d = new Real3DDlg(this, &m_settings);

	if(!m_pReal3d->isVisible())
		m_pReal3d->show();
	m_pReal3d->activateWindow();

	m_pReal3d->CalcPeaks(m_sceneRealLattice.GetLattice()->GetWS3D(), m_latticecommon);
	//CalcPeaks();
}

void TazDlg::Show3DBZ()
{
	if(!m_pBZ3d)
	{
		m_pBZ3d = new BZ3DDlg(this, &m_settings);

		// also track current q position
		QObject::connect(&m_sceneRecip, SIGNAL(paramsChanged(const RecipParams&)),
			m_pBZ3d, SLOT(RecipParamsChanged(const RecipParams&)));
		m_sceneRecip.emitAllParams();
	}

	if(!m_pBZ3d->isVisible())
		m_pBZ3d->show();
	m_pBZ3d->activateWindow();

	m_pBZ3d->RenderBZ(m_sceneRecip.GetTriangle()->GetBZ3D(),
		m_latticecommon,
		&m_sceneRecip.GetTriangle()->GetBZ3DPlaneVerts(),
		&m_sceneRecip.GetTriangle()->GetBZ3DSymmVerts());
	//CalcPeaks();
}

#else

void TazDlg::Show3D() {}
void TazDlg::Show3DReal() {}
void TazDlg::Show3DBZ() {}

#endif


void TazDlg::EnableSmallq(bool bEnable)
{
	m_sceneRecip.GetTriangle()->SetqVisible(bEnable);
}

void TazDlg::EnableCoordAxes(bool bEnable)
{
	m_sceneRecip.GetTriangle()->SetCoordAxesVisible(bEnable);
}

void TazDlg::EnableBZ(bool bEnable)
{
	m_sceneRecip.GetTriangle()->SetBZVisible(bEnable);
}

void TazDlg::EnableWS(bool bEnable)
{
	m_sceneRealLattice.GetLattice()->SetWSVisible(bEnable);
}

void TazDlg::ShowEwaldSphere()
{
	EwaldSphere iEw = EWALD_NONE;
	if(m_pEwaldSphereNone->isChecked())
		iEw = EWALD_NONE;
	else if(m_pEwaldSphereKi->isChecked())
		iEw = EWALD_KI;
	else if(m_pEwaldSphereKf->isChecked())
		iEw = EWALD_KF;
	m_sceneRecip.GetTriangle()->SetEwaldSphereVisible(iEw);
}

void TazDlg::EnableRealQDir(bool bEnable)
{
	m_sceneReal.GetTasLayout()->SetRealQVisible(bEnable);
	m_sceneTof.GetTofLayout()->SetRealQVisible(bEnable);
}

// Q position
void TazDlg::recipParamsChanged(const RecipParams& params)
{
	t_real dQx = -params.Q_rlu[0], dQy = -params.Q_rlu[1], dQz = -params.Q_rlu[2];
	t_real dE = params.dE;

	tl::set_eps_0(dQx, g_dEps); tl::set_eps_0(dQy, g_dEps); tl::set_eps_0(dQz, g_dEps);
	tl::set_eps_0(dE, g_dEps);

	std::ostringstream ostrPos;
	ostrPos.precision(g_iPrecGfx);
	ostrPos << "Q = (" << dQx << ", " << dQy << ", " << dQz  << ") rlu";
	ostrPos << ", E = " << dE << " meV";

	ostrPos << ", BZ: ("
		<< params.G_rlu_accurate[0] << ", "
		<< params.G_rlu_accurate[1] << ", "
		<< params.G_rlu_accurate[2] << ")";

	m_pCoordQStatusMsg->setText(ostrPos.str().c_str());
}

// cursor position
void TazDlg::RecipCoordsChanged(t_real dh, t_real dk, t_real dl,
	bool bHasNearest, t_real dNearestH, t_real dNearestK, t_real dNearestL)
{
	tl::set_eps_0(dh, g_dEps); tl::set_eps_0(dk, g_dEps); tl::set_eps_0(dl, g_dEps);
	tl::set_eps_0(dNearestH, g_dEps); tl::set_eps_0(dNearestK, g_dEps); tl::set_eps_0(dNearestL, g_dEps);

	std::ostringstream ostrPos;
	ostrPos.precision(g_iPrecGfx);
	ostrPos << "Cur: (" << dh << ", " << dk << ", " << dl  << ") rlu";
	if(bHasNearest)
		ostrPos << ", BZ: ("
			<< dNearestH << ", " << dNearestK << ", " << dNearestL << ")";

	m_pCoordCursorStatusMsg->setText(ostrPos.str().c_str());
}

// cursor position
void TazDlg::RealCoordsChanged(t_real dh, t_real dk, t_real dl,
	bool bHasNearest, t_real dNearestH, t_real dNearestK, t_real dNearestL)
{
	tl::set_eps_0(dh, g_dEps); tl::set_eps_0(dk, g_dEps); tl::set_eps_0(dl, g_dEps);
	tl::set_eps_0(dNearestH, g_dEps); tl::set_eps_0(dNearestK, g_dEps); tl::set_eps_0(dNearestL, g_dEps);

	std::ostringstream ostrPos;
	ostrPos.precision(g_iPrecGfx);
	ostrPos << "Cur: (" << dh << ", " << dk << ", " << dl  << ") frac";
	if(bHasNearest)
		ostrPos << ", WS: ("
			<< dNearestH << ", " << dNearestK << ", " << dNearestL << ")";

	m_pCoordCursorStatusMsg->setText(ostrPos.str().c_str());
}



//--------------------------------------------------------------------------------
// parameter dialogs

void TazDlg::ShowRecipParams()
{
	focus_dlg(&m_dlgRecipParam);
}

void TazDlg::ShowRealParams()
{
	focus_dlg(&m_dlgRealParam);
}




//--------------------------------------------------------------------------------
// context menus

void TazDlg::RecipContextMenu(const QPoint& _pt)
{
	if(!m_pviewRecip) return;

	QPoint pt = this->m_pviewRecip->mapToGlobal(_pt);
	m_pMenuViewRecip->exec(pt);
}

void TazDlg::RealContextMenu(const QPoint& _pt)
{
	if(!m_pviewReal) return;

	QPoint pt = this->m_pviewReal->mapToGlobal(_pt);
	m_pMenuViewReal->exec(pt);
}



//--------------------------------------------------------------------------------
// obstacles

void TazDlg::ShowDeadAnglesDlg()
{
	if(!m_pDeadAnglesDlg)
	{
		m_pDeadAnglesDlg = new DeadAnglesDlg(this, &m_settings);
		QObject::connect(m_pDeadAnglesDlg, SIGNAL(ApplyDeadAngles(const std::vector<DeadAngle<t_real_glob>>&)),
			this, SLOT(ApplyDeadAngles(const std::vector<DeadAngle<t_real_glob>>&)));
	}

	m_pDeadAnglesDlg->SetDeadAngles(m_vecDeadAngles);
	focus_dlg(m_pDeadAnglesDlg);
}

void TazDlg::ApplyDeadAngles(const std::vector<DeadAngle<t_real>>& vecAngles)
{
	m_vecDeadAngles = vecAngles;
	if(m_sceneReal.GetTasLayout())
		m_sceneReal.GetTasLayout()->SetDeadAngles(&m_vecDeadAngles);
}



//--------------------------------------------------------------------------------
// about, log & help dialogs

void TazDlg::ShowAbout()
{
	if(!m_pAboutDlg)
		m_pAboutDlg = new AboutDlg(this, &m_settings);

	focus_dlg(m_pAboutDlg);
}

void TazDlg::ShowLog()
{
	if(!m_pLogDlg)
		m_pLogDlg = new LogDlg(this, &m_settings, m_strLogFile);

	focus_dlg(m_pLogDlg);
}

void TazDlg::ShowHelp()
{
#ifndef NO_HELP_ASSISTANT
	std::string strHelp = find_resource("res/doc/takin.qhc");
	if(strHelp != "")
	{
		std::string strHelpProg = "assistant";
		std::string strHelpProgVer = strHelpProg + "-qt" + tl::var_to_str(QT_VER);

		if(std::system((strHelpProgVer + " -collectionFile " + strHelp + "&").c_str()) == 0)
			return;
		if(std::system((strHelpProg + " -collectionFile " + strHelp + "&").c_str()) == 0)
			return;

		tl::log_warn("Help viewer not found, trying associated application.");
	}
#endif


	// try opening html files directly
	std::string strHelpHtml = find_resource("doc/index_help.html");
	if(strHelpHtml == "")	// try alternate directory
		strHelpHtml = find_resource("res/doc/index_help.html");
	if(strHelpHtml != "")
	{
		std::string strFile = "file:///" + fs::absolute(strHelpHtml).string();
		if(QDesktopServices::openUrl(QUrl(strFile.c_str())))
			return;
	}

	QMessageBox::critical(this, "Error", "Help could not be displayed.");
}

void TazDlg::ShowDevelDoc()
{
	std::string strHelpHtml = find_resource("doc/devel/html/index.html");
	if(strHelpHtml != "")
	{
		std::string strFile = "file:///" + fs::absolute(strHelpHtml).string();
		if(QDesktopServices::openUrl(QUrl(strFile.c_str())))
			return;
	}

	QMessageBox::critical(this, "Error", "Documentation could not be displayed.");
}

#include "taz.moc"
