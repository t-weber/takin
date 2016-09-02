/**
 * resolution calculation
 * @author tweber
 * @date 2013 - 2016
 * @license GPLv2
 */

#include "ResoDlg.h"
#include <iostream>
#include <fstream>
#include <iomanip>
//#include <boost/units/io.hpp>

#include "tlibs/string/string.h"
#include "tlibs/helper/flags.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/helper/misc.h"
#include "tlibs/math/math.h"
#include "tlibs/math/lattice.h"
#include "tlibs/time/chrono.h"

#include "libs/globals.h"
#include "mc.h"

#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>

using t_mat = ublas::matrix<t_real_reso>;
using t_vec = ublas::vector<t_real_reso>;

static const auto angs = tl::get_one_angstrom<t_real_reso>();
static const auto rads = tl::get_one_radian<t_real_reso>();
static const auto meV = tl::get_one_meV<t_real_reso>();
static const auto cm = tl::get_one_centimeter<t_real_reso>();
static const auto meters = tl::get_one_meter<t_real_reso>();
static const auto sec = tl::get_one_second<t_real_reso>();


ResoDlg::ResoDlg(QWidget *pParent, QSettings* pSettings)
	: QDialog(pParent), m_bDontCalc(1), m_pSettings(pSettings)
{
	setupUi(this);
	spinMCSample->setEnabled(0);		// TODO
	spinMCSampleLive->setEnabled(0);	// TODO

	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnSave->setIcon(load_icon("res/icons/document-save.svg"));
	btnLoad->setIcon(load_icon("res/icons/document-open.svg"));

	setupAlgos();
	QObject::connect(comboAlgo, SIGNAL(currentIndexChanged(int)), this, SLOT(AlgoChanged()));
	comboAlgo->setCurrentIndex(1);

	groupGuide->setChecked(false);

	m_vecSpinBoxes = {spinMonod, spinMonoMosaic, spinAnad,
		spinAnaMosaic, spinSampleMosaic,
		spinHCollMono, spinHCollBSample,
		spinHCollASample, spinHCollAna, spinVCollMono,
		spinVCollBSample, spinVCollASample, spinVCollAna,
		spinMonoRefl, spinAnaEffic,

		spinMonoW, spinMonoH, spinMonoThick, spinMonoCurvH, spinMonoCurvV,
		spinSampleW_Q, spinSampleW_perpQ, spinSampleH,
		spinAnaW, spinAnaH, spinAnaThick, spinAnaCurvH, spinAnaCurvV,
		spinSrcW, spinSrcH,
		spinGuideDivH, spinGuideDivV,
		spinDetW, spinDetH,
		spinDistMonoSample, spinDistSampleAna, spinDistAnaDet, spinDistSrcMono,

		spinMonoMosaicV, spinAnaMosaicV,
		spinSamplePosX, spinSamplePosY, spinSamplePosZ,

		spinDistTofPulseMono, spinDistTofMonoSample, spinDistTofSampleDet,
		spinDistTofPulseMonoSig, spinDistTofMonoSampleSig, spinDistTofSampleDetSig,
		spinTofPulseSig, spinTofMonoSig, spinTofDetSig,
		spinTof2thI, spinTofphI, spinTofphF,
		spinTof2thISig, spinTof2thFSig, spinTofphISig, spinTofphFSig,

		spinSigKi, spinSigKi_perp, spinSigKi_z,
		spinSigKf, spinSigKf_perp, spinSigKf_z,
	};

	m_vecSpinNames = {"reso/mono_d", "reso/mono_mosaic", "reso/ana_d",
		"reso/ana_mosaic", "reso/sample_mosaic",
		"reso/h_coll_mono", "reso/h_coll_before_sample",
		"reso/h_coll_after_sample", "reso/h_coll_ana",
		"reso/v_coll_mono", "reso/v_coll_before_sample",
		"reso/v_coll_after_sample", "reso/v_coll_ana",
		"reso/mono_refl", "reso/ana_effic",

		"reso/pop_mono_w", "reso/pop_mono_h", "reso/pop_mono_thick", "reso/pop_mono_curvh", "reso/pop_mono_curvv",
		"reso/pop_sample_wq", "reso/pop_sampe_wperpq", "reso/pop_sample_h",
		"reso/pop_ana_w", "reso/pop_ana_h", "reso/pop_ana_thick", "reso/pop_ana_curvh", "reso/pop_ana_curvv",
		"reso/pop_src_w", "reso/pop_src_h",
		"reso/pop_guide_divh", "reso/pop_guide_divv",
		"reso/pop_det_w", "reso/pop_det_h",
		"reso/pop_dist_mono_sample", "reso/pop_dist_sample_ana", "reso/pop_dist_ana_det", "reso/pop_dist_src_mono",

		"reso/eck_mono_mosaic_v", "reso/eck_ana_mosaic_v",
		"reso/eck_sample_pos_x", "reso/eck_sample_pos_y", "reso/eck_sample_pos_z",

		"reso/viol_dist_pulse_mono", "reso/viol_dist_mono_sample", "reso/viol_dist_sample_det",
		"reso/viol_dist_pulse_mono_sig", "reso/viol_dist_mono_sample_sig", "reso/viol_dist_sample_det_sig",
		"reso/viol_time_pulse_sig", "reso/viol_time_mono_sig", "reso/viol_time_det_sig",
		"reso/viol_angle_tt_i", "reso/viol_angle_ph_i", "reso/viol_angle_ph_f",
		"reso/viol_angle_tt_i_sig", "reso/viol_angle_tt_f_sig", "reso/viol_angle_ph_i_sig", "reso/viol_angle_ph_f_sig",

		"reso/simple_sig_ki", "reso/simple_sig_ki_perp", "reso/simple_sig_ki_z",
		"reso/simple_sig_kf", "reso/simple_sig_kf_perp", "reso/simple_sig_kf_z",
	};

	m_vecIntSpinBoxes = { spinMCNeutronsLive, spinMCSampleLive };
	m_vecIntSpinNames = { "reso/mc_live_neutrons", "reso/mc_live_sample_neutrons" };

	m_vecPosEditBoxes = {editE, editQ, editKi, editKf};
	m_vecPosEditNames = {"reso/E", "reso/Q", "reso/ki", "reso/kf"};

	m_vecCheckBoxes = {checkUseR0, checkUseKi3, checkUseKf3};
	m_vecCheckNames = {"reso/use_R0", "reso/use_ki3", "reso/use_kf3"};


	m_vecRadioPlus = {radioMonoScatterPlus, radioAnaScatterPlus,
		radioSampleScatterPlus,
		radioSampleCub, radioSrcRect, radioDetRect,
		radioTofDetSph};
	m_vecRadioMinus = {radioMonoScatterMinus, radioAnaScatterMinus,
		radioSampleScatterMinus, radioSampleCyl,
		radioSrcCirc, radioDetCirc,
		radioTofDetCyl};
	m_vecRadioNames = {"reso/mono_scatter_sense", "reso/ana_scatter_sense",
		"reso/sample_scatter_sense", "reso/pop_sample_cuboid",
		"reso/pop_source_rect", "reso/pop_det_rect",
		"reso/viol_det_sph"};

	m_vecComboBoxes = {/*comboAlgo,*/
		comboAnaHori, comboAnaVert,
		comboMonoHori, comboMonoVert};
	m_vecComboNames = {/*"reso/algo",*/
		"reso/pop_ana_use_curvh", "reso/pop_ana_use_curvv",
		"reso/pop_mono_use_curvh", "reso/pop_mono_use_curvv"};

	ReadLastConfig();

	QObject::connect(groupGuide, SIGNAL(toggled(bool)), this, SLOT(Calc()));

	for(QDoubleSpinBox* pSpinBox : m_vecSpinBoxes)
		QObject::connect(pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(Calc()));
	for(QSpinBox* pSpinBox : m_vecIntSpinBoxes)
		QObject::connect(pSpinBox, SIGNAL(valueChanged(int)), this, SLOT(Calc()));
	for(QLineEdit* pEditBox : m_vecPosEditBoxes)
		QObject::connect(pEditBox, SIGNAL(textEdited(const QString&)), this, SLOT(RefreshQEPos()));
	for(QRadioButton* pRadio : m_vecRadioPlus)
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(Calc()));
	for(QCheckBox* pCheck : m_vecCheckBoxes)
		QObject::connect(pCheck, SIGNAL(toggled(bool)), this, SLOT(Calc()));
	for(QComboBox* pCombo : m_vecComboBoxes)
		QObject::connect(pCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(Calc()));
	QObject::connect(comboAlgo, SIGNAL(currentIndexChanged(int)), this, SLOT(Calc()));

	connect(checkElli4dAutoCalc, SIGNAL(stateChanged(int)), this, SLOT(checkAutoCalcElli4dChanged()));
	connect(btnCalcElli4d, SIGNAL(clicked()), this, SLOT(CalcElli4d()));
	connect(btnMCGenerate, SIGNAL(clicked()), this, SLOT(MCGenerate()));
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(ButtonBoxClicked(QAbstractButton*)));
	connect(btnSave, SIGNAL(clicked()), this, SLOT(SaveRes()));
	connect(btnLoad, SIGNAL(clicked()), this, SLOT(LoadRes()));

	m_bDontCalc = 0;
	RefreshQEPos();
	//Calc();
}

ResoDlg::~ResoDlg() {}

void ResoDlg::setupAlgos()
{
	comboAlgo->addItem("TAS: Cooper-Nathans", static_cast<int>(ResoAlgo::CN));
	comboAlgo->addItem("TAS: Popovici", static_cast<int>(ResoAlgo::POP));
	comboAlgo->addItem("TAS: Eckold-Sobolev", static_cast<int>(ResoAlgo::ECK));
	comboAlgo->insertSeparator(3);
	comboAlgo->addItem("TOF: Violini", static_cast<int>(ResoAlgo::VIOL));
	comboAlgo->insertSeparator(5);
	comboAlgo->addItem("Simple", static_cast<int>(ResoAlgo::SIMPLE));
}

void ResoDlg::SaveRes()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		strDirLast = m_pSettings->value("reso/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getSaveFileName(this,
		"Save resolution configuration",
		strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);

	if(qstrFile == "")
		return;

	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);

	std::map<std::string, std::string> mapConf;
	Save(mapConf, strXmlRoot);

	tl::Prop<std::string> xml;
	xml.Add(mapConf);
	bool bOk = xml.Save(strFile, tl::PropType::XML);
	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not save resolution file.");

	if(bOk && m_pSettings)
		m_pSettings->setValue("reso/last_dir", QString(strDir.c_str()));
}

void ResoDlg::LoadRes()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		strDirLast = m_pSettings->value("reso/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getOpenFileName(this,
		"Open resolution configuration...",
		strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);
	if(qstrFile == "")
		return;


	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);

	tl::Prop<std::string> xml;
	if(!xml.Load(strFile, tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not load resolution file.");
		return;
	}

	Load(xml, strXmlRoot);
	if(m_pSettings)
		m_pSettings->setValue("reso/last_dir", QString(strDir.c_str()));

}

void ResoDlg::RefreshQEPos()
{
	try
	{
		tl::t_wavenumber_si<t_real_reso> Q = editQ->text().toDouble() / angs;
		tl::t_wavenumber_si<t_real_reso> ki = editKi->text().toDouble() / angs;
		tl::t_wavenumber_si<t_real_reso> kf = editKf->text().toDouble() / angs;
		//t_real_reso dE = editE->text().toDouble();
		tl::t_energy_si<t_real_reso> E = tl::get_energy_transfer(ki, kf);

		tl::t_angle_si<t_real_reso> kiQ = tl::get_angle_ki_Q(ki, kf, Q, 1);
		tl::t_angle_si<t_real_reso> kfQ = tl::get_angle_kf_Q(ki, kf, Q, 1);
		tl::t_angle_si<t_real_reso> twotheta = tl::get_sample_twotheta(ki, kf, Q, 1);

		const t_real_reso dMono = spinMonod->value();
		const t_real_reso dAna = spinAnad->value();

		m_simpleparams.ki = m_tofparams.ki = m_tasparams.ki = ki;
		m_simpleparams.kf = m_tofparams.kf = m_tasparams.kf = kf;
		m_simpleparams.E = m_tofparams.E = m_tasparams.E = E;
		m_simpleparams.Q = m_tofparams.Q = m_tasparams.Q = Q;

		m_simpleparams.twotheta = m_tofparams.twotheta = m_tasparams.twotheta = twotheta;
		m_simpleparams.angle_ki_Q = m_tofparams.angle_ki_Q = m_tasparams.angle_ki_Q = kiQ;
		m_simpleparams.angle_kf_Q = m_tofparams.angle_kf_Q = m_tasparams.angle_kf_Q = kfQ;

		m_tasparams.thetam = 0.5 * tl::get_mono_twotheta(ki, dMono*angs, 1);
		m_tasparams.thetaa = 0.5 * tl::get_mono_twotheta(kf, dAna*angs, 1);

#ifndef NDEBUG
		tl::log_debug("Manually changed parameters: ",
			"ki=",ki, ", kf=", kf, ", Q=",Q, ", E=", E,
			", tt=", twotheta, ", kiQ=", kiQ, ", kfQ=", kfQ, ".");
#endif
		Calc();
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}
}

void ResoDlg::Calc()
{
	try
	{
		m_bEll4dCurrent = 0;
		if(m_bDontCalc) return;

		EckParams &cn = m_tasparams;
		ViolParams &tof = m_tofparams;
		SimpleResoParams &simple = m_simpleparams;

		ResoResults &res = m_res;

		// CN
		cn.mono_d = t_real_reso(spinMonod->value()) * angs;
		cn.mono_mosaic = t_real_reso(tl::m2r(spinMonoMosaic->value())) * rads;
		cn.ana_d = t_real_reso(spinAnad->value()) * angs;
		cn.ana_mosaic = t_real_reso(tl::m2r(spinAnaMosaic->value())) * rads;
		cn.sample_mosaic = t_real_reso(tl::m2r(spinSampleMosaic->value())) * rads;

		cn.dmono_sense = (radioMonoScatterPlus->isChecked() ? +1. : -1.);
		cn.dana_sense = (radioAnaScatterPlus->isChecked() ? +1. : -1.);
		cn.dsample_sense = (radioSampleScatterPlus->isChecked() ? +1. : -1.);
		//std::cout << "sample sense: " << cn.dsample_sense << std::endl;
		//if(spinQ->value() < 0.)
		//	cn.dsample_sense = -cn.dsample_sense;

		cn.coll_h_pre_mono = t_real_reso(tl::m2r(spinHCollMono->value())) * rads;
		cn.coll_h_pre_sample = t_real_reso(tl::m2r(spinHCollBSample->value())) * rads;
		cn.coll_h_post_sample = t_real_reso(tl::m2r(spinHCollASample->value())) * rads;
		cn.coll_h_post_ana = t_real_reso(tl::m2r(spinHCollAna->value())) * rads;

		cn.coll_v_pre_mono = t_real_reso(tl::m2r(spinVCollMono->value())) * rads;
		cn.coll_v_pre_sample = t_real_reso(tl::m2r(spinVCollBSample->value())) * rads;
		cn.coll_v_post_sample = t_real_reso(tl::m2r(spinVCollASample->value())) * rads;
		cn.coll_v_post_ana = t_real_reso(tl::m2r(spinVCollAna->value())) * rads;

		cn.dmono_refl = spinMonoRefl->value();
		cn.dana_effic = spinAnaEffic->value();

		if(checkUseR0->isChecked())
			cn.flags |= CALC_R0;
		else
			cn.flags &= ~CALC_R0;
		if(checkUseKi3->isChecked())
			cn.flags |= CALC_KI3;
		else
			cn.flags &= ~CALC_KI3;
		if(checkUseKf3->isChecked())
			cn.flags |= CALC_KF3;
		else
			cn.flags &= ~CALC_KF3;


		// Position
		/*cn.ki = t_real_reso(editKi->text().toDouble()) / angs;
		cn.kf = t_real_reso(editKf->text().toDouble()) / angs;
		cn.Q = t_real_reso(editQ->text().toDouble()) / angs;
		cn.E = t_real_reso(editE->text().toDouble()) * meV;
		//cn.E = tl::get_energy_transfer(cn.ki, cn.kf);
		//std::cout << "E = " << editE->text().toStdString() << std::endl;*/


		// Pop
		cn.mono_w = t_real_reso(spinMonoW->value()) * cm;
		cn.mono_h = t_real_reso(spinMonoH->value()) * cm;
		cn.mono_thick = t_real_reso(spinMonoThick->value()) * cm;
		cn.mono_curvh = t_real_reso(spinMonoCurvH->value()) * cm;
		cn.mono_curvv = t_real_reso(spinMonoCurvV->value()) * cm;
		cn.bMonoIsCurvedH = cn.bMonoIsCurvedV = 0;
		cn.bMonoIsOptimallyCurvedH = cn.bMonoIsOptimallyCurvedV = 0;
		spinMonoCurvH->setEnabled(0); spinMonoCurvV->setEnabled(0);

		if(comboMonoHori->currentIndex()==2)
		{
			cn.bMonoIsCurvedH = 1;
			spinMonoCurvH->setEnabled(1);
		}
		else if(comboMonoHori->currentIndex()==1)
			cn.bMonoIsCurvedH = cn.bMonoIsOptimallyCurvedH = 1;

		if(comboMonoVert->currentIndex()==2)
		{
			cn.bMonoIsCurvedV = 1;
			spinMonoCurvV->setEnabled(1);
		}
		else if(comboMonoVert->currentIndex()==1)
			cn.bMonoIsCurvedV = cn.bMonoIsOptimallyCurvedV = 1;

		cn.ana_w = t_real_reso(spinAnaW->value()) *cm;
		cn.ana_h = t_real_reso(spinAnaH->value()) * cm;
		cn.ana_thick = t_real_reso(spinAnaThick->value()) * cm;
		cn.ana_curvh = t_real_reso(spinAnaCurvH->value()) * cm;
		cn.ana_curvv = t_real_reso(spinAnaCurvV->value()) * cm;
		cn.bAnaIsCurvedH = cn.bAnaIsCurvedV = 0;
		cn.bAnaIsOptimallyCurvedH = cn.bAnaIsOptimallyCurvedV = 0;
		spinAnaCurvH->setEnabled(0); spinAnaCurvV->setEnabled(0);

		if(comboAnaHori->currentIndex()==2)
		{
			cn.bAnaIsCurvedH = 1;
			spinAnaCurvH->setEnabled(1);
		}
		else if(comboAnaHori->currentIndex()==1)
			cn.bAnaIsCurvedH = cn.bAnaIsOptimallyCurvedH = 1;

		if(comboAnaVert->currentIndex()==2)
		{
			cn.bAnaIsCurvedV = 1;
			spinAnaCurvV->setEnabled(1);
		}
		else if(comboAnaVert->currentIndex()==1)
			cn.bAnaIsCurvedV = cn.bAnaIsOptimallyCurvedV = 1;

		cn.bSampleCub = radioSampleCub->isChecked();
		cn.sample_w_q = t_real_reso(spinSampleW_Q->value()) * cm;
		cn.sample_w_perpq = t_real_reso(spinSampleW_perpQ->value()) * cm;
		cn.sample_h = t_real_reso(spinSampleH->value()) * cm;

		cn.bSrcRect = radioSrcRect->isChecked();
		cn.src_w = t_real_reso(spinSrcW->value()) * cm;
		cn.src_h = t_real_reso(spinSrcH->value()) * cm;

		cn.bDetRect = radioDetRect->isChecked();
		cn.det_w = t_real_reso(spinDetW->value()) * cm;
		cn.det_h = t_real_reso(spinDetH->value()) * cm;

		cn.bGuide = groupGuide->isChecked();
		cn.guide_div_h = t_real_reso(tl::m2r(spinGuideDivH->value())) * rads;
		cn.guide_div_v = t_real_reso(tl::m2r(spinGuideDivV->value())) * rads;

		cn.dist_mono_sample = t_real_reso(spinDistMonoSample->value()) * cm;
		cn.dist_sample_ana = t_real_reso(spinDistSampleAna->value()) * cm;
		cn.dist_ana_det = t_real_reso(spinDistAnaDet->value()) * cm;
		cn.dist_src_mono = t_real_reso(spinDistSrcMono->value()) * cm;


		// Eck
		cn.mono_mosaic_v = t_real_reso(tl::m2r(spinMonoMosaicV->value())) * rads;
		cn.ana_mosaic_v = t_real_reso(tl::m2r(spinAnaMosaicV->value())) * rads;
		cn.pos_x = t_real_reso(spinSamplePosX->value()) * cm;
		cn.pos_y = t_real_reso(spinSamplePosY->value()) * cm;
		cn.pos_z = t_real_reso(spinSamplePosZ->value()) * cm;

		// TODO
		cn.mono_numtiles_h = 1;
		cn.mono_numtiles_v = 1;
		cn.ana_numtiles_h = 1;
		cn.ana_numtiles_v = 1;


		// TOF
		tof.len_pulse_mono = t_real_reso(spinDistTofPulseMono->value()) * cm;
		tof.len_mono_sample = t_real_reso(spinDistTofMonoSample->value()) * cm;
		tof.len_sample_det = t_real_reso(spinDistTofSampleDet->value()) * cm;

		tof.sig_len_pulse_mono = t_real_reso(spinDistTofPulseMonoSig->value()) * cm;
		tof.sig_len_mono_sample = t_real_reso(spinDistTofMonoSampleSig->value()) * cm;
		tof.sig_len_sample_det = t_real_reso(spinDistTofSampleDetSig->value()) * cm;

		tof.sig_pulse = t_real_reso(spinTofPulseSig->value() * 1e-6) * sec;
		tof.sig_mono = t_real_reso(spinTofMonoSig->value() * 1e-6) * sec;
		tof.sig_det = t_real_reso(spinTofDetSig->value() * 1e-6) * sec;

		tof.twotheta_i = tl::d2r(t_real_reso(spinTof2thI->value())) * rads;
		tof.angle_outplane_i = tl::d2r(t_real_reso(spinTofphI->value())) * rads;
		tof.angle_outplane_f = tl::d2r(t_real_reso(spinTofphF->value())) * rads;

		tof.sig_twotheta_i = tl::d2r(t_real_reso(spinTof2thISig->value())) * rads;
		tof.sig_twotheta_f = tl::d2r(t_real_reso(spinTof2thFSig->value())) * rads;
		tof.sig_outplane_i = tl::d2r(t_real_reso(spinTofphISig->value())) * rads;
		tof.sig_outplane_f = tl::d2r(t_real_reso(spinTofphFSig->value())) * rads;

		tof.det_shape = radioTofDetSph->isChecked() ? TofDetShape::SPH : TofDetShape::CYL;


		// Simple
		simple.sig_ki = t_real_reso(spinSigKi->value()) / angs;
		simple.sig_kf = t_real_reso(spinSigKf->value()) / angs;
		simple.sig_ki_perp = t_real_reso(spinSigKi_perp->value()) / angs;
		simple.sig_kf_perp = t_real_reso(spinSigKf_perp->value()) / angs;
		simple.sig_ki_z = t_real_reso(spinSigKi_z->value()) / angs;
		simple.sig_kf_z = t_real_reso(spinSigKf_z->value()) / angs;

		//tl::log_debug(m_tofparams.angle_ki_Q);
		//tl::log_debug(m_tofparams.angle_kf_Q);
		//tl::log_debug(m_tofparams.twotheta);

		// Calculation
		switch(ResoDlg::GetSelectedAlgo())
		{
			case ResoAlgo::CN: res = calc_cn(cn); break;
			case ResoAlgo::POP: res = calc_pop(cn); break;
			case ResoAlgo::ECK: res = calc_eck(cn); break;
			case ResoAlgo::VIOL: res = calc_viol(tof); break;
			case ResoAlgo::SIMPLE: res = calc_simplereso(simple); break;
			default: tl::log_err("Unknown resolution algorithm selected."); return;
		}

		editE->setText(tl::var_to_str(t_real_reso(cn.E/meV), g_iPrec).c_str());
		//if(m_pInstDlg) m_pInstDlg->SetParams(cn, res);
		//if(m_pScatterDlg) m_pScatterDlg->SetParams(cn, res);

		if(res.bOk)
		{
			// --------------------------------------------------------------------------------
			// Vanadium width
			struct Ellipse2d<t_real_reso> ellVa =
				calc_res_ellipse<t_real_reso>(res.reso, res.reso_v, res.reso_s,
				res.Q_avg, 0, 3, 1, 2, -1);

			//std::cout << ellVa.phi/M_PI*180. << std::endl;
			t_real_reso dVanadiumFWHM_Q = ellVa.x_hwhm*2.;
			t_real_reso dVanadiumFWHM_E = ellVa.y_hwhm*2.;
			if(std::fabs(ellVa.phi) >= tl::get_pi<t_real_reso>()/4.)
			{
				dVanadiumFWHM_Q = ellVa.y_hwhm*2.;
				dVanadiumFWHM_E = ellVa.x_hwhm*2.;
			}
			// --------------------------------------------------------------------------------

			const std::string& strAA_1 = tl::get_spec_char_utf8("AA")
				+ tl::get_spec_char_utf8("sup-")
				+ tl::get_spec_char_utf8("sup1");
			const std::string& strAA_3 = tl::get_spec_char_utf8("AA")
				+ tl::get_spec_char_utf8("sup-")
				+ tl::get_spec_char_utf8("sup3");

			std::ostringstream ostrRes;

			//ostrRes << std::scientific;
			ostrRes.precision(g_iPrec);
			ostrRes << "<html><body>\n";

			ostrRes << "<p><b>Correction Factors:</b>\n";
			ostrRes << "\t<ul><li>Resolution Volume: " << res.dResVol << " meV " << strAA_3 << "</li>\n";
			ostrRes << "\t<li>R0: " << res.dR0 << "</li></ul></p>\n\n";

			ostrRes << "<p><b>Bragg FWHMs:</b>\n";
			ostrRes << "\t<ul><li>Q_para: " << res.dBraggFWHMs[0] << " " << strAA_1 << "</li>\n";
			ostrRes << "\t<li>Q_ortho: " << res.dBraggFWHMs[1] << " " << strAA_1 << "</li>\n";
			ostrRes << "\t<li>Q_z: " << res.dBraggFWHMs[2] << " " << strAA_1 << "</li>\n";
			ostrRes << "\t<li>E: " << res.dBraggFWHMs[3] << " meV</li></ul></p>\n\n";

			ostrRes << "<p><b>Vanadium FWHM:</b> "
				<< dVanadiumFWHM_Q << " " << strAA_1 << ", "
				<< dVanadiumFWHM_E << " meV</p>\n\n";

			ostrRes << "<p><b>Resolution Matrix (Q_para, Q_ortho, Q_z, E):</b>\n\n";
			ostrRes << "<blockquote><table border=\"0\" width=\"75%\">\n";
			for(std::size_t i=0; i<res.reso.size1(); ++i)
			{
				ostrRes << "<tr>\n";
				for(std::size_t j=0; j<res.reso.size2(); ++j)
					ostrRes << "<td>" << std::setw(g_iPrec*2) << res.reso(i,j) << "</td>";
				ostrRes << "</tr>\n";

				if(i!=res.reso.size1()-1)
					ostrRes << "\n";
			}
			ostrRes << "</table></blockquote></p>\n";

			ostrRes << "<p><b>Resolution Vector:</b> ";
			for(std::size_t iVec=0; iVec<res.reso_v.size(); ++iVec)
			{
				ostrRes << res.reso_v[iVec];
				if(iVec != res.reso_v.size()-1)
					ostrRes << ", ";
			}
			ostrRes << "</p>\n";

			ostrRes << "<p><b>Resolution Scalar</b>: " << res.reso_s << "</p>\n";

			ostrRes << "</body></html>";

			editResults->setHtml(QString::fromUtf8(ostrRes.str().c_str()));
			labelStatus->setText("Calculation successful.");

#ifndef NDEBUG
			// check against ELASTIC approximation for perp. slope from Shirane p. 268
			// valid for small mosaicities
			t_real_reso dEoverQperp = tl::co::hbar*tl::co::hbar*cn.ki / tl::co::m_n
				* units::cos(cn.twotheta/2.)
				* (1. + units::tan(units::abs(cn.twotheta/2.))
				* units::tan(units::abs(cn.twotheta/2.) - units::abs(cn.thetam)))
					/ meV / angs;

			tl::log_info("E/Q_perp (approximation for ki=kf) = ", dEoverQperp, " meV*A");
			tl::log_info("E/Q_perp (2nd approximation for ki=kf) = ", t_real_reso(4.*cn.ki * angs), " meV*A");

			//std::cout << "thetas = " << m_tasparams.thetas/rads/M_PI*180. << std::endl;
			//std::cout << "2thetam = " << 2.*m_tasparams.thetam/rads/M_PI*180. << std::endl;
			//std::cout << "2thetaa = " << 2.*m_tasparams.thetaa/rads/M_PI*180. << std::endl;
#endif

			if(checkElli4dAutoCalc->isChecked())
			{
				CalcElli4d();
				m_bEll4dCurrent = 1;
			}

			if(groupSim->isChecked())
				RefreshSimCmd();

			// calculate rlu quadric if a sample is defined
			if(m_bHasUB)
			{
				// hkl crystal system:
				// Qavg system in 1/A -> rotate back to orient system in 1/A ->
				// transform to hkl rlu system
				t_mat matQVec0 = tl::rotation_matrix_2d(-m_dAngleQVec0);
				matQVec0.resize(4,4, true);
				matQVec0(2,2) = matQVec0(3,3) = 1.;
				matQVec0(2,0) = matQVec0(2,1) = matQVec0(2,3) = 0.;
				matQVec0(3,0) = matQVec0(3,1) = matQVec0(3,2) = 0.;
				matQVec0(0,2) = matQVec0(0,3) = 0.;
				matQVec0(1,2) = matQVec0(1,3) = 0.;
				const t_mat matQVec0inv = ublas::trans(matQVec0);

				const t_mat matUBinvQVec0 = ublas::prod(m_matUBinv, matQVec0);
				const t_mat matQVec0invUB = ublas::prod(matQVec0inv, m_matUB);
				// TODO: check: does this work for non-cubic crystals, i.e. non-orthogonal B matrices?
				m_resoHKL = tl::transform(m_res.reso, matQVec0invUB, 1);
				//m_resoHKL = ublas::prod(m_res.reso, matUBinvQVec0);
				//m_resoHKL = ublas::prod(matQVec0invUB, m_resoHKL);
				m_Q_avgHKL = ublas::prod(matUBinvQVec0, m_res.Q_avg);

				//std::cout << tl::r2d(m_dAngleQVec0) << std::endl;
				//std::cout << m_Q_avgHKL << std::endl;
				//std::cout << m_resoHKL << std::endl;


				// system of scattering plane: (orient1, orient2, up)
				// Qavg system in 1/A -> rotate back to orient system in 1/A ->
				// transform to hkl rlu system -> rotate forward to orient system in rlu
				const t_mat matToOrient = ublas::prod(m_matUrlu, matUBinvQVec0);
				const t_mat matToOrientinv = ublas::prod(matQVec0invUB, m_matUinvrlu);

				// TODO: check: does this work for non-cubic crystals, i.e. non-orthogonal B matrices?
				m_resoOrient = tl::transform(m_res.reso, matToOrientinv, 1);
				//m_resoOrient = ublas::prod(m_res.reso, matToOrient);
				//m_resoOrient = ublas::prod(matToOrientinv, m_resoOrient);
				m_Q_avgOrient = ublas::prod(matToOrient, m_res.Q_avg);
				//std::cout << m_Q_avgOrient << std::endl;

				if(m_res.reso_v.size() == 4)
				{
					m_reso_vHKL = ublas::prod(matUBinvQVec0, m_res.reso_v);
					m_reso_vOrient = ublas::prod(matToOrient, m_res.reso_v);
				}
			}


			// generate live MC neutrons
			const std::size_t iNumMC = spinMCNeutronsLive->value();
			if(iNumMC)
			{
				McNeutronOpts<t_mat> opts;
				opts.bCenter = 0;
				opts.matU = m_matU;
				opts.matB = m_matB;
				opts.matUB = m_matUB;
				opts.matUinv = m_matUinv;
				opts.matBinv = m_matBinv;
				opts.matUBinv = m_matUBinv;

				t_mat* pMats[] = {&opts.matU, &opts.matB, &opts.matUB,
					&opts.matUinv, &opts.matBinv, &opts.matUBinv};

				for(t_mat *pMat : pMats)
				{
					pMat->resize(4,4,1);

					for(int i0=0; i0<3; ++i0)
						(*pMat)(i0,3) = (*pMat)(3,i0) = 0.;
					(*pMat)(3,3) = 1.;
				}

				opts.dAngleQVec0 = m_dAngleQVec0;

				if(m_bHasUB)
				{
					// rlu system
					opts.coords = McNeutronCoords::RLU;
					if(m_vecMC_HKL.size() != iNumMC)
						m_vecMC_HKL.resize(iNumMC);
					mc_neutrons(m_ell4d, iNumMC, opts, m_vecMC_HKL.begin());
				}
				else
					m_vecMC_HKL.clear();

				// Qpara, Qperp system
				opts.coords = McNeutronCoords::DIRECT;
				if(m_vecMC_direct.size() != iNumMC)
					m_vecMC_direct.resize(iNumMC);
				mc_neutrons(m_ell4d, iNumMC, opts, m_vecMC_direct.begin());
			}
			else
			{
				m_vecMC_direct.clear();
				m_vecMC_HKL.clear();
			}

			EmitResults();
		}
		else
		{
			QString strErr = "Error: ";
			strErr += res.strErr.c_str();
			labelStatus->setText(QString("<font color='red'>") + strErr + QString("</font>"));
		}
	}
	catch(const std::exception& ex)
	{
		tl::log_err("Cannot calculate resolution: ", ex.what(), ".");
	}
}


void ResoDlg::RefreshSimCmd()
{
	const t_real_reso dMin = t_real_reso(tl::get_pi<t_real_reso>()/180./60.);

	std::ostringstream ostrCmd;
	ostrCmd.precision(g_iPrec);

	ostrCmd << "./templateTAS -n 1e6 verbose=1 ";

	ostrCmd << "KI=" << t_real_reso(m_tasparams.ki * angs) << " ";
	ostrCmd << "KF=" << t_real_reso(m_tasparams.kf * angs) << " ";
	ostrCmd << "QM=" << t_real_reso(m_tasparams.Q * angs) << " ";
	ostrCmd << "EN=" << t_real_reso(m_tasparams.E / meV) << " ";
	//ostrCmt << "FX=" << (m_tasparams.bki_fix ? "1" : "2") << " ";

	ostrCmd << "L1=" << t_real_reso(m_tasparams.dist_src_mono / meters) << " ";
	ostrCmd << "L2=" << t_real_reso(m_tasparams.dist_mono_sample / meters) << " ";
	ostrCmd << "L3=" << t_real_reso(m_tasparams.dist_sample_ana / meters) << " ";
	ostrCmd << "L4=" << t_real_reso(m_tasparams.dist_ana_det / meters) << " ";

	ostrCmd << "SM=" << m_tasparams.dmono_sense << " ";
	ostrCmd << "SS=" << m_tasparams.dsample_sense << " ";
	ostrCmd << "SA=" << m_tasparams.dana_sense << " ";

	ostrCmd << "DM=" << t_real_reso(m_tasparams.mono_d / angs) << " ";
	ostrCmd << "DA=" << t_real_reso(m_tasparams.ana_d / angs) << " ";

	ostrCmd << "RMV=" << t_real_reso(m_tasparams.mono_curvv / meters) << " ";
	ostrCmd << "RMH=" << t_real_reso(m_tasparams.mono_curvh / meters) << " ";
	ostrCmd << "RAV=" << t_real_reso(m_tasparams.ana_curvv / meters) << " ";
	ostrCmd << "RAH=" << t_real_reso(m_tasparams.ana_curvh / meters) << " ";

	ostrCmd << "ETAM=" << t_real_reso(m_tasparams.mono_mosaic/rads/dMin) << " ";
	ostrCmd << "ETAA=" << t_real_reso(m_tasparams.ana_mosaic/rads/dMin) << " ";

	ostrCmd << "ALF1=" << t_real_reso(m_tasparams.coll_h_pre_mono/rads/dMin) << " ";
	ostrCmd << "ALF2=" << t_real_reso(m_tasparams.coll_h_pre_sample/rads/dMin) << " ";
	ostrCmd << "ALF3=" << t_real_reso(m_tasparams.coll_h_post_sample/rads/dMin) << " ";
	ostrCmd << "ALF4=" << t_real_reso(m_tasparams.coll_h_post_ana/rads/dMin) << " ";
	ostrCmd << "BET1=" << t_real_reso(m_tasparams.coll_v_pre_mono/rads/dMin) << " ";
	ostrCmd << "BET2=" << t_real_reso(m_tasparams.coll_v_pre_sample/rads/dMin) << " ";
	ostrCmd << "BET3=" << t_real_reso(m_tasparams.coll_v_post_sample/rads/dMin) << " ";
	ostrCmd << "BET4=" << t_real_reso(m_tasparams.coll_v_post_ana/rads/dMin) << " ";

	ostrCmd << "WM=" << t_real_reso(m_tasparams.mono_w / meters) << " ";
	ostrCmd << "HM=" << t_real_reso(m_tasparams.mono_h / meters) << " ";
	ostrCmd << "WA=" << t_real_reso(m_tasparams.ana_w / meters) << " ";
	ostrCmd << "HA=" << t_real_reso(m_tasparams.ana_h / meters) << " ";

	ostrCmd << "NVM=" << m_tasparams.mono_numtiles_v << " ";
	ostrCmd << "NHM=" << m_tasparams.mono_numtiles_h << " ";
	ostrCmd << "NVA=" << m_tasparams.ana_numtiles_v << " ";
	ostrCmd << "NHA=" << m_tasparams.ana_numtiles_h << " ";

	editSim->setPlainText(ostrCmd.str().c_str());
}

void ResoDlg::SetSelectedAlgo(ResoAlgo algo)
{
	for(int iItem=0; iItem<comboAlgo->count(); ++iItem)
	{
		QVariant varAlgo = comboAlgo->itemData(iItem);
		if(algo == static_cast<ResoAlgo>(varAlgo.toInt()))
		{
			comboAlgo->setCurrentIndex(iItem);
			//tl::log_debug("Set algo: ", iItem);
			return;
		}
	}

	tl::log_err("Unknown resolution algorithm set.");
}

ResoAlgo ResoDlg::GetSelectedAlgo() const
{
	ResoAlgo algoSel = ResoAlgo::UNKNOWN;
	QVariant varAlgo = comboAlgo->itemData(comboAlgo->currentIndex());
	if(varAlgo == QVariant::Invalid)
		tl::log_err("Unknown resolution algorithm selected.");
	else
		algoSel = static_cast<ResoAlgo>(varAlgo.toInt());
	return algoSel;
}

void ResoDlg::EmitResults()
{
	ResoAlgo algoSel = ResoDlg::GetSelectedAlgo();
	EllipseDlgParams params;

	params.reso = &m_res.reso;
	params.reso_v = &m_res.reso_v;
	params.reso_s = m_res.reso_s;
	params.Q_avg = &m_res.Q_avg;

	params.resoHKL = &m_resoHKL;
	params.reso_vHKL = &m_reso_vHKL;
	params.Q_avgHKL = &m_Q_avgHKL;

	params.resoOrient = &m_resoOrient;
	params.reso_vOrient = &m_reso_vOrient;
	params.Q_avgOrient = &m_Q_avgOrient;
	
	params.vecMC_direct = &m_vecMC_direct;
	params.vecMC_HKL = &m_vecMC_HKL;

	params.algo = algoSel;

	emit ResoResultsSig(params);
}

void ResoDlg::WriteLastConfig()
{
	if(!m_pSettings)
		return;

	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
		m_pSettings->setValue(m_vecSpinNames[iSpinBox].c_str(), m_vecSpinBoxes[iSpinBox]->value());
	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
		m_pSettings->setValue(m_vecIntSpinNames[iSpinBox].c_str(), m_vecIntSpinBoxes[iSpinBox]->value());
	for(std::size_t iEditBox=0; iEditBox<m_vecPosEditBoxes.size(); ++iEditBox)
		m_pSettings->setValue(m_vecPosEditNames[iEditBox].c_str(), m_vecPosEditBoxes[iEditBox]->text().toDouble());
	for(std::size_t iRadio=0; iRadio<m_vecRadioPlus.size(); ++iRadio)
		m_pSettings->setValue(m_vecRadioNames[iRadio].c_str(), m_vecRadioPlus[iRadio]->isChecked());
	for(std::size_t iCheck=0; iCheck<m_vecCheckBoxes.size(); ++iCheck)
		m_pSettings->setValue(m_vecCheckNames[iCheck].c_str(), m_vecCheckBoxes[iCheck]->isChecked());
	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
		m_pSettings->setValue(m_vecComboNames[iCombo].c_str(), m_vecComboBoxes[iCombo]->currentIndex());

	const int iAlgo = static_cast<int>(ResoDlg::GetSelectedAlgo());
		m_pSettings->setValue("reso/algo", iAlgo);

	m_pSettings->setValue("reso/use_guide", groupGuide->isChecked());
}

void ResoDlg::ReadLastConfig()
{
	if(!m_pSettings)
		return;

	bool bOldDontCalc = m_bDontCalc;
	m_bDontCalc = 1;

	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
	{
		if(!m_pSettings->contains(m_vecSpinNames[iSpinBox].c_str()))
			continue;
		m_vecSpinBoxes[iSpinBox]->setValue(m_pSettings->value(m_vecSpinNames[iSpinBox].c_str()).value<t_real_reso>());
	}

	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
	{
		if(!m_pSettings->contains(m_vecIntSpinNames[iSpinBox].c_str()))
			continue;
		m_vecIntSpinBoxes[iSpinBox]->setValue(m_pSettings->value(m_vecIntSpinNames[iSpinBox].c_str()).value<int>());
	}

	for(std::size_t iEditBox=0; iEditBox<m_vecPosEditBoxes.size(); ++iEditBox)
	{
		if(!m_pSettings->contains(m_vecPosEditNames[iEditBox].c_str()))
			continue;
		t_real_reso dEditVal = m_pSettings->value(m_vecPosEditNames[iEditBox].c_str()).value<t_real_reso>();
		m_vecPosEditBoxes[iEditBox]->setText(tl::var_to_str(dEditVal, g_iPrec).c_str());
	}

	for(std::size_t iCheckBox=0; iCheckBox<m_vecCheckBoxes.size(); ++iCheckBox)
	{
		if(!m_pSettings->contains(m_vecCheckNames[iCheckBox].c_str()))
			continue;
		m_vecCheckBoxes[iCheckBox]->setChecked(m_pSettings->value(m_vecCheckNames[iCheckBox].c_str()).value<bool>());
	}

	for(std::size_t iRadio=0; iRadio<m_vecRadioPlus.size(); ++iRadio)
	{
		if(!m_pSettings->contains(m_vecRadioNames[iRadio].c_str()))
			continue;

		bool bChecked = m_pSettings->value(m_vecRadioNames[iRadio].c_str()).value<bool>();
		if(bChecked)
		{
			m_vecRadioPlus[iRadio]->setChecked(1);
			//m_vecRadioMinus[iRadio]->setChecked(0);;
		}
		else
		{
			//m_vecRadioPlus[iRadio]->setChecked(0);
			m_vecRadioMinus[iRadio]->setChecked(1);;
		}
	}

	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
	{
		if(!m_pSettings->contains(m_vecComboNames[iCombo].c_str()))
			continue;
		m_vecComboBoxes[iCombo]->setCurrentIndex(
			m_pSettings->value(m_vecComboNames[iCombo].c_str()).value<int>());
	}

	if(m_pSettings->contains("reso/algo"))
		SetSelectedAlgo(static_cast<ResoAlgo>(m_pSettings->value("reso/algo").value<int>()));

	groupGuide->setChecked(m_pSettings->value("reso/use_guide").value<bool>());

	m_bDontCalc = bOldDontCalc;
	RefreshQEPos();
	//Calc();
}

void ResoDlg::Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot)
{
	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << m_vecSpinBoxes[iSpinBox]->value();

		mapConf[strXmlRoot + m_vecSpinNames[iSpinBox]] = ostrVal.str();
	}

	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << m_vecIntSpinBoxes[iSpinBox]->value();

		mapConf[strXmlRoot + m_vecIntSpinNames[iSpinBox]] = ostrVal.str();
	}

	for(std::size_t iEditBox=0; iEditBox<m_vecPosEditBoxes.size(); ++iEditBox)
	{
		std::string strVal = m_vecPosEditBoxes[iEditBox]->text().toStdString();
		mapConf[strXmlRoot + m_vecPosEditNames[iEditBox]] = strVal;
	}

	for(std::size_t iCheckBox=0; iCheckBox<m_vecCheckBoxes.size(); ++iCheckBox)
		mapConf[strXmlRoot + m_vecCheckNames[iCheckBox]] = (m_vecCheckBoxes[iCheckBox]->isChecked() ? "1" : "0");

	for(std::size_t iRadio=0; iRadio<m_vecRadioPlus.size(); ++iRadio)
		mapConf[strXmlRoot + m_vecRadioNames[iRadio]] = (m_vecRadioPlus[iRadio]->isChecked() ? "1" : "0");

	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
		mapConf[strXmlRoot + m_vecComboNames[iCombo]] = tl::var_to_str<int>(m_vecComboBoxes[iCombo]->currentIndex());

	const int iAlgo = static_cast<int>(ResoDlg::GetSelectedAlgo());
		mapConf[strXmlRoot + "reso/algo"] = tl::var_to_str<int>(iAlgo);

	mapConf[strXmlRoot + "reso/use_guide"] = groupGuide->isChecked() ? "1" : "0";

	mapConf[strXmlRoot + "reso/comment"] = textComment->toPlainText().toStdString();
	mapConf[strXmlRoot + "meta/timestamp"] = tl::var_to_str<t_real_reso>(tl::epoch<t_real_reso>());
}

void ResoDlg::Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot)
{
	bool bOldDontCalc = m_bDontCalc;
	m_bDontCalc = 1;

	for(std::size_t iSpinBox=0; iSpinBox<m_vecSpinBoxes.size(); ++iSpinBox)
	{
		boost::optional<t_real_reso> odSpinVal = xml.QueryOpt<t_real_reso>(strXmlRoot+m_vecSpinNames[iSpinBox]);
		if(odSpinVal) m_vecSpinBoxes[iSpinBox]->setValue(*odSpinVal);
	}

	for(std::size_t iSpinBox=0; iSpinBox<m_vecIntSpinBoxes.size(); ++iSpinBox)
	{
		boost::optional<int> odSpinVal = xml.QueryOpt<int>(strXmlRoot+m_vecIntSpinNames[iSpinBox]);
		if(odSpinVal) m_vecIntSpinBoxes[iSpinBox]->setValue(*odSpinVal);
	}

	for(std::size_t iEditBox=0; iEditBox<m_vecPosEditBoxes.size(); ++iEditBox)
	{
		boost::optional<t_real_reso> odEditVal = xml.QueryOpt<t_real_reso>(strXmlRoot+m_vecPosEditNames[iEditBox]);
		if(odEditVal) m_vecPosEditBoxes[iEditBox]->setText(tl::var_to_str(*odEditVal, g_iPrec).c_str());
	}

	for(std::size_t iCheck=0; iCheck<m_vecCheckBoxes.size(); ++iCheck)
	{
		boost::optional<int> obChecked = xml.QueryOpt<int>(strXmlRoot+m_vecCheckNames[iCheck]);
		if(obChecked) m_vecCheckBoxes[iCheck]->setChecked(*obChecked);
	}

	for(std::size_t iRadio=0; iRadio<m_vecRadioPlus.size(); ++iRadio)
	{
		boost::optional<int> obChecked = xml.QueryOpt<int>(strXmlRoot+m_vecRadioNames[iRadio]);
		if(obChecked)
		{
			if(*obChecked)
				m_vecRadioPlus[iRadio]->setChecked(1);
			else
				m_vecRadioMinus[iRadio]->setChecked(1);
		}
	}

	for(std::size_t iCombo=0; iCombo<m_vecComboBoxes.size(); ++iCombo)
	{
		boost::optional<int> oiComboIdx = xml.QueryOpt<int>(strXmlRoot+m_vecComboNames[iCombo]);
		if(oiComboIdx) m_vecComboBoxes[iCombo]->setCurrentIndex(*oiComboIdx);
	}

	boost::optional<int> oiAlgo = xml.QueryOpt<int>(strXmlRoot + "reso/algo");
	if(oiAlgo) SetSelectedAlgo(static_cast<ResoAlgo>(*oiAlgo));

	boost::optional<int> obGroupVal = xml.QueryOpt<int>(strXmlRoot+"reso/use_guide");
	if(obGroupVal) groupGuide->setChecked(*obGroupVal);

	textComment->setText(xml.Query<std::string>(strXmlRoot+"reso/comment").c_str());
	t_real_reso dTimestamp = xml.Query<t_real_reso>(strXmlRoot+"meta/timestamp");
	editTimestamp->setText(tl::epoch_to_str(dTimestamp).c_str());

	m_bDontCalc = bOldDontCalc;
	RefreshQEPos();
	//Calc();
}


void ResoDlg::ResoParamsChanged(const ResoParams& params)
{
	//tl::log_debug("reso params changed");

	bool bOldDontCalc = m_bDontCalc;
	m_bDontCalc = 1;

	if(params.bSensesChanged[0]) params.bScatterSenses[0] ? radioMonoScatterPlus->setChecked(1) : radioMonoScatterMinus->setChecked(1);
	if(params.bSensesChanged[1]) params.bScatterSenses[1] ? radioSampleScatterPlus->setChecked(1) : radioSampleScatterMinus->setChecked(1);
	if(params.bSensesChanged[2]) params.bScatterSenses[2] ? radioAnaScatterPlus->setChecked(1) : radioAnaScatterMinus->setChecked(1);

	if(params.bMonoDChanged) spinMonod->setValue(params.dMonoD);
	if(params.bAnaDChanged) spinAnad->setValue(params.dAnaD);

	m_bDontCalc = bOldDontCalc;
	Calc();
}

void ResoDlg::RecipParamsChanged(const RecipParams& parms)
{
	//tl::log_debug("recip params changed");

	bool bOldDontCalc = m_bDontCalc;
	m_bDontCalc = 1;

	try
	{
		m_simpleparams.twotheta = m_tofparams.twotheta = m_tasparams.twotheta =
			units::abs(t_real_reso(parms.d2Theta) * rads);
		//std::cout << parms.dTheta/M_PI*180. << " " << parms.d2Theta/M_PI*180. << std::endl;

		m_simpleparams.ki = m_tofparams.ki = m_tasparams.ki = t_real_reso(parms.dki) / angs;
		m_simpleparams.kf = m_tofparams.kf = m_tasparams.kf = t_real_reso(parms.dkf) / angs;
		m_simpleparams.E = m_tofparams.E = m_tasparams.E = t_real_reso(parms.dE) * meV;

		//m_dAngleQVec0 = parms.dAngleQVec0;
		//std::cout << "qvec0 in rlu: " << m_dAngleQVec0 << std::endl;

		t_vec vecHKL = -tl::make_vec({parms.Q_rlu[0], parms.Q_rlu[1], parms.Q_rlu[2]});
		t_real_reso dQ = parms.dQ;

		if(m_bHasUB)
		{
			if(m_matUB.size1() != vecHKL.size())
				vecHKL.resize(m_matUB.size1(), true);
			t_vec vecQ = ublas::prod(m_matUB, vecHKL);
			vecQ.resize(2,1);
			m_dAngleQVec0 = -tl::vec_angle(vecQ);
			dQ = ublas::norm_2(vecQ);
		}

		m_simpleparams.Q = m_tofparams.Q = m_tasparams.Q = dQ / angs;
		//std::cout << "qvec0 in 1/A: " << m_dAngleQVec0 << std::endl;


		m_simpleparams.angle_ki_Q = m_tofparams.angle_ki_Q = m_tasparams.angle_ki_Q =
			/*M_PI*rads -*/ tl::get_angle_ki_Q(m_tasparams.ki, m_tasparams.kf, m_tasparams.Q, 1);
		m_simpleparams.angle_kf_Q = m_tofparams.angle_kf_Q = m_tasparams.angle_kf_Q =
			/*M_PI*rads -*/ tl::get_angle_kf_Q(m_tasparams.ki, m_tasparams.kf, m_tasparams.Q, 1);
		//m_tasparams.angle_ki_Q = units::abs(m_tasparams.angle_ki_Q);
		//m_tasparams.angle_kf_Q = units::abs(m_tasparams.angle_kf_Q);

		/*std::cout << "ki = " << t_real_reso(m_tasparams.ki*angs) << std::endl;
		std::cout << "kf = " << t_real_reso(m_tasparams.kf*angs) << std::endl;
		std::cout << "Q = " << t_real_reso(m_tasparams.Q*angs) << std::endl;

		std::cout << "kiQ = " << t_real_reso(m_tasparams.angle_ki_Q/M_PI/rads * 180.) << std::endl;
		std::cout << "kfQ = " << t_real_reso(m_tasparams.angle_kf_Q/M_PI/rads * 180.) << std::endl;*/


		editQ->setText(tl::var_to_str(dQ, g_iPrec).c_str());
		editE->setText(tl::var_to_str(parms.dE, g_iPrec).c_str());
		editKi->setText(tl::var_to_str(parms.dki, g_iPrec).c_str());
		editKf->setText(tl::var_to_str(parms.dkf, g_iPrec).c_str());
	}
	catch(const std::exception& ex)
	{
		tl::log_err("Cannot set reciprocal parameters for resolution: ", ex.what(), ".");
	}

	m_bDontCalc = bOldDontCalc;
	if(m_bUpdateOnRecipEvent)
		Calc();
}

void ResoDlg::RealParamsChanged(const RealParams& parms)
{
	//tl::log_debug("real params changed");

	bool bOldDontCalc = m_bDontCalc;
	m_bDontCalc = 1;

	m_tasparams.thetam = units::abs(t_real_reso(parms.dMonoT) * rads);
	m_tasparams.thetaa = units::abs(t_real_reso(parms.dAnaT) * rads);

	m_tasparams.twotheta = t_real_reso(parms.dSampleTT) * rads;
	m_tasparams.twotheta = units::abs(m_tasparams.twotheta);
	m_simpleparams.twotheta = m_tofparams.twotheta = m_tasparams.twotheta;

	//std::cout << parms.dMonoT/M_PI*180. << ", " << parms.dAnaT/M_PI*180. << std::endl;
	//std::cout << parms.dSampleTT/M_PI*180. << std::endl;

	m_bDontCalc = bOldDontCalc;
	if(m_bUpdateOnRealEvent)
		Calc();
}

void ResoDlg::SampleParamsChanged(const SampleParams& parms)
{
	try
	{
		//tl::log_debug("sample params changed");

		tl::Lattice<t_real_reso> lattice(parms.dLattice[0],parms.dLattice[1],parms.dLattice[2],
			parms.dAngles[0],parms.dAngles[1],parms.dAngles[2]);

		m_vecOrient1 = tl::make_vec<t_vec>({parms.dPlane1[0], parms.dPlane1[1], parms.dPlane1[2]});
		m_vecOrient2 = tl::make_vec<t_vec>({parms.dPlane2[0], parms.dPlane2[1], parms.dPlane2[2]});
		//m_vecOrient1 /= ublas::norm_2(m_vecOrient1);
		//m_vecOrient2 /= ublas::norm_2(m_vecOrient2);

		m_matB = tl::get_B(lattice, 1);
		m_matU = tl::get_U(m_vecOrient1, m_vecOrient2, &m_matB);
		m_matUrlu = tl::get_U(m_vecOrient1, m_vecOrient2);
		m_matUB = ublas::prod(m_matU, m_matB);

		bool bHasB = tl::inverse(m_matB, m_matBinv);
		bool bHasU = tl::inverse(m_matU, m_matUinv);
		bool bHasUrlu = tl::inverse(m_matUrlu, m_matUinvrlu);
		m_matUBinv = ublas::prod(m_matBinv, m_matUinv);

		for(auto* pmat : {&m_matB, &m_matU, &m_matUB, &m_matUBinv, &m_matUrlu, &m_matUinvrlu})
		{
			pmat->resize(4,4,1);
			(*pmat)(3,0) = (*pmat)(3,1) = (*pmat)(3,2) = 0.;
			(*pmat)(0,3) = (*pmat)(1,3) = (*pmat)(2,3) = 0.;
			(*pmat)(3,3) = 1.;
		}

		m_bHasUB = bHasB && bHasU && bHasUrlu;
	}
	catch(const std::exception& ex)
	{
		m_bHasUB = 0;
		tl::log_err("Cannot set sample parameters for resolution: ", ex.what(), ".");
	}
}


// --------------------------------------------------------------------------------
// Monte-Carlo stuff

void ResoDlg::checkAutoCalcElli4dChanged()
{
	if(checkElli4dAutoCalc->isChecked() && !m_bEll4dCurrent)
		CalcElli4d();
}

void ResoDlg::CalcElli4d()
{
	m_ell4d = calc_res_ellipsoid4d<t_real_reso>(
		m_res.reso, m_res.reso_v, m_res.reso_s, m_res.Q_avg);

	std::ostringstream ostrElli;
	ostrElli << "<html><body>\n";

	ostrElli << "<p><b>Ellipsoid volume:</b> " << m_ell4d.vol << "</p>\n\n";

	ostrElli << "<p><b>Ellipsoid offsets:</b>\n"
		<< "\t<ul><li>Qx = " << m_ell4d.x_offs << "</li>\n"
		<< "\t<li>Qy = " << m_ell4d.y_offs << "</li>\n"
		<< "\t<li>Qz = " << m_ell4d.z_offs << "</li>\n"
		<< "\t<li>E = " << m_ell4d.w_offs << "</li></ul></p>\n\n";

	ostrElli << "<p><b>Ellipsoid HWHMs (unsorted):</b>\n"
		<< "\t<ul><li>" << m_ell4d.x_hwhm << "</li>\n"
		<< "\t<li>" << m_ell4d.y_hwhm << "</li>\n"
		<< "\t<li>" << m_ell4d.z_hwhm << "</li>\n"
		<< "\t<li>" << m_ell4d.w_hwhm << "</li></ul></p>\n\n";

	ostrElli << "</body></html>\n";

	editElli->setHtml(QString::fromUtf8(ostrElli.str().c_str()));
}


void ResoDlg::MCGenerate()
{
	if(!m_bEll4dCurrent)
		CalcElli4d();

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strLastDir = m_pSettings ? m_pSettings->value("reso/mc_dir", ".").toString() : ".";
	QString _strFile = QFileDialog::getSaveFileName(this, "Save MC neutron data...",
		strLastDir, "DAT files (*.dat);;All files (*.*)", nullptr, fileopt);
	if(_strFile == "")
		return;

	std::string strFile = _strFile.toStdString();

	const int iNeutrons = spinMCNeutrons->value();
	const bool bCenter = checkMCCenter->isChecked();

	std::ofstream ofstr(strFile);
	if(!ofstr.is_open())
	{
		QMessageBox::critical(this, "Error", "Cannot open file.");
		return;
	}

	std::vector<t_vec> vecNeutrons;
	McNeutronOpts<t_mat> opts;
	opts.bCenter = bCenter;
	opts.coords = McNeutronCoords(comboMCCoords->currentIndex());
	opts.matU = m_matU;
	opts.matB = m_matB;
	opts.matUB = m_matUB;
	opts.matUinv = m_matUinv;
	opts.matBinv = m_matBinv;
	opts.matUBinv = m_matUBinv;

	t_mat* pMats[] = {&opts.matU, &opts.matB, &opts.matUB,
		&opts.matUinv, &opts.matBinv, &opts.matUBinv};

	for(t_mat *pMat : pMats)
	{
		pMat->resize(4,4,1);

		for(int i0=0; i0<3; ++i0)
			(*pMat)(i0,3) = (*pMat)(3,i0) = 0.;
		(*pMat)(3,3) = 1.;
	}


	opts.dAngleQVec0 = m_dAngleQVec0;
	vecNeutrons.resize(iNeutrons);
	mc_neutrons(m_ell4d, iNeutrons, opts, vecNeutrons.begin());


	ofstr.precision(g_iPrec);

	if(opts.coords == McNeutronCoords::DIRECT)
	{
		ofstr << "# coord_sys: direct\n";
		ofstr << "# " << std::setw(std::max<int>(g_iPrec*2-2, 4)) << m_ell4d.x_lab << " "
			<< std::setw(g_iPrec*2) << m_ell4d.y_lab << " "
			<< std::setw(g_iPrec*2) << m_ell4d.z_lab << " "
			<< std::setw(g_iPrec*2) << m_ell4d.w_lab << " \n";
	}
	else if(opts.coords == McNeutronCoords::ANGS)
	{
		ofstr << "# coord_sys: angstrom\n";
		ofstr << "# " << std::setw(std::max<int>(g_iPrec*2-2, 4)) << "Qx (1/A) "
			<< std::setw(g_iPrec*2) << "Qy (1/A) "
			<< std::setw(g_iPrec*2) << "Qz (1/A) "
			<< std::setw(g_iPrec*2) << "E (meV) " << "\n";
	}
	else if(opts.coords == McNeutronCoords::RLU)
	{
		ofstr << "# coord_sys: rlu\n";
		ofstr << "# " << std::setw(std::max<int>(g_iPrec*2-2, 4)) << "h (rlu) "
			<< std::setw(g_iPrec*2) << "k (rlu) "
			<< std::setw(g_iPrec*2) << "l (rlu) "
			<< std::setw(g_iPrec*2) << "E (meV) " << "\n";
	}
	else
	{
		ofstr << "# coord_sys: unknown\n";
	}


	for(const t_vec& vecNeutron : vecNeutrons)
	{
		for(unsigned i=0; i<4; ++i)
			ofstr << std::setw(g_iPrec*2) << vecNeutron[i] << " ";
		ofstr << "\n";
	}

	if(m_pSettings)
		m_pSettings->setValue("reso/mc_dir", QString(tl::get_dir(strFile).c_str()));
}


// --------------------------------------------------------------------------------


void ResoDlg::AlgoChanged()
{
	std::string strAlgo = "<html><body>\n";

	switch(GetSelectedAlgo())
	{
		case ResoAlgo::CN:
		{
			tabWidget->setTabEnabled(0,1);
			tabWidget->setTabEnabled(1,0);
			tabWidget->setTabEnabled(2,0);
			tabWidget->setTabEnabled(3,0);
			tabWidget->setTabEnabled(4,0);
			strAlgo = "<b>M. J. Cooper and <br>R. Nathans</b><br>\n";
			strAlgo += "<a href=http://dx.doi.org/10.1107/S0365110X67002816>"
				"Acta Cryst. 23, <br>pp. 357-367</a><br>\n";
			strAlgo += "1967";
			break;
		}
		case ResoAlgo::POP:
		{
			tabWidget->setTabEnabled(0,1);
			tabWidget->setTabEnabled(1,1);
			tabWidget->setTabEnabled(2,0);
			tabWidget->setTabEnabled(3,0);
			tabWidget->setTabEnabled(4,0);
			strAlgo = "<b>M. Popovici</b><br>\n";
			strAlgo += "<a href=http://dx.doi.org/10.1107/S0567739475001088>"
				"Acta Cryst. A 31, <br>pp. 507-513</a><br>\n";
			strAlgo += "1975";
			break;
		}
		case ResoAlgo::ECK:
		{
			tabWidget->setTabEnabled(0,1);
			tabWidget->setTabEnabled(1,1);
			tabWidget->setTabEnabled(2,1);
			tabWidget->setTabEnabled(3,0);
			tabWidget->setTabEnabled(4,0);
			strAlgo = "<b>G. Eckold and <br>O. Sobolev</b><br>\n";
			strAlgo += "<a href=http://dx.doi.org/10.1016/j.nima.2014.03.019>"
				"NIM A 752, <br>pp. 54-64</a><br>\n";
			strAlgo += "2014";
			break;
		}
		case ResoAlgo::VIOL:
		{
			tabWidget->setTabEnabled(0,0);
			tabWidget->setTabEnabled(1,0);
			tabWidget->setTabEnabled(2,0);
			tabWidget->setTabEnabled(3,1);
			tabWidget->setTabEnabled(4,0);
			strAlgo = "<b>N. Violini et al.</b><br>\n";
			strAlgo += "<a href=http://dx.doi.org/10.1016/j.nima.2013.10.042>"
				"NIM A 736, <br>pp. 31-39</a><br>\n";
			strAlgo += "2014";
			break;
		}
		case ResoAlgo::SIMPLE:
		{
			tabWidget->setTabEnabled(0,0);
			tabWidget->setTabEnabled(1,0);
			tabWidget->setTabEnabled(2,0);
			tabWidget->setTabEnabled(3,0);
			tabWidget->setTabEnabled(4,1);
			strAlgo = "<b>Simple</b><br>\n";
			break;
		}
		default:
		{
			strAlgo += "<i>unknown</i>";
			break;
		}
	}

	strAlgo += "\n</body></html>\n";
	labelAlgoRef->setText(strAlgo.c_str());
	labelAlgoRef->setOpenExternalLinks(1);
}


// --------------------------------------------------------------------------------


void ResoDlg::ButtonBoxClicked(QAbstractButton* pBtn)
{
	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ApplyRole ||
	   buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		WriteLastConfig();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::RejectRole)
	{
		reject();
	}

	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		QDialog::accept();
	}
}

void ResoDlg::hideEvent(QHideEvent *event)
{
	if(m_pSettings)
		m_pSettings->setValue("reso/wnd_geo", saveGeometry());
}

void ResoDlg::showEvent(QShowEvent *event)
{
	if(m_pSettings)
		restoreGeometry(m_pSettings->value("reso/wnd_geo").toByteArray());
}

#include "ResoDlg.moc"
