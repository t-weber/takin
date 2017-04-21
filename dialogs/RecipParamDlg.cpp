/**
 * Reciprocal Space Parameters
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 26-mar-2014
 * @license GPLv2
 */

#include "RecipParamDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/math/linalg.h"
#include "tlibs/phys/neutrons.h"

namespace ublas = boost::numeric::ublas;
using t_real = t_real_glob;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_energy_si<t_real> meV = tl::get_one_meV<t_real>();
static const tl::t_time_si<t_real> sec = tl::get_one_second<t_real>();
static const tl::t_length_si<t_real> meter = tl::get_one_meter<t_real>();


RecipParamDlg::RecipParamDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

#if QT_VER >= 5
	QObject::connect(editKi, &QLineEdit::textChanged, this, &RecipParamDlg::KiChanged);
	QObject::connect(editKf, &QLineEdit::textChanged, this, &RecipParamDlg::KfChanged);

	QObject::connect(btnUseG, &QAbstractButton::clicked, this, &RecipParamDlg::SetGOrigin);
	QObject::connect(editOriginH, &QLineEdit::textEdited, this, &RecipParamDlg::OriginChanged);
	QObject::connect(editOriginK, &QLineEdit::textEdited, this, &RecipParamDlg::OriginChanged);
	QObject::connect(editOriginL, &QLineEdit::textEdited, this, &RecipParamDlg::OriginChanged);
	QObject::connect(radioEm, &QRadioButton::toggled, this, &RecipParamDlg::OriginChanged);
#else
	QObject::connect(editKi, SIGNAL(textChanged(const QString&)), this, SLOT(KiChanged()));
	QObject::connect(editKf, SIGNAL(textChanged(const QString&)), this, SLOT(KfChanged()));

	QObject::connect(btnUseG, SIGNAL(clicked()), this, SLOT(SetGOrigin()));
	QObject::connect(editOriginH, SIGNAL(textEdited(const QString&)), this, SLOT(OriginChanged()));
	QObject::connect(editOriginK, SIGNAL(textEdited(const QString&)), this, SLOT(OriginChanged()));
	QObject::connect(editOriginL, SIGNAL(textEdited(const QString&)), this, SLOT(OriginChanged()));
	QObject::connect(radioEm, SIGNAL(toggled(bool)), this, SLOT(OriginChanged()));
#endif

	if(m_pSettings)
	{
		if(m_pSettings->contains("recip_params/geo"))
			restoreGeometry(m_pSettings->value("recip_params/geo").toByteArray());
		if(m_pSettings->contains("recip_params/defs_Em"))
			radioEm->setChecked(m_pSettings->value("recip_params/defs_Em").toBool());
	}
}

RecipParamDlg::~RecipParamDlg()
{}

void RecipParamDlg::paramsChanged(const RecipParams& parms)
{
	m_params = parms;

	t_real dQ = m_params.dQ;
	if(m_params.d2Theta < 0.)
		dQ = -dQ;

	t_real dq_rlu = std::sqrt(m_params.q_rlu[0]*m_params.q_rlu[0]
		+ m_params.q_rlu[1]*m_params.q_rlu[1]
		+ m_params.q_rlu[2]*m_params.q_rlu[2]);

	this->editKi->setText(tl::var_to_str<t_real>(m_params.dki, g_iPrec).c_str());
	this->editKf->setText(tl::var_to_str<t_real>(m_params.dkf, g_iPrec).c_str());
	this->editQ->setText(tl::var_to_str<t_real>(dQ, g_iPrec).c_str());
	this->editq->setText(tl::var_to_str<t_real>(m_params.dq, g_iPrec).c_str());
	this->editqrlu->setText(tl::var_to_str<t_real>(dq_rlu, g_iPrec).c_str());
	this->editE->setText(tl::var_to_str<t_real>(m_params.dE, g_iPrec).c_str());
	this->edit2Theta->setText(tl::var_to_str<t_real>(tl::r2d(m_params.d2Theta), g_iPrec).c_str());
	this->editTheta->setText(tl::var_to_str<t_real>(tl::r2d(m_params.dTheta), g_iPrec).c_str());
	this->editKiQ->setText(tl::var_to_str<t_real>(tl::r2d(m_params.dKiQ), g_iPrec).c_str());
	this->editKfQ->setText(tl::var_to_str<t_real>(tl::r2d(m_params.dKfQ), g_iPrec).c_str());

	this->editQx->setText(tl::var_to_str<t_real>(-m_params.Q[0], g_iPrec).c_str());
	this->editQy->setText(tl::var_to_str<t_real>(-m_params.Q[1], g_iPrec).c_str());
	this->editQz->setText(tl::var_to_str<t_real>(-m_params.Q[2], g_iPrec).c_str());
	this->editQxrlu->setText(tl::var_to_str<t_real>(-m_params.Q_rlu[0], g_iPrec).c_str());
	this->editQyrlu->setText(tl::var_to_str<t_real>(-m_params.Q_rlu[1], g_iPrec).c_str());
	this->editQzrlu->setText(tl::var_to_str<t_real>(-m_params.Q_rlu[2], g_iPrec).c_str());

	this->editqx->setText(tl::var_to_str<t_real>(-m_params.q[0], g_iPrec).c_str());
	this->editqy->setText(tl::var_to_str<t_real>(-m_params.q[1], g_iPrec).c_str());
	this->editqz->setText(tl::var_to_str<t_real>(-m_params.q[2], g_iPrec).c_str());
	this->editqxrlu->setText(tl::var_to_str<t_real>(-m_params.q_rlu[0], g_iPrec).c_str());
	this->editqyrlu->setText(tl::var_to_str<t_real>(-m_params.q_rlu[1], g_iPrec).c_str());
	this->editqzrlu->setText(tl::var_to_str<t_real>(-m_params.q_rlu[2], g_iPrec).c_str());

	this->editGx->setText(tl::var_to_str<t_real>(-m_params.G[0], g_iPrec).c_str());
	this->editGy->setText(tl::var_to_str<t_real>(-m_params.G[1], g_iPrec).c_str());
	this->editGz->setText(tl::var_to_str<t_real>(-m_params.G[2], g_iPrec).c_str());
	this->editGxrlu->setText(tl::var_to_str<t_real>(-m_params.G_rlu[0], g_iPrec).c_str());
	this->editGyrlu->setText(tl::var_to_str<t_real>(-m_params.G_rlu[1], g_iPrec).c_str());
	this->editGzrlu->setText(tl::var_to_str<t_real>(-m_params.G_rlu[2], g_iPrec).c_str());


	// focus
	ublas::vector<t_real> vecG = tl::make_vec({-m_params.G_rlu[0], -m_params.G_rlu[1], -m_params.G_rlu[2]});
	ublas::vector<t_real> vecUp = tl::make_vec({m_params.orient_up[0], m_params.orient_up[1], m_params.orient_up[2]});
	ublas::vector<t_real> vecFm = tl::cross_3(vecG, vecUp);
	vecFm /= ublas::norm_2(vecFm);
	tl::set_eps_0(vecFm, g_dEps);

	this->editFmx->setText(tl::var_to_str<t_real>(vecFm[0], g_iPrec).c_str());
	this->editFmy->setText(tl::var_to_str<t_real>(vecFm[1], g_iPrec).c_str());
	this->editFmz->setText(tl::var_to_str<t_real>(vecFm[2], g_iPrec).c_str());

	this->editFpx->setText(tl::var_to_str<t_real>(-vecFm[0], g_iPrec).c_str());
	this->editFpy->setText(tl::var_to_str<t_real>(-vecFm[1], g_iPrec).c_str());
	this->editFpz->setText(tl::var_to_str<t_real>(-vecFm[2], g_iPrec).c_str());


	/*if(editOriginH->text().trimmed()=="" || editOriginK->text().trimmed()=="" || editOriginL->text().trimmed()=="")
		SetGOrigin();*/
	OriginChanged();
}


void RecipParamDlg::KiChanged()
{
	tl::t_wavenumber_si<t_real> ki = tl::str_to_var<t_real>(editKi->text().toStdString()) / angs;
	tl::t_energy_si<t_real> Ei = tl::k2E(ki);
	tl::t_length_si<t_real> lami = tl::k2lam(ki);
	tl::t_velocity_si<t_real> vi = tl::k2v(ki);

	editEi->setText(tl::var_to_str<t_real>(Ei / meV, g_iPrec).c_str());
	editLami->setText(tl::var_to_str<t_real>(lami / angs, g_iPrec).c_str());
	editVi->setText(tl::var_to_str<t_real>(vi*sec/meter, g_iPrec).c_str());
}

void RecipParamDlg::KfChanged()
{
	tl::t_wavenumber_si<t_real> kf = tl::str_to_var<t_real>(editKf->text().toStdString()) / angs;
	tl::t_energy_si<t_real> Ef = tl::k2E(kf);
	tl::t_length_si<t_real> lamf = tl::k2lam(kf);
	tl::t_velocity_si<t_real> vf = tl::k2v(kf);

	editEf->setText(tl::var_to_str<t_real>(Ef / meV, g_iPrec).c_str());
	editLamf->setText(tl::var_to_str<t_real>(lamf / angs, g_iPrec).c_str());
	editVf->setText(tl::var_to_str<t_real>(vf*sec/meter, g_iPrec).c_str());
}


void RecipParamDlg::SetGOrigin()
{
	editOriginH->setText(editGxrlu->text());
	editOriginK->setText(editGyrlu->text());
	editOriginL->setText(editGzrlu->text());

	OriginChanged();
}

void RecipParamDlg::OriginChanged()
{
	const bool bEm = radioEm->isChecked();
	const t_real dH = tl::str_to_var<t_real>(editOriginH->text().toStdString());
	const t_real dK = tl::str_to_var<t_real>(editOriginK->text().toStdString());
	const t_real dL = tl::str_to_var<t_real>(editOriginL->text().toStdString());
	const t_real dSq = dH*dH + dK*dK + dL*dL;

	const ublas::vector<t_real> vecQ = tl::make_vec({dH, dK, dL});
	ublas::vector<t_real> vecUp = tl::make_vec({m_params.orient_up[0], m_params.orient_up[1], m_params.orient_up[2]});
	ublas::vector<t_real> vecFm = tl::cross_3(vecQ, vecUp);
	vecUp /= ublas::norm_2(vecUp);
	vecFm /= ublas::norm_2(vecFm);

	if(!bEm) vecFm = -vecFm;
	tl::set_eps_0(vecUp, g_dEps);
	tl::set_eps_0(vecFm, g_dEps);


	std::ostringstream ostrDefs;
	ostrDefs << "import numpy as np\n\n\n";
	ostrDefs << "origin = np.array([" << dH << ", " << dK << ", " << dL << "])\n\n";

	ostrDefs << "longdir = origin";
	if(!tl::float_equal<t_real>(dSq, 1.))
		ostrDefs << " / np.sqrt(" << dSq << ")";
	ostrDefs << "\n";

	ostrDefs << "transdir = np.array([" << vecFm[0] << ", " << vecFm[1] << ", " << vecFm[2] << "])\n";
	ostrDefs << "updir = np.array([" << vecUp[0] << ", " << vecUp[1] << ", " << vecUp[2] << "])\n\n\n";

	ostrDefs << "# define some arbitrary direction in the scattering plane here:\n";
	ostrDefs << "# userangle = 45. / 180. * np.pi\n";
	ostrDefs << "# userdir = longdir*np.cos(userangle) + transdir*np.sin(userangle)\n";

	editDefs->setText(ostrDefs.str().c_str());
}


void RecipParamDlg::closeEvent(QCloseEvent *pEvt)
{
	QDialog::closeEvent(pEvt);
}

void RecipParamDlg::accept()
{
	if(m_pSettings)
	{
		m_pSettings->setValue("recip_params/defs_Em", radioEm->isChecked());
		m_pSettings->setValue("recip_params/geo", saveGeometry());
	}

	QDialog::accept();
}

void RecipParamDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "RecipParamDlg.moc"
