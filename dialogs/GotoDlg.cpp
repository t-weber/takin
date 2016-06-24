/*
 * Goto Dialog
 * @author Tobias Weber
 * @date 15-oct-2014
 * @license GPLv2
 */

#include "GotoDlg.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/string/string.h"
#include "tlibs/string/spec_char.h"

#include <QFileDialog>
#include <QMessageBox>

using t_real = t_real_glob;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_energy_si<t_real> meV = tl::get_one_meV<t_real>();
static const tl::t_angle_si<t_real> rads = tl::get_one_radian<t_real>();


GotoDlg::GotoDlg(QWidget* pParent, QSettings* pSett) : QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnAdd->setIcon(load_icon("res/list-add.svg"));
	btnDel->setIcon(load_icon("res/list-remove.svg"));
	btnSave->setIcon(load_icon("res/document-save.svg"));
	btnLoad->setIcon(load_icon("res/document-open.svg"));

	std::vector<QLineEdit*> vecObjs {editH, editK, editL};
	std::vector<QLineEdit*> vecAngles {edit2ThetaM, editThetaM, edit2ThetaA, editThetaA, edit2ThetaS, editThetaS};

#if QT_VER >= 5
	QObject::connect(buttonBox, &QDialogButtonBox::clicked, this, &GotoDlg::ButtonBoxClicked);

	QObject::connect(btnAdd, &QAbstractButton::clicked, this, static_cast<void(GotoDlg::*)()>(&GotoDlg::AddPosToList));
	QObject::connect(btnDel, &QAbstractButton::clicked, this, &GotoDlg::RemPosFromList);
	QObject::connect(btnLoad, &QAbstractButton::clicked, this, &GotoDlg::LoadList);
	QObject::connect(btnSave, &QAbstractButton::clicked, this, &GotoDlg::SaveList);
	QObject::connect(listSeq, &QListWidget::itemSelectionChanged, this, &GotoDlg::ListItemSelected);
	QObject::connect(listSeq, &QListWidget::itemDoubleClicked, this, &GotoDlg::ListItemDoubleClicked);

	QObject::connect(editKi, &QLineEdit::textEdited, this, &GotoDlg::EditedKiKf);
	QObject::connect(editKf, &QLineEdit::textEdited, this, &GotoDlg::EditedKiKf);
	QObject::connect(editE, &QLineEdit::textEdited, this, &GotoDlg::EditedE);
	QObject::connect(btnGetPos, &QAbstractButton::clicked, this, &GotoDlg::GetCurPos);

	for(QLineEdit* pObj : vecObjs)
		QObject::connect(pObj, &QLineEdit::textEdited, this, &GotoDlg::CalcSample);
	for(QLineEdit* pObj : vecAngles)
		QObject::connect(pObj, &QLineEdit::textEdited, this, &GotoDlg::EditedAngles);
#else
	QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(ButtonBoxClicked(QAbstractButton*)));

	QObject::connect(btnAdd, SIGNAL(clicked()), this, SLOT(AddPosToList()));
	QObject::connect(btnDel, SIGNAL(clicked()), this, SLOT(RemPosFromList()));
	QObject::connect(btnLoad, SIGNAL(clicked()), this, SLOT(LoadList()));
	QObject::connect(btnSave, SIGNAL(clicked()), this, SLOT(SaveList()));
	QObject::connect(listSeq, SIGNAL(itemSelectionChanged()), this, SLOT(ListItemSelected()));
	QObject::connect(listSeq, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
		this, SLOT(ListItemDoubleClicked(QListWidgetItem*)));

	QObject::connect(editKi, SIGNAL(textEdited(const QString&)), this, SLOT(EditedKiKf()));
	QObject::connect(editKf, SIGNAL(textEdited(const QString&)), this, SLOT(EditedKiKf()));
	QObject::connect(editE, SIGNAL(textEdited(const QString&)), this, SLOT(EditedE()));
	QObject::connect(btnGetPos, SIGNAL(clicked()), this, SLOT(GetCurPos()));

	for(QObject* pObj : vecObjs)
		QObject::connect(pObj, SIGNAL(textEdited(const QString&)), this, SLOT(CalcSample()));
	for(QObject* pObj : vecAngles)
		QObject::connect(pObj, SIGNAL(textEdited(const QString&)), this, SLOT(EditedAngles()));
#endif

	if(m_pSettings)
	{
		if(m_pSettings->contains("goto_pos/geo"))
			restoreGeometry(m_pSettings->value("goto_pos/geo").toByteArray());
		if(m_pSettings->contains("goto_pos/ki_fix"))
			radioFixedKi->setChecked(m_pSettings->value("goto_pos/ki_fix").toBool());
	}
}

GotoDlg::~GotoDlg()
{
	ClearList();
}


void GotoDlg::CalcSample()
{
	m_bSampleOk = 0;

	std::vector<QLineEdit*> vecEdits{editThetaS, edit2ThetaS};
	for(QLineEdit* pEdit : vecEdits)
		pEdit->setText("");

	bool bHOk=0, bKOk=0, bLOk=0, bKiOk=0, bKfOk=0;

	t_real dH = editH->text().toDouble(&bHOk);
	t_real dK = editK->text().toDouble(&bKOk);
	t_real dL = editL->text().toDouble(&bLOk);
	t_real dKi = editKi->text().toDouble(&bKiOk);
	t_real dKf = editKf->text().toDouble(&bKfOk);

	if(!bHOk || !bKOk || !bLOk || !bKiOk || !bKfOk)
		return;

	ublas::vector<t_real> vecQ;
	bool bFailed = 0;
	try
	{
		tl::get_tas_angles(m_lattice,
			m_vec1, m_vec2,
			dKi, dKf,
			dH, dK, dL,
			m_bSenseS,
			&m_dSampleTheta, &m_dSample2Theta,
			&vecQ);

		if(tl::is_nan_or_inf<t_real>(m_dSample2Theta))
			throw tl::Err("Invalid sample 2theta.");
		if(tl::is_nan_or_inf<t_real>(m_dSampleTheta))
			throw tl::Err("Invalid sample theta.");
	}
	catch(const std::exception& ex)
	{
		editThetaS->setText("invalid");
		edit2ThetaS->setText("invalid");

		//log_err(ex.what());
		labelStatus->setText((std::string("Error: ") + ex.what()).c_str());
		bFailed = 1;
	}

	tl::set_eps_0(vecQ, g_dEps);
	tl::set_eps_0(m_dSample2Theta, g_dEps);
	tl::set_eps_0(m_dSampleTheta, g_dEps);

	const std::wstring strAA = tl::get_spec_char_utf16("AA") + tl::get_spec_char_utf16("sup-") + tl::get_spec_char_utf16("sup1");

	std::wostringstream ostrStatus;
	ostrStatus << "Q = " << vecQ << " " << strAA << ", |Q| = " << ublas::norm_2(vecQ) << " " << strAA;
	labelQ->setText(QString::fromWCharArray(ostrStatus.str().c_str()));

#ifndef NDEBUG
	try
	{
		ublas::vector<t_real> vecQ0 = vecQ;
		vecQ0.resize(2, true);
		t_real dAngleQVec0 = tl::r2d(tl::vec_angle(vecQ0));
		tl::log_info("Angle Q Orient0: ", dAngleQVec0, " deg.");
	}
	catch(const std::exception& ex)
	{}
#endif

	if(bFailed) return;

	editThetaS->setText(tl::var_to_str(tl::r2d(m_dSampleTheta), g_iPrec).c_str());
	edit2ThetaS->setText(tl::var_to_str(tl::r2d(m_dSample2Theta), g_iPrec).c_str());

	m_bSampleOk = 1;

	if(m_bMonoAnaOk && m_bSampleOk)
		labelStatus->setText("Position OK.");
}

void GotoDlg::CalcMonoAna()
{
	m_bMonoAnaOk = 0;

	std::vector<QLineEdit*> vecEdits{editThetaM, edit2ThetaM, editThetaA, edit2ThetaA};
	for(QLineEdit* pEdit : vecEdits)
		pEdit->setText("");

	bool bKiOk=0, bKfOk=0;
	t_real dKi = editKi->text().toDouble(&bKiOk);
	t_real dKf = editKf->text().toDouble(&bKfOk);
	if(!bKiOk || !bKfOk) return;

	tl::t_wavenumber_si<t_real> ki = dKi / angs;
	tl::t_wavenumber_si<t_real> kf = dKf / angs;

	bool bMonoOk = 0;
	bool bAnaOk = 0;

	try
	{
		m_dMono2Theta = tl::get_mono_twotheta(ki, m_dMono*angs, m_bSenseM) / rads;
		if(tl::is_nan_or_inf<t_real>(m_dMono2Theta))
			throw tl::Err("Invalid monochromator angle.");

		tl::set_eps_0(m_dMono2Theta, g_dEps);
		bMonoOk = 1;
	}
	catch(const std::exception& ex)
	{
		editThetaM->setText("invalid");
		edit2ThetaM->setText("invalid");

		//log_err(ex.what());
		labelStatus->setText((std::string("Error: ") + ex.what()).c_str());
	}

	try
	{
		m_dAna2Theta = tl::get_mono_twotheta(kf, m_dAna*angs, m_bSenseA) / rads;
		if(tl::is_nan_or_inf<t_real>(m_dAna2Theta))
			throw tl::Err("Invalid analysator angle.");

		tl::set_eps_0(m_dAna2Theta, g_dEps);
		bAnaOk = 1;
	}
	catch(const std::exception& ex)
	{
		editThetaA->setText("invalid");
		edit2ThetaA->setText("invalid");

		//log_err(ex.what());
		labelStatus->setText((std::string("Error: ") + ex.what()).c_str());
	}

	if(!bMonoOk || !bAnaOk)
		return;


	t_real dTMono = m_dMono2Theta / 2.;
	t_real dTAna = m_dAna2Theta / 2.;

	edit2ThetaM->setText(tl::var_to_str(tl::r2d(m_dMono2Theta), g_iPrec).c_str());
	editThetaM->setText(tl::var_to_str(tl::r2d(dTMono), g_iPrec).c_str());
	edit2ThetaA->setText(tl::var_to_str(tl::r2d(m_dAna2Theta), g_iPrec).c_str());
	editThetaA->setText(tl::var_to_str(tl::r2d(dTAna), g_iPrec).c_str());

	m_bMonoAnaOk = 1;

	if(m_bMonoAnaOk && m_bSampleOk)
		labelStatus->setText("Position OK.");
}

void GotoDlg::EditedKiKf()
{
	bool bKiOk=0, bKfOk=0;
	t_real dKi = editKi->text().toDouble(&bKiOk);
	t_real dKf = editKf->text().toDouble(&bKfOk);
	if(!bKiOk || !bKfOk) return;

	tl::t_energy_si<t_real> Ei = tl::k2E(dKi / angs);
	tl::t_energy_si<t_real> Ef = tl::k2E(dKf / angs);

	tl::t_energy_si<t_real> E = Ei-Ef;
	t_real dE = E/meV;
	tl::set_eps_0(dE, g_dEps);

	std::string strE = tl::var_to_str<t_real>(dE, g_iPrec);
	editE->setText(strE.c_str());

	CalcMonoAna();
	CalcSample();
}

void GotoDlg::EditedE()
{
	bool bOk = 0;
	t_real dE = editE->text().toDouble(&bOk);
	if(!bOk) return;
	tl::t_energy_si<t_real> E = dE * meV;

	bool bImag=0;
	tl::t_wavenumber_si<t_real> k_E = tl::E2k(E, bImag);
	t_real dSign = 1.;
	if(bImag) dSign = -1.;

	if(radioFixedKi->isChecked())
	{
		bool bKOk = 0;
		t_real dKi = editKi->text().toDouble(&bKOk);
		if(!bKOk) return;

		tl::t_wavenumber_si<t_real> ki = dKi / angs;
		tl::t_wavenumber_si<t_real> kf =
			tl::my_units_sqrt<tl::t_wavenumber_si<t_real>>(ki*ki - dSign*k_E*k_E);

		t_real dKf = kf*angs;
		tl::set_eps_0(dKf, g_dEps);

		std::string strKf = tl::var_to_str<t_real>(dKf, g_iPrec);
		editKf->setText(strKf.c_str());
	}
	else
	{
		bool bKOk = 0;
		t_real dKf = editKf->text().toDouble(&bKOk);
		if(!bKOk) return;

		tl::t_wavenumber_si<t_real> kf = dKf / angs;
		tl::t_wavenumber_si<t_real> ki =
			tl::my_units_sqrt<tl::t_wavenumber_si<t_real>>(kf*kf + dSign*k_E*k_E);

		t_real dKi = ki*angs;
		tl::set_eps_0(dKi, g_dEps);

		std::string strKi = tl::var_to_str<t_real>(dKi, g_iPrec);
		editKi->setText(strKi.c_str());
	}

	CalcMonoAna();
	CalcSample();
}

// calc. tas angles -> hkl
void GotoDlg::EditedAngles()
{
	bool bthmOk;
	t_real th_m = tl::d2r(edit2ThetaM->text().toDouble(&bthmOk)/2.);
	tl::set_eps_0(th_m, g_dEps);
	if(bthmOk)
		editThetaM->setText(tl::var_to_str<t_real>(tl::r2d(th_m), g_iPrec).c_str());

	bool bthaOk;
	t_real th_a = tl::d2r(edit2ThetaA->text().toDouble(&bthaOk)/2.);
	tl::set_eps_0(th_a, g_dEps);
	if(bthaOk)
		editThetaA->setText(tl::var_to_str<t_real>(tl::r2d(th_a), g_iPrec).c_str());

	bool bthsOk, bttsOk;
	t_real th_s = tl::d2r(editThetaS->text().toDouble(&bthsOk));
	t_real tt_s = tl::d2r(edit2ThetaS->text().toDouble(&bttsOk));

	if(!bthmOk || !bthaOk || !bthsOk || !bttsOk)
		return;


	t_real h,k,l;
	t_real dKi, dKf, dE;
	ublas::vector<t_real> vecQ;
	bool bFailed = 0;
	try
	{
		tl::get_hkl_from_tas_angles<t_real>(m_lattice,
			m_vec1, m_vec2,
			m_dMono, m_dAna,
			th_m, th_a, th_s, tt_s,
			m_bSenseM, m_bSenseA, m_bSenseS,
			&h, &k, &l,
			&dKi, &dKf, &dE, 0,
			&vecQ);

		if(tl::is_nan_or_inf<t_real>(h) || tl::is_nan_or_inf<t_real>(k) || tl::is_nan_or_inf<t_real>(l))
			throw tl::Err("Invalid hkl.");
	}
	catch(const std::exception& ex)
	{
		//log_err(ex.what());
		labelStatus->setText((std::string("Error: ") + ex.what()).c_str());
		bFailed = 1;
	}

	const std::wstring strAA = tl::get_spec_char_utf16("AA") + tl::get_spec_char_utf16("sup-") + tl::get_spec_char_utf16("sup1");
	std::wostringstream ostrStatus;
	ostrStatus << "Q = " << vecQ << " " << strAA << ", |Q| = " << ublas::norm_2(vecQ) << " " << strAA;
	labelQ->setText(QString::fromWCharArray(ostrStatus.str().c_str()));

	if(bFailed) return;

	for(t_real* d : {&h,&k,&l, &dKi,&dKf,&dE})
		tl::set_eps_0(*d, g_dEps);

	editH->setText(tl::var_to_str<t_real>(h, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k, g_iPrec).c_str());
	editL->setText(tl::var_to_str<t_real>(l, g_iPrec).c_str());

	editKi->setText(tl::var_to_str<t_real>(dKi, g_iPrec).c_str());
	editKf->setText(tl::var_to_str<t_real>(dKf, g_iPrec).c_str());
	editE->setText(tl::var_to_str<t_real>(dE, g_iPrec).c_str());

	m_dMono2Theta = th_m*2.;
	m_dAna2Theta = th_a*2.;
	m_dSample2Theta = tt_s;
	m_dSampleTheta = th_s;
	m_bMonoAnaOk = 1;
	m_bSampleOk = 1;

	if(m_bMonoAnaOk && m_bSampleOk)
		labelStatus->setText("Position OK.");
}

void GotoDlg::GetCurPos()
{
	if(!m_bHasParamsRecip)
		return;

	tl::set_eps_0(m_paramsRecip.dki, g_dEps);
	tl::set_eps_0(m_paramsRecip.dkf, g_dEps);
	tl::set_eps_0(m_paramsRecip.Q_rlu[0], g_dEps);
	tl::set_eps_0(m_paramsRecip.Q_rlu[1], g_dEps);
	tl::set_eps_0(m_paramsRecip.Q_rlu[2], g_dEps);

	editKi->setText(tl::var_to_str(m_paramsRecip.dki, g_iPrec).c_str());
	editKf->setText(tl::var_to_str(m_paramsRecip.dkf, g_iPrec).c_str());

	editH->setText(tl::var_to_str(-m_paramsRecip.Q_rlu[0], g_iPrec).c_str());
	editK->setText(tl::var_to_str(-m_paramsRecip.Q_rlu[1], g_iPrec).c_str());
	editL->setText(tl::var_to_str(-m_paramsRecip.Q_rlu[2], g_iPrec).c_str());

	EditedKiKf();
	CalcMonoAna();
	CalcSample();
}

void GotoDlg::RecipParamsChanged(const RecipParams& params)
{
	m_bHasParamsRecip = 1;
	m_paramsRecip = params;
}

void GotoDlg::ButtonBoxClicked(QAbstractButton* pBtn)
{
	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::ApplyRole ||
	   buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		ApplyCurPos();
	}
	else if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::RejectRole)
	{
		reject();
	}

	if(buttonBox->buttonRole(pBtn) == QDialogButtonBox::AcceptRole)
	{
		if(m_pSettings)
		{
			m_pSettings->setValue("goto_pos/geo", saveGeometry());
			m_pSettings->setValue("goto_pos/ki_fix", radioFixedKi->isChecked());
		}

		QDialog::accept();
	}
}

void GotoDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


//------------------------------------------------------------------------------

struct HklPos
{
	t_real dh, dk, dl;
	t_real dki, dkf;
	t_real dE;
};

bool GotoDlg::ApplyCurPos()
{
	if(!m_bMonoAnaOk || !m_bSampleOk)
		return false;

	CrystalOptions crys;
	TriangleOptions triag;

	triag.bChangedMonoTwoTheta = 1;
	triag.dMonoTwoTheta = this->m_dMono2Theta;

	triag.bChangedAnaTwoTheta = 1;
	triag.dAnaTwoTheta = this->m_dAna2Theta;

	triag.bChangedTheta = 1;
	triag.dTheta = this->m_dSampleTheta;

	triag.bChangedAngleKiVec0 = 1;

	t_real dSampleTheta = m_dSampleTheta;
	if(!m_bSenseS) dSampleTheta = -dSampleTheta;
	triag.dAngleKiVec0 = tl::get_pi<t_real>()/2. - dSampleTheta;
	//log_info("kivec0 = ", triag.dAngleKiVec0/M_PI*180.);
	//log_info("th = ", m_dSampleTheta/M_PI*180.);

	triag.bChangedTwoTheta = 1;
	triag.dTwoTheta = this->m_dSample2Theta;
	// TODO: correct hack in taz.cpp
	if(!m_bSenseS) triag.dTwoTheta = -triag.dTwoTheta;

	emit vars_changed(crys, triag);
	return true;
}

bool GotoDlg::GotoPos(QListWidgetItem* pItem, bool bApply)
{
	if(!pItem) return false;
	HklPos* pPos = (HklPos*)pItem->data(Qt::UserRole).value<void*>();
	if(!pPos) return false;

	editH->setText(tl::var_to_str(pPos->dh, g_iPrec).c_str());
	editK->setText(tl::var_to_str(pPos->dk, g_iPrec).c_str());
	editL->setText(tl::var_to_str(pPos->dl, g_iPrec).c_str());
	editKi->setText(tl::var_to_str(pPos->dki, g_iPrec).c_str());
	editKf->setText(tl::var_to_str(pPos->dkf, g_iPrec).c_str());

	EditedKiKf();
	CalcSample();

	if(bApply)
		return ApplyCurPos();
	return m_bMonoAnaOk && m_bSampleOk;
}

bool GotoDlg::GotoPos(unsigned int iItem)
{
	if(int(iItem) >= listSeq->count())
		return false;
	return GotoPos(listSeq->item(int(iItem)), 1);
}

void GotoDlg::ListItemSelected()
{
	QListWidgetItem *pItem = listSeq->currentItem();
	GotoPos(pItem, 0);
}

void GotoDlg::ListItemDoubleClicked(QListWidgetItem* pItem)
{
	bool bOk = GotoPos(pItem, 1);
	if(!bOk)
		QMessageBox::critical(this, "Error", "Invalid position.");
}

void GotoDlg::AddPosToList(t_real dh, t_real dk, t_real dl, t_real dki, t_real dkf)
{
	HklPos *pPos = new HklPos;

	pPos->dh = dh;
	pPos->dk = dk;
	pPos->dl = dl;
	pPos->dki = dki;
	pPos->dkf = dkf;
	pPos->dE = (tl::k2E(pPos->dki/angs) - tl::k2E(pPos->dkf/angs))/tl::meV;

	tl::set_eps_0(pPos->dh, g_dEps);
	tl::set_eps_0(pPos->dk, g_dEps);
	tl::set_eps_0(pPos->dl, g_dEps);
	tl::set_eps_0(pPos->dki, g_dEps);
	tl::set_eps_0(pPos->dkf, g_dEps);
	tl::set_eps_0(pPos->dE, g_dEps);

	const std::wstring strAA = tl::get_spec_char_utf16("AA") + tl::get_spec_char_utf16("sup-") + tl::get_spec_char_utf16("sup1");

	std::wostringstream ostrHKL;
	ostrHKL.precision(4);
	ostrHKL << "(" << pPos->dh << ", " << pPos->dk << ", " << pPos->dl << ") rlu\n";
	ostrHKL << "ki = " << pPos->dki << " " << strAA;
	ostrHKL << ", kf = " << pPos->dkf << " " << strAA << "\n";
	ostrHKL << "E = " << pPos->dE << " meV";

	QString qstr = QString::fromWCharArray(ostrHKL.str().c_str());
	QListWidgetItem* pItem = new QListWidgetItem(qstr, listSeq);
	pItem->setData(Qt::UserRole, QVariant::fromValue<void*>(pPos));
}

void GotoDlg::AddPosToList()
{
	t_real dh = tl::str_to_var<t_real>(editH->text().toStdString());
	t_real dk = tl::str_to_var<t_real>(editK->text().toStdString());
	t_real dl = tl::str_to_var<t_real>(editL->text().toStdString());
	t_real dki = tl::str_to_var<t_real>(editKi->text().toStdString());
	t_real dkf = tl::str_to_var<t_real>(editKf->text().toStdString());

	AddPosToList(dh, dk, dl, dki, dkf);
}

void GotoDlg::RemPosFromList()
{
	QListWidgetItem *pItem = listSeq->currentItem();
	if(pItem)
	{
		HklPos* pPos = (HklPos*)pItem->data(Qt::UserRole).value<void*>();
		if(pPos) delete pPos;
		delete pItem;
	}
}

void GotoDlg::ClearList()
{
	while(listSeq->count())
	{
		QListWidgetItem *pItem = listSeq->item(0);
		if(!pItem) break;

		HklPos* pPos = (HklPos*)pItem->data(Qt::UserRole).value<void*>();
		if(pPos) delete pPos;
		delete pItem;
	}
}


void GotoDlg::LoadList()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		strDirLast = m_pSettings->value("goto_pos/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getOpenFileName(this,
		"Load Positions", strDirLast,
		"TAZ files (*.taz *.TAZ)", nullptr,
		fileopt);
	if(qstrFile == "")
		return;


	std::string strFile = qstrFile.toStdString();
	std::string strDir = tl::get_dir(strFile);

	tl::Prop<std::string> xml;
	if(!xml.Load(strFile.c_str(), tl::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not load positions.");
		return;
	}

	Load(xml, strXmlRoot);
	if(m_pSettings)
		m_pSettings->setValue("goto_pos/last_dir", QString(strDir.c_str()));
}

void GotoDlg::SaveList()
{
	const std::string strXmlRoot("taz/");

	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSettings && !m_pSettings->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSettings)
		m_pSettings->value("goto_pos/last_dir", ".").toString();
	QString qstrFile = QFileDialog::getSaveFileName(this,
		"Save Positions", strDirLast,
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
	bool bOk = xml.Save(strFile.c_str(), tl::PropType::XML);
	if(!bOk)
		QMessageBox::critical(this, "Error", "Could not save positions.");

	if(bOk && m_pSettings)
		m_pSettings->setValue("goto_pos/last_dir", QString(strDir.c_str()));
}

void GotoDlg::Save(std::map<std::string, std::string>& mapConf, const std::string& strXmlRoot)
{
	mapConf[strXmlRoot + "goto_pos/h"] = editH->text().toStdString();
	mapConf[strXmlRoot + "goto_pos/k"] = editK->text().toStdString();
	mapConf[strXmlRoot + "goto_pos/l"] = editL->text().toStdString();
	mapConf[strXmlRoot + "goto_pos/ki"] = editKi->text().toStdString();
	mapConf[strXmlRoot + "goto_pos/kf"] = editKf->text().toStdString();
	mapConf[strXmlRoot + "goto_pos/cki"] = radioFixedKi->isChecked()?"1":"0";

	// favlist
	for(int iItem=0; iItem<listSeq->count(); ++iItem)
	{
		const QListWidgetItem *pItem = listSeq->item(iItem);
		if(!pItem) continue;
		const HklPos* pPos = (HklPos*)pItem->data(Qt::UserRole).value<void*>();
		if(!pPos) continue;

		std::ostringstream ostrItemBase;
		ostrItemBase << "goto_favlist/pos_" << iItem << "/";
		std::string strItemBase = ostrItemBase.str();

		mapConf[strXmlRoot + strItemBase + "h"] = tl::var_to_str(pPos->dh);
		mapConf[strXmlRoot + strItemBase + "k"] = tl::var_to_str(pPos->dk);
		mapConf[strXmlRoot + strItemBase + "l"] = tl::var_to_str(pPos->dl);
		mapConf[strXmlRoot + strItemBase + "ki"] = tl::var_to_str(pPos->dki);
		mapConf[strXmlRoot + strItemBase + "kf"] = tl::var_to_str(pPos->dkf);
	}
}

void GotoDlg::Load(tl::Prop<std::string>& xml, const std::string& strXmlRoot)
{
	bool bOk=0;

	editH->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "goto_pos/h").c_str(), 1., &bOk), g_iPrec).c_str());
	editK->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "goto_pos/k").c_str(), 0., &bOk), g_iPrec).c_str());
	editL->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "goto_pos/l").c_str(), 0., &bOk), g_iPrec).c_str());
	editKi->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "goto_pos/ki").c_str(), 1.4, &bOk), g_iPrec).c_str());
	editKf->setText(tl::var_to_str(xml.Query<t_real>((strXmlRoot + "goto_pos/kf").c_str(), 1.4, &bOk), g_iPrec).c_str());
	radioFixedKi->setChecked(xml.Query<bool>((strXmlRoot + "goto_pos/cki").c_str(), 0, &bOk));

	// favlist
	ClearList();
	unsigned int iItem=0;
	while(1)
	{
		std::ostringstream ostrItemBase;
		ostrItemBase << "goto_favlist/pos_" << iItem << "/";
		std::string strItemBase = ostrItemBase.str();

		if(!xml.Exists((strXmlRoot + strItemBase).c_str()))
			break;

		t_real dh = xml.Query<t_real>((strXmlRoot + strItemBase + "h").c_str(), 0., &bOk);
		t_real dk = xml.Query<t_real>((strXmlRoot + strItemBase + "k").c_str(), 0., &bOk);
		t_real dl = xml.Query<t_real>((strXmlRoot + strItemBase + "l").c_str(), 0., &bOk);
		t_real dki = xml.Query<t_real>((strXmlRoot + strItemBase + "ki").c_str(), 0., &bOk);
		t_real dkf = xml.Query<t_real>((strXmlRoot + strItemBase + "kf").c_str(), 0., &bOk);

		AddPosToList(dh, dk, dl, dki, dkf);
		++iItem;
	}

	EditedKiKf();
	CalcSample();
}


#include "GotoDlg.moc"
