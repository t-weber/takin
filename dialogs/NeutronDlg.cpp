/**
 * Neutron Properties Dialog
 * @author Tobias Weber
 * @date jul-2013, 28-may-2014
 * @license GPLv2
 */

#include "NeutronDlg.h"
#include <boost/units/io.hpp>

#include "tlibs/string/string.h"
#include "tlibs/string/eval.h"
#include "tlibs/math/math.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/helper/misc.h"

#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using t_real = t_real_glob;
namespace co = boost::units::si::constants::codata;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_energy_si<t_real> meV = tl::get_one_meV<t_real>();
static const tl::t_angle_si<t_real> rads = tl::get_one_radian<t_real>();
static const tl::t_temperature_si<t_real> kelvin = tl::get_one_kelvin<t_real>();
static const tl::t_time_si<t_real> ps = tl::get_one_picosecond<t_real>();
static const tl::t_time_si<t_real> sec = tl::get_one_second<t_real>();
static const tl::t_length_si<t_real> meter = tl::get_one_meter<t_real>();
static const tl::t_flux_si<t_real> tesla = tl::get_one_tesla<t_real>();
static const tl::t_flux_si<t_real> millitesla = tesla*t_real(1e-3);
static const tl::t_flux_si<t_real> kilogauss = tl::get_one_kilogauss<t_real>();


NeutronDlg::NeutronDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	setupConstants();

	QObject::connect(editLam, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronLam()));
	QObject::connect(editE, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronE()));
	QObject::connect(editOm, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronOm()));
	QObject::connect(editF, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronF()));
	QObject::connect(editV, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronv()));
	QObject::connect(editK, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronk()));
	QObject::connect(editT, SIGNAL(textEdited(const QString&)), this, SLOT(CalcNeutronT()));

	QObject::connect(btnSyncKi, SIGNAL(clicked()), this, SLOT(SetExtKi()));
	QObject::connect(btnSyncKf, SIGNAL(clicked()), this, SLOT(SetExtKf()));

	CalcNeutronLam();



	std::vector<QLineEdit*> editsDir = {editBraggDirN, editBraggDirLam, editBraggDirD, editBraggDirT, editBraggDirTT};
	std::vector<QLineEdit*> editsReci = {editBraggReciN, editBraggReciLam, editBraggReciQ, editBraggReciT, editBraggReciTT};
	std::vector<QRadioButton*> radioDir = {/*radioBraggDirN,*/ radioBraggDirLam, radioBraggDirD, radioBraggDirTT};
	std::vector<QRadioButton*> radioReci = {/*radioBraggReciN,*/ radioBraggReciLam, radioBraggReciQ, radioBraggReciTT};

	QObject::connect(editBraggDirT, SIGNAL(textEdited(const QString&)), this, SLOT(RealThetaEdited()));
	QObject::connect(editBraggReciT, SIGNAL(textEdited(const QString&)), this, SLOT(RecipThetaEdited()));
	QObject::connect(editBraggDirTT, SIGNAL(textEdited(const QString&)), this, SLOT(RealTwoThetaEdited()));
	QObject::connect(editBraggReciTT, SIGNAL(textEdited(const QString&)), this, SLOT(RecipTwoThetaEdited()));

	for(QLineEdit* pEdit : editsDir)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcBraggReal()));
	for(QLineEdit* pEdit : editsReci)
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcBraggRecip()));
	for(QRadioButton* pRadio : radioDir)
	{
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(EnableRealEdits()));
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(CalcBraggReal()));
	}
	for(QRadioButton* pRadio : radioReci)
	{
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(EnableRecipEdits()));
		QObject::connect(pRadio, SIGNAL(toggled(bool)), this, SLOT(CalcBraggRecip()));
	}
	QObject::connect(editEval, SIGNAL(textEdited(const QString&)), this, SLOT(Eval(const QString&)));


	EnableRealEdits();
	EnableRecipEdits();
	CalcBraggReal();
	CalcBraggRecip();


	if(m_pSettings)
	{
		if(m_pSettings->contains("neutron_props/geo"))
			restoreGeometry(m_pSettings->value("neutron_props/geo").toByteArray());

		bool bOk = 0;
		t_real dLam = m_pSettings->value("neutron_props/lam").toDouble(&bOk);
		if(!bOk)
			dLam = 5.;

		std::string strLam = tl::var_to_str(dLam, g_iPrec);
		editLam->setText(strLam.c_str());

		CalcNeutronLam();
	}
}

NeutronDlg::~NeutronDlg()
{}


// -----------------------------------------------------------------------------


void NeutronDlg::CalcNeutronLam()
{
	std::string strInput = editLam->text().toStdString();

	tl::t_length_si<t_real> lam_n = tl::str_to_var<t_real>(strInput) * angs;

	tl::t_wavenumber_si<t_real> k_n = tl::lam2k(lam_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);
	tl::t_energy_si<t_real> E_n = p_n*p_n / (t_real(2.)*tl::get_m_n<t_real>());

	editE->setText(tl::var_to_str<t_real>(E_n / meV, g_iPrec).c_str());
	editOm->setText(tl::var_to_str<t_real>(E_n / tl::get_hbar<t_real>() * ps, g_iPrec).c_str());
	editF->setText(tl::var_to_str<t_real>(E_n / tl::get_h<t_real>() * ps, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k_n * angs, g_iPrec).c_str());
	editV->setText(tl::var_to_str<t_real>((p_n / tl::get_m_n<t_real>()) * sec / meter, g_iPrec).c_str());
	editT->setText(tl::var_to_str<t_real>((E_n / tl::get_kB<t_real>()) / kelvin, g_iPrec).c_str());
}

void NeutronDlg::CalcNeutronk()
{
	std::string strInput = editK->text().toStdString();

	tl::t_wavenumber_si<t_real> k_n = tl::str_to_var<t_real>(strInput) / angs;
	tl::t_length_si<t_real> lam_n = tl::k2lam(k_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);
	tl::t_energy_si<t_real> E_n = p_n*p_n / (t_real(2.)*tl::get_m_n<t_real>());

	editLam->setText(tl::var_to_str<t_real>(lam_n / angs, g_iPrec).c_str());
	editE->setText(tl::var_to_str<t_real>(E_n / meV, g_iPrec).c_str());
	editOm->setText(tl::var_to_str<t_real>(E_n / tl::get_hbar<t_real>() * ps, g_iPrec).c_str());
	editF->setText(tl::var_to_str<t_real>(E_n / tl::get_h<t_real>() * ps, g_iPrec).c_str());
	editV->setText(tl::var_to_str<t_real>((p_n / tl::get_m_n<t_real>()) * sec / meter, g_iPrec).c_str());
	editT->setText(tl::var_to_str<t_real>((E_n / tl::get_kB<t_real>()) / kelvin, g_iPrec).c_str());
}

void NeutronDlg::CalcNeutronv()
{
	std::string strInput = editV->text().toStdString();

	tl::t_velocity_si<t_real> v_n = tl::str_to_var<t_real>(strInput) * meter / sec;
	tl::t_wavenumber_si<t_real> k_n = tl::v2k(v_n);
	tl::t_length_si<t_real> lam_n = tl::k2lam(k_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);
	tl::t_energy_si<t_real> E_n = p_n*p_n / (t_real(2.)*tl::get_m_n<t_real>());

	editLam->setText(tl::var_to_str<t_real>(lam_n / angs, g_iPrec).c_str());
	editE->setText(tl::var_to_str<t_real>(E_n / meV, g_iPrec).c_str());
	editOm->setText(tl::var_to_str<t_real>(E_n / tl::get_hbar<t_real>() * ps, g_iPrec).c_str());
	editF->setText(tl::var_to_str<t_real>(E_n / tl::get_h<t_real>() * ps, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k_n * angs, g_iPrec).c_str());
	editT->setText(tl::var_to_str<t_real>((E_n / tl::get_kB<t_real>()) / kelvin, g_iPrec).c_str());
}

void NeutronDlg::CalcNeutronE()
{
	std::string strInput = editE->text().toStdString();

	bool bImag = 0;
	tl::t_energy_si<t_real> E_n = tl::str_to_var<t_real>(strInput) * meV;
	tl::t_wavenumber_si<t_real> k_n = tl::E2k(E_n, bImag);
	tl::t_length_si<t_real> lam_n = tl::k2lam(k_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);

	editOm->setText(tl::var_to_str<t_real>(E_n / tl::get_hbar<t_real>() * ps, g_iPrec).c_str());
	editF->setText(tl::var_to_str<t_real>(E_n / tl::get_h<t_real>() * ps, g_iPrec).c_str());
	editLam->setText(tl::var_to_str<t_real>(lam_n / angs, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k_n * angs, g_iPrec).c_str());
	editV->setText(tl::var_to_str<t_real>((p_n / tl::get_m_n<t_real>()) * sec / meter, g_iPrec).c_str());
	editT->setText(tl::var_to_str<t_real>((E_n / tl::get_kB<t_real>()) / kelvin, g_iPrec).c_str());
}

void NeutronDlg::CalcNeutronOm()
{
	std::string strInput = editOm->text().toStdString();

	bool bImag = 0;
	tl::t_energy_si<t_real> E_n = tl::str_to_var<t_real>(strInput) / ps * tl::get_hbar<t_real>();
	tl::t_wavenumber_si<t_real> k_n = tl::E2k(E_n, bImag);
	tl::t_length_si<t_real> lam_n = tl::k2lam(k_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);

	editE->setText(tl::var_to_str<t_real>(E_n / meV, g_iPrec).c_str());
	editF->setText(tl::var_to_str<t_real>(E_n / tl::get_h<t_real>() * ps, g_iPrec).c_str());
	editLam->setText(tl::var_to_str<t_real>(lam_n / angs, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k_n * angs, g_iPrec).c_str());
	editV->setText(tl::var_to_str<t_real>((p_n / tl::get_m_n<t_real>()) * sec / meter, g_iPrec).c_str());
	editT->setText(tl::var_to_str<t_real>((E_n / tl::get_kB<t_real>()) / kelvin, g_iPrec).c_str());
}

void NeutronDlg::CalcNeutronF()
{
	std::string strInput = editF->text().toStdString();

	bool bImag = 0;
	tl::t_energy_si<t_real> E_n = tl::str_to_var<t_real>(strInput) / ps * tl::get_h<t_real>();
	tl::t_wavenumber_si<t_real> k_n = tl::E2k(E_n, bImag);
	tl::t_length_si<t_real> lam_n = tl::k2lam(k_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);

	editE->setText(tl::var_to_str<t_real>(E_n / meV, g_iPrec).c_str());
	editOm->setText(tl::var_to_str<t_real>(E_n / tl::get_hbar<t_real>() * ps, g_iPrec).c_str());
	editLam->setText(tl::var_to_str<t_real>(lam_n / angs, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k_n * angs, g_iPrec).c_str());
	editV->setText(tl::var_to_str<t_real>((p_n / tl::get_m_n<t_real>()) * sec / meter, g_iPrec).c_str());
	editT->setText(tl::var_to_str<t_real>((E_n / tl::get_kB<t_real>()) / kelvin, g_iPrec).c_str());
}

void NeutronDlg::CalcNeutronT()
{
	std::string strInput = editT->text().toStdString();

	bool bImag;
	tl::t_temperature_si<t_real> T_n = tl::str_to_var<t_real>(strInput) * kelvin;
	tl::t_energy_si<t_real> E_n = T_n * tl::get_kB<t_real>();
	tl::t_wavenumber_si<t_real> k_n = tl::E2k(E_n, bImag);
	tl::t_length_si<t_real> lam_n = tl::k2lam(k_n);
	tl::t_momentum_si<t_real> p_n = tl::lam2p(lam_n);

	editLam->setText(tl::var_to_str<t_real>(lam_n / angs, g_iPrec).c_str());
	editK->setText(tl::var_to_str<t_real>(k_n * angs, g_iPrec).c_str());
	editV->setText(tl::var_to_str<t_real>((p_n / tl::get_m_n<t_real>()) * sec / meter, g_iPrec).c_str());
	editE->setText(tl::var_to_str<t_real>(E_n / meV, g_iPrec).c_str());
	editOm->setText(tl::var_to_str<t_real>(E_n / tl::get_hbar<t_real>() * ps, g_iPrec).c_str());
	editF->setText(tl::var_to_str<t_real>(E_n / tl::get_h<t_real>() * ps, g_iPrec).c_str());
}


void NeutronDlg::paramsChanged(const RecipParams& parms)
{
	m_dExtKi = parms.dki;
	m_dExtKf = parms.dkf;
}

void NeutronDlg::SetExtKi()
{
	editK->setText(tl::var_to_str<t_real>(m_dExtKi, g_iPrec).c_str());
	CalcNeutronk();
}

void NeutronDlg::SetExtKf()
{
	editK->setText(tl::var_to_str<t_real>(m_dExtKf, g_iPrec).c_str());
	CalcNeutronk();
}


// -----------------------------------------------------------------------------


void NeutronDlg::setupConstants()
{
	// -------------------------------------------------------------------------
	// constants
	struct Constant
	{
		std::string strSymbol;
		std::string strName;

		std::string strVal;
	};

	std::vector<Constant> vecConsts;

	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::one_eV;

		Constant constant;
		constant.strSymbol = "eV";
		constant.strName = "1 electron volt";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::get_h<t_real>();

		Constant constant;
		constant.strSymbol = "h";
		constant.strName = "Planck constant";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << (tl::get_h<t_real>() / tl::one_eV) << " eV";

		Constant constant;
		constant.strSymbol = "h";
		constant.strName = "Planck constant";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::get_hbar<t_real>();

		Constant constant;
		constant.strSymbol = "hbar";
		constant.strName = "Planck constant";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << (tl::get_hbar<t_real>() / tl::one_eV) << " eV";

		Constant constant;
		constant.strSymbol = "hbar";
		constant.strName = "Planck constant";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}

	// neutron
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::get_m_n<t_real>();

		Constant constant;
		constant.strSymbol = "m_n";
		constant.strName = "Neutron mass";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << t_real(tl::get_g_n<t_real>());

		Constant constant;
		constant.strSymbol = "g_n";
		constant.strName = "Neutron g factor";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << co::gamma_n;		// TODO: replace with a tl::... getter

		Constant constant;
		constant.strSymbol = "gamma_n";
		constant.strName = "Neutron gyromagnetic ratio";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		//ostrVal << tl::get_mu_n<t_real>();
		ostrVal << t_real(tl::get_mu_n<t_real>() / meV * tesla) << " meV/T";

		Constant constant;
		constant.strSymbol = "mu_n";
		constant.strName = "Neutron magnetic moment";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		//ostrVal << co::mu_N;
		ostrVal << t_real(tl::get_mu_N<t_real>() / meV * tesla) << " meV/T";

		Constant constant;
		constant.strSymbol = "mu_N";
		constant.strName = "Nuclear magneton";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}



	// electron
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::get_m_e<t_real>();

		Constant constant;
		constant.strSymbol = "m_e";
		constant.strName = "Electron mass";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << t_real(tl::get_g_e<t_real>());

		Constant constant;
		constant.strSymbol = "g_e";
		constant.strName = "Electron g factor";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << co::gamma_e;		// TODO: replace with a tl::... getter

		Constant constant;
		constant.strSymbol = "gamma_e";
		constant.strName = "Electron gyromagnetic ratio";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		//ostrVal << tl::get_mu_n<t_real>();
		ostrVal << t_real(tl::get_mu_e<t_real>() / meV * tesla) << " meV/T";

		Constant constant;
		constant.strSymbol = "mu_e";
		constant.strName = "Electron magnetic moment";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << t_real(tl::get_muB<t_real>() / meV * tesla) << " meV/T";

		Constant constant;
		constant.strSymbol = "mu_B";
		constant.strName = "Bohr magneton";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}



	{
		std::ostringstream ostrVal;
		//ostrVal << std::scientific;
		ostrVal << t_real(-tl::get_g_e<t_real>() * tl::get_muB<t_real>() / meV * tesla) << " meV/T";

		Constant constant;
		constant.strSymbol = "-g_e * mu_B";
		constant.strName = "Zeeman shift";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::get_c<t_real>();

		Constant constant;
		constant.strSymbol = "c";
		constant.strName = "Vacuum speed of light";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << tl::get_kB<t_real>();

		Constant constant;
		constant.strSymbol = "k_B";
		constant.strName = "Boltzmann constant";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	{
		std::ostringstream ostrVal;
		ostrVal << std::scientific;
		ostrVal << t_real(tl::get_kB<t_real>() / meV * kelvin) << " meV/K";

		Constant constant;
		constant.strSymbol = "k_B";
		constant.strName = "Boltzmann constant";
		constant.strVal = tl::insert_before<std::string>(ostrVal.str(), "(", "\n");

		vecConsts.push_back(std::move(constant));
	}
	// -------------------------------------------------------------------------



	// -------------------------------------------------------------------------
	// conversions
	struct Conversion
	{
		std::string strName;
		std::string strVal;
	};

	std::vector<Conversion> vecConvs;

	{
		Conversion conv;
		conv.strName = "k^2 in A^(-2)  ->  E in meV";
		conv.strVal = tl::var_to_str(tl::get_KSQ2E<t_real>(), g_iPrec)
			+ " meV per A^(-2)";

		vecConvs.emplace_back(std::move(conv));
	}
	{
		Conversion conv;
		conv.strName = "E in meV  ->  k^2 in A^(-2)";
		conv.strVal = tl::var_to_str(tl::get_E2KSQ<t_real>(), g_iPrec)
			+ " A^(-2) per meV";

		vecConvs.emplace_back(std::move(conv));
	}

	{
		t_real tConv = meV / tl::get_h<t_real>() * ps;

		Conversion conv;
		conv.strName = "E in meV  ->  f in THz";
		conv.strVal = tl::var_to_str(tConv, g_iPrec) + " THz per meV";

		vecConvs.emplace_back(std::move(conv));
	}
	{
		t_real tConv = t_real(1)/(meV / tl::get_h<t_real>() * ps);

		Conversion conv;
		conv.strName = "f in THz  ->  E in meV";
		conv.strVal = tl::var_to_str(tConv, g_iPrec) + " meV per THz";

		vecConvs.emplace_back(std::move(conv));
	}

	{
		Conversion conv;
		conv.strName = "B in kG  ->  B in mT";
		conv.strVal = tl::var_to_str(t_real(kilogauss/millitesla), g_iPrec) + " mT per kG";

		vecConvs.emplace_back(std::move(conv));
	}
	{
		Conversion conv;
		conv.strName = "B in mT  ->  B in kG";
		conv.strVal = tl::var_to_str(t_real(millitesla/kilogauss), g_iPrec) + " kG per mT";

		vecConvs.emplace_back(std::move(conv));
	}
	// -------------------------------------------------------------------------



	// -------------------------------------------------------------------------
	// constants table
	const bool bSortConst = tableConst->isSortingEnabled();
	tableConst->setSortingEnabled(0);

	tableConst->setColumnCount(3);
	tableConst->setRowCount(vecConsts.size());
	tableConst->setColumnWidth(2, 200);
	//tableConst->verticalHeader()->setDefaultSectionSize(tableConst->verticalHeader()->minimumSectionSize()+2);

	for(unsigned int iConst=0; iConst<vecConsts.size(); ++iConst)
	{
		const Constant& constant = vecConsts[iConst];

		QTableWidgetItem *pConstSym = new QTableWidgetItem();
		pConstSym->setText(constant.strSymbol.c_str());
		tableConst->setItem(iConst,0,pConstSym);

		QTableWidgetItem *pConstName = new QTableWidgetItem();
		pConstName->setText(constant.strName.c_str());
		tableConst->setItem(iConst,1,pConstName);

		QTableWidgetItem *pConstVal = new QTableWidgetItem();
		pConstVal->setText(constant.strVal.c_str());
		tableConst->setItem(iConst,2,pConstVal);

		pConstName->setFlags(pConstName->flags() & ~Qt::ItemIsEditable);
		//pConstVal->setFlags(pConstVal->flags() & ~Qt::ItemIsEditable);
	}

	tableConst->setSortingEnabled(bSortConst);
	// -------------------------------------------------------------------------


	// -------------------------------------------------------------------------
	// conversion table
	const bool bSortConv = tableConv->isSortingEnabled();
	tableConv->setSortingEnabled(0);

	tableConv->setColumnCount(2);
	tableConv->setRowCount(vecConvs.size());
	tableConv->setColumnWidth(0, 200);
	tableConv->setColumnWidth(1, 200);
	//tableConv->verticalHeader()->setDefaultSectionSize(tableConv->verticalHeader()->minimumSectionSize()+2);

	for(unsigned int iConst=0; iConst<vecConvs.size(); ++iConst)
	{
		const Conversion& conv = vecConvs[iConst];

		QTableWidgetItem *pConstName = new QTableWidgetItem();
		pConstName->setText(conv.strName.c_str());
		tableConv->setItem(iConst, 0, pConstName);

		QTableWidgetItem *pConstVal = new QTableWidgetItem();
		pConstVal->setText(conv.strVal.c_str());
		tableConv->setItem(iConst, 1, pConstVal);

		pConstName->setFlags(pConstName->flags() & ~Qt::ItemIsEditable);
		//pConstVal->setFlags(pConstVal->flags() & ~Qt::ItemIsEditable);
	}

	tableConv->setSortingEnabled(bSortConv);
	// -------------------------------------------------------------------------
}


// -----------------------------------------------------------------------------

void NeutronDlg::EnableRealEdits()
{
	//void (QLineEdit::*pFunc)(bool) = &QLineEdit::setDisabled;
	void (QLineEdit::*pFunc)(bool) = &QLineEdit::setReadOnly;

	(editBraggDirLam->*pFunc)(0);
	(editBraggDirD->*pFunc)(0);
	(editBraggDirT->*pFunc)(0);
	(editBraggDirTT->*pFunc)(0);

	if(radioBraggDirLam->isChecked())
		(editBraggDirLam->*pFunc)(1);
	else if(radioBraggDirD->isChecked())
		(editBraggDirD->*pFunc)(1);
	else if(radioBraggDirTT->isChecked())
	{
		(editBraggDirT->*pFunc)(1);
		(editBraggDirTT->*pFunc)(1);
	}
}

void NeutronDlg::EnableRecipEdits()
{
	//void (QLineEdit::*pFunc)(bool) = &QLineEdit::setDisabled;
	void (QLineEdit::*pFunc)(bool) = &QLineEdit::setReadOnly;

	(editBraggReciLam->*pFunc)(0);
	(editBraggReciQ->*pFunc)(0);
	(editBraggReciT->*pFunc)(0);
	(editBraggReciTT->*pFunc)(0);

	if(radioBraggReciLam->isChecked())
		(editBraggReciLam->*pFunc)(1);
	else if(radioBraggReciQ->isChecked())
		(editBraggReciQ->*pFunc)(1);
	else if(radioBraggReciTT->isChecked())
	{
		(editBraggReciT->*pFunc)(1);
		(editBraggReciTT->*pFunc)(1);
	}
}


void NeutronDlg::SetEditTT(QLineEdit *pEditT, QLineEdit *pEditTT)
{
	std::string strT = pEditT->text().toStdString();
	t_real dT = tl::str_to_var<t_real>(strT);
	std::string strTT = tl::var_to_str(dT*2., g_iPrec);
	pEditTT->setText(strTT.c_str());
}

void NeutronDlg::RealThetaEdited() { SetEditTT(editBraggDirT, editBraggDirTT); }
void NeutronDlg::RecipThetaEdited() { SetEditTT(editBraggReciT, editBraggReciTT); }


void NeutronDlg::SetEditT(QLineEdit *pEditT, QLineEdit *pEditTT)
{
	std::string strTT = pEditTT->text().toStdString();
	t_real dTT = tl::str_to_var<t_real>(strTT);
	std::string strT = tl::var_to_str(dTT*0.5, g_iPrec);
	pEditT->setText(strT.c_str());
}

void NeutronDlg::RealTwoThetaEdited() { SetEditT(editBraggDirT, editBraggDirTT); }
void NeutronDlg::RecipTwoThetaEdited() { SetEditT(editBraggReciT, editBraggReciTT); }


void NeutronDlg::CalcBraggReal()
{
	std::string strN = editBraggDirN->text().toStdString();
	std::string strLam = editBraggDirLam->text().toStdString();
	std::string strD = editBraggDirD->text().toStdString();
	std::string strTT = editBraggDirTT->text().toStdString();

	int iOrder = tl::str_to_var<int>(strN);
	tl::t_length_si<t_real> lam = tl::str_to_var<t_real>(strLam)*angs;
	tl::t_length_si<t_real> d = tl::str_to_var<t_real>(strD)*angs;
	tl::t_angle_si<t_real> tt = tl::d2r(tl::str_to_var<t_real>(strTT))*rads;

	if(radioBraggDirLam->isChecked())
	{
		lam = tl::bragg_real_lam(d, tt, t_real(iOrder));
		std::string strLam = tl::var_to_str(t_real(lam/angs), g_iPrec);
		editBraggDirLam->setText(strLam.c_str());
	}
	else if(radioBraggDirD->isChecked())
	{
		d = tl::bragg_real_d(lam, tt, t_real(iOrder));
		std::string strD = tl::var_to_str(t_real(d/angs), g_iPrec);
		editBraggDirD->setText(strD.c_str());
	}
	else if(radioBraggDirTT->isChecked())
	{
		try
		{
			tt = tl::bragg_real_twotheta(d, lam, t_real(iOrder));
			std::string strTT = tl::var_to_str<t_real>(tl::r2d(tt/rads), g_iPrec);
			std::string strT = tl::var_to_str<t_real>(tl::r2d(t_real(0.5)*tt/rads), g_iPrec);
			editBraggDirTT->setText(strTT.c_str());
			editBraggDirT->setText(strT.c_str());
		}
		catch(const std::exception& ex)
		{
			editBraggDirTT->setText(ex.what());
			editBraggDirT->setText(ex.what());
		}
	}
}

void NeutronDlg::CalcBraggRecip()
{
	std::string strN = editBraggReciN->text().toStdString();
	std::string strLam = editBraggReciLam->text().toStdString();
	std::string strQ = editBraggReciQ->text().toStdString();
	std::string strTT = editBraggReciTT->text().toStdString();

	int iOrder = tl::str_to_var<int>(strN);
	tl::t_length_si<t_real> lam = tl::str_to_var<t_real>(strLam)*angs;
	tl::t_wavenumber_si<t_real> Q = tl::str_to_var<t_real>(strQ)/angs;
	tl::t_angle_si<t_real> tt = tl::d2r(tl::str_to_var<t_real>(strTT))*rads;

	if(radioBraggReciLam->isChecked())
	{
		lam = tl::bragg_recip_lam(Q, tt, t_real(iOrder));
		std::string strLam = tl::var_to_str(t_real(lam/angs), g_iPrec);
		editBraggReciLam->setText(strLam.c_str());
	}
	else if(radioBraggReciQ->isChecked())
	{
		Q = tl::bragg_recip_Q(lam, tt, t_real(iOrder));
		std::string strQ = tl::var_to_str(t_real(Q*angs), g_iPrec);
		editBraggReciQ->setText(strQ.c_str());
	}
	else if(radioBraggReciTT->isChecked())
	{
		try
		{
			tt = tl::bragg_recip_twotheta(Q, lam, t_real(iOrder));
			std::string strTT = tl::var_to_str<t_real>(tl::r2d(tt/rads), g_iPrec);
			std::string strT = tl::var_to_str<t_real>(tl::r2d(t_real(0.5)*tt/rads), g_iPrec);
			editBraggReciTT->setText(strTT.c_str());
			editBraggReciT->setText(strT.c_str());
		}
		catch(const std::exception& ex)
		{
			editBraggReciTT->setText(ex.what());
			editBraggReciT->setText(ex.what());
		}
	}
}


// -----------------------------------------------------------------------------

void NeutronDlg::Eval(const QString& _str)
{
	std::string str = _str.toStdString();
	std::pair<bool, t_real> pairRes = tl::eval_expr<std::string, t_real>(str);
	tl::set_eps_0(pairRes.second);

	if(!pairRes.first)
		editEvalRes->setText("invalid");
	else
		editEvalRes->setText(tl::var_to_str(pairRes.second, g_iPrec).c_str());
}

// -----------------------------------------------------------------------------


void NeutronDlg::accept()
{
	if(m_pSettings)
	{
		m_pSettings->setValue("neutron_props/geo", saveGeometry());
		m_pSettings->setValue("neutron_props/lam", editLam->text());
	}

	QDialog::accept();
}

void NeutronDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}

#include "NeutronDlg.moc"
