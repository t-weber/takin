/*
 * Spurion Dialog
 * @author Tobias Weber
 * @date 26-may-2014
 * @license GPLv2
 */

#include "SpurionDlg.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/string/string.h"
#include "tlibs/string/spec_char.h"

#include <sstream>
#include <iostream>
#include <qwt_picker_machine.h>

using t_real = t_real_glob;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_energy_si<t_real> meV = tl::get_one_meV<t_real>();


SpurionDlg::SpurionDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}

	m_plotwrap.reset(new QwtPlotWrapper(plotbragg));
	m_plotwrap->GetCurve(0)->setTitle("Bragg Tail");

	if(m_plotwrap->HasTrackerSignal())
		connect(m_plotwrap->GetPicker(), SIGNAL(moved(const QPointF&)), this, SLOT(cursorMoved(const QPointF&)));

	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, "q (1/A)");
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, "E (meV)");


	QObject::connect(radioFixedEi, SIGNAL(toggled(bool)), this, SLOT(ChangedKiKfMode()));

	QObject::connect(radioFixedEi, SIGNAL(toggled(bool)), this, SLOT(Calc()));
	QObject::connect(btnSync, SIGNAL(toggled(bool)), this, SLOT(Calc()));
	QObject::connect(spinE, SIGNAL(valueChanged(double)), this, SLOT(Calc()));

	QObject::connect(checkFilter, SIGNAL(toggled(bool)), this, SLOT(CalcInel()));
	QObject::connect(spinOrder, SIGNAL(valueChanged(int)), this, SLOT(CalcInel()));

	QObject::connect(spinMinQ, SIGNAL(valueChanged(double)), this, SLOT(CalcBragg()));
	QObject::connect(spinMaxQ, SIGNAL(valueChanged(double)), this, SLOT(CalcBragg()));

	Calc();


	if(m_pSettings && m_pSettings->contains("spurions/geo"))
		restoreGeometry(m_pSettings->value("spurions/geo").toByteArray());
}

SpurionDlg::~SpurionDlg()
{}


void SpurionDlg::ChangedKiKfMode()
{
	if(radioFixedEi->isChecked())
		labelE->setText("E_i (meV):");
	else
		labelE->setText("E_f (meV):");
}

void SpurionDlg::Calc()
{
	const bool bFixedEi = radioFixedEi->isChecked();

	if(btnSync->isChecked())
	{
		const t_real dSyncedE = bFixedEi ? m_dEi : m_dEf;
		spinE->setValue(dSyncedE);
	}

	CalcInel();
	CalcBragg();
}

void SpurionDlg::CalcInel()
{
	const bool bFixedEi = radioFixedEi->isChecked();
	t_real dE = spinE->value();

	const unsigned int iMaxOrder = (unsigned int)spinOrder->value();
	const bool bFilter = checkFilter->isChecked();

	std::vector<t_real> vecSpurions;
	std::vector<std::string> vecInfo;

	if(bFilter)
	{
		for(unsigned int iOrder=1; iOrder<=iMaxOrder; ++iOrder)
		{
			unsigned int iOrderMono=1, iOrderAna=1;
			if(bFixedEi)
				iOrderAna = iOrder;
			else
				iOrderMono = iOrder;

			t_real dE_sp = tl::get_inelastic_spurion(bFixedEi, dE*meV,
				iOrderMono, iOrderAna) / meV;

			if(dE_sp != 0.)
			{
				vecSpurions.push_back(dE_sp);

				std::ostringstream ostrInfo;
				ostrInfo << "Mono order: " << iOrderMono
					<< ", Ana order: " << iOrderAna;
				vecInfo.push_back(ostrInfo.str());
			}
		}
	}
	else
	{
		for(unsigned int iOrderMono=1; iOrderMono<=iMaxOrder; ++iOrderMono)
		for(unsigned int iOrderAna=1; iOrderAna<=iMaxOrder; ++iOrderAna)
		{
			t_real dE_sp = tl::get_inelastic_spurion(bFixedEi, dE*meV,
				iOrderMono, iOrderAna) / meV;

			if(dE_sp != 0.)
			{
				vecSpurions.push_back(dE_sp);

				std::ostringstream ostrInfo;
				ostrInfo << "Mono order: " << iOrderMono
						<< ", Ana order: " << iOrderAna;
				vecInfo.push_back(ostrInfo.str());
			}
		}
	}

	const std::string& strDelta = tl::get_spec_char_utf8("Delta");
	const std::string& strBullet = tl::get_spec_char_utf8("bullet");

	std::ostringstream ostr;
	ostr << "Spurious inelastic signals for " + strDelta + "E = \n\n";
	for(unsigned int i=0; i<vecSpurions.size(); ++i)
	{
		const t_real dE_Sp = vecSpurions[i];
		const std::string& strInfo = vecInfo[i];

		ostr << "  " << strBullet << " ";
		ostr << tl::var_to_str(dE_Sp, 4) << " meV";
		ostr << " (" << strInfo << ")\n";
	}

	textSpurions->setPlainText(QString::fromUtf8(ostr.str().c_str(), ostr.str().size()));
}

void SpurionDlg::CalcBragg()
{
	const unsigned int NUM_POINTS = 512;

	const bool bFixedEi = radioFixedEi->isChecked();
	t_real dE = t_real(spinE->value());
	bool bImag;
	tl::t_wavenumber_si<t_real> k = tl::E2k(dE*meV, bImag);

	const t_real dMinq = spinMinQ->value();
	const t_real dMaxq = spinMaxQ->value();

	m_vecQ = tl::linspace(dMinq, dMaxq, NUM_POINTS);
	m_vecE.clear();
	m_vecE.reserve(m_vecQ.size());

	for(t_real dq : m_vecQ)
	{
		tl::t_wavenumber_si<t_real> q = dq/angs;
		tl::t_energy_si<t_real> E = tl::get_bragg_tail(k, q, bFixedEi);

		m_vecE.push_back(E/meV);
	}

	set_qwt_data<t_real>()(*m_plotwrap, m_vecQ, m_vecE);
}

void SpurionDlg::cursorMoved(const QPointF& pt)
{
	std::string strX = tl::var_to_str(pt.x(), g_iPrecGfx);
	std::string strY = tl::var_to_str(pt.y(), g_iPrecGfx);

	std::ostringstream ostr;
	ostr << "(" << strX << ", " << strY << ")";

	this->labelStatus->setText(ostr.str().c_str());
}


void SpurionDlg::paramsChanged(const RecipParams& parms)
{
	tl::t_wavenumber_si<t_real> ki = parms.dki / angs;
	tl::t_wavenumber_si<t_real> kf = parms.dkf / angs;
	tl::t_energy_si<t_real> Ei = tl::k2E(ki);
	tl::t_energy_si<t_real> Ef = tl::k2E(kf);

	m_dEi = Ei / meV;
	m_dEf = Ef / meV;

	Calc();
}


void SpurionDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("spurions/geo", saveGeometry());

	QDialog::accept();
}

void SpurionDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


#include "SpurionDlg.moc"
