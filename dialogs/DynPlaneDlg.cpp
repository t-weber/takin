/*
 * Dynamic Plane Dialog
 * @author tweber
 * @date 2013, jan-2015
 * @license GPLv2
 */

#include "DynPlaneDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/neutrons.h"
#include <boost/units/io.hpp>

using t_real = t_real_glob;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_energy_si<t_real> meV = tl::get_one_meV<t_real>();
static const tl::t_angle_si<t_real> rads = tl::get_one_radian<t_real>();


DynPlaneDlg::DynPlaneDlg(QWidget* pParent, QSettings *pSettings)
		: QDialog(pParent), m_pSettings(pSettings)
{
	this->setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);
	}


	m_plotwrap.reset(new QwtPlotWrapper(plot));
	m_plotwrap->GetCurve(0)->setTitle("Kinematic Plane");
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, "Q (1/A)");
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, "E (meV)");
	if(m_plotwrap->HasTrackerSignal())
		connect(m_plotwrap->GetPicker(), SIGNAL(moved(const QPointF&)), this, SLOT(cursorMoved(const QPointF&)));


	QObject::connect(comboFixedE, SIGNAL(currentIndexChanged(int)), this, SLOT(FixedKiKfToggled()));

	std::vector<QDoubleSpinBox*> vecSpinBoxes = {spinEiEf, spinMinQ, spinMaxQ, spinAngle};
	for(QDoubleSpinBox* pSpin : vecSpinBoxes)
		QObject::connect(pSpin, SIGNAL(valueChanged(double)), this, SLOT(Calc()));
	QObject::connect(btnSync, SIGNAL(toggled(bool)), this, SLOT(Calc()));

	Calc();


	if(m_pSettings && m_pSettings->contains("dyn_plane/geo"))
		restoreGeometry(m_pSettings->value("dyn_plane/geo").toByteArray());
}

DynPlaneDlg::~DynPlaneDlg()
{}

void DynPlaneDlg::cursorMoved(const QPointF& pt)
{
	std::string strX = tl::var_to_str(pt.x(), g_iPrecGfx);
	std::string strY = tl::var_to_str(pt.y(), g_iPrecGfx);

	std::ostringstream ostr;
	ostr << "(" << strX << ", " << strY << ")";

	this->labelStatus->setText(ostr.str().c_str());
}

void DynPlaneDlg::Calc()
{
	const unsigned int NUM_POINTS = 512;

	const t_real dMinQ = spinMinQ->value();
	const t_real dMaxQ = spinMaxQ->value();
	const t_real dAngle = tl::d2r(spinAngle->value());
	const bool bFixedKi = (comboFixedE->currentIndex()==0);

	if(btnSync->isChecked())
		spinEiEf->setValue(bFixedKi ? m_dEi : m_dEf);

	tl::t_energy_si<t_real> EiEf = t_real(spinEiEf->value()) * meV;


	//m_pPlanePlot->clear();
	std::vector<t_real> vecQ[2], vecE[2];
	vecQ[0].reserve(NUM_POINTS); vecE[0].reserve(NUM_POINTS);
	vecQ[1].reserve(NUM_POINTS); vecE[1].reserve(NUM_POINTS);

	tl::t_angle_si<t_real> twotheta = dAngle * rads;

	for(unsigned int iPt=0; iPt<NUM_POINTS; ++iPt)
	{
		for(unsigned int iSign=0; iSign<=1; ++iSign)
		{
			tl::t_wavenumber_si<t_real> Q = (dMinQ + (dMaxQ - dMinQ)/t_real(NUM_POINTS)*t_real(iPt)) / angs;
			tl::t_energy_si<t_real> dE = tl::kinematic_plane(bFixedKi, iSign, EiEf, Q, twotheta);

			t_real _dQ = Q * angs;
			t_real _dE = dE / meV;

			if(!std::isnan(_dQ) && !std::isnan(_dE) && !std::isinf(_dQ) && !std::isinf(_dE))
			{
				vecQ[iSign].push_back(Q * angs);
				vecE[iSign].push_back(dE / meV);
			}
		}
	}

	m_vecQ.clear();
	m_vecE.clear();

	m_vecQ.insert(m_vecQ.end(), vecQ[0].rbegin(), vecQ[0].rend());
	m_vecE.insert(m_vecE.end(), vecE[0].rbegin(), vecE[0].rend());

	m_vecQ.insert(m_vecQ.end(), vecQ[1].begin(), vecQ[1].end());
	m_vecE.insert(m_vecE.end(), vecE[1].begin(), vecE[1].end());

	set_qwt_data<t_real>()(*m_plotwrap, m_vecQ, m_vecE);
}

void DynPlaneDlg::RecipParamsChanged(const RecipParams& params)
{
	m_d2Theta = params.d2Theta;
	m_dEi = tl::k2E(params.dki/angs)/meV;
	m_dEf = tl::k2E(params.dkf/angs)/meV;

	Calc();
}

void DynPlaneDlg::FixedKiKfToggled()
{
	if(comboFixedE->currentIndex() == 0)
		labelFixedKiKf->setText("E_i (meV):");
	else
		labelFixedKiKf->setText("E_f (meV):");

	Calc();
}


void DynPlaneDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}

void DynPlaneDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("dyn_plane/geo", saveGeometry());

	QDialog::accept();
}


#include "DynPlaneDlg.moc"
