/**
 * Scan Monitor
 * @author Tobias Weber
 * @date jul-2016
 * @license GPLv2
 */

#include "ScanMonDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/file/prop.h"
#include "tlibs/math/linalg.h"

using t_real = t_real_glob;


ScanMonDlg::ScanMonDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("scanmon/geo"))
			restoreGeometry(m_pSettings->value("scanmon/geo").toByteArray());
	}

	// ------------------------------------------------------------------------
	// live plot
	QColor colorBck(240, 240, 240, 255);
	plot->setCanvasBackground(colorBck);

	m_plotwrap.reset(new QwtPlotWrapper(plot, 2, true));
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, "x");
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, "Counts");

	QPen penCurve;
	penCurve.setColor(QColor(0,0,0x99));
	penCurve.setWidth(2);
	m_plotwrap->GetCurve(0)->setPen(penCurve);
	m_plotwrap->GetCurve(0)->setStyle(QwtPlotCurve::CurveStyle::Lines);
	m_plotwrap->GetCurve(0)->setTitle("Scan Curve");

	QPen penPoints;
	penPoints.setColor(QColor(0xff,0,0));
	penPoints.setWidth(4);
	m_plotwrap->GetCurve(1)->setPen(penPoints);
	m_plotwrap->GetCurve(1)->setStyle(QwtPlotCurve::CurveStyle::Dots);
	m_plotwrap->GetCurve(1)->setTitle("Scan Points");
	// ------------------------------------------------------------------------

	progress->setRange(0, 1000);
}

void ScanMonDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("scanmon/geo", saveGeometry());

	QDialog::accept();
}


static t_real get_value(const std::string& strVal)
{
	t_real val(0);

	std::size_t iBeg = strVal.find_first_of("0123456789e.+-");
	if(iBeg == std::string::npos)
		return val;
	std::string strVal2 = strVal.substr(iBeg);

	val = tl::str_to_var<t_real>(strVal2);
	return val;
}

void ScanMonDlg::UpdateValue(const std::string& strKey, const CacheVal& val)
{
	bool bUpdateProgress = 0;

	switch(val.ty)
	{
		case CacheValType::TIMER:
			m_dTimer = get_value(val.strVal);
			bUpdateProgress = 1;
			break;
		case CacheValType::PRESET:
			m_dPreset = get_value(val.strVal);
			bUpdateProgress = 1;
			break;
		case CacheValType::COUNTER:
			m_dCounter = get_value(val.strVal);
			bUpdateProgress = 1;
			break;
		case CacheValType::LIVE_PLOT:
			UpdatePlot(val.strVal);
			break;
		default:
			return;
	}

	if(bUpdateProgress)
	{
		t_real dProgress = 0.;
		t_real dExpCtr = 0.;

		if(!tl::float_equal(m_dPreset, t_real(0.)))
		{
			dProgress = m_dTimer / m_dPreset;
			if(!tl::float_equal(dProgress, t_real(0.)))
				dExpCtr = m_dCounter / dProgress;
		}

		if(dProgress < t_real(0.)) dProgress = t_real(0.);
		else if(dProgress > t_real(1.)) dProgress = t_real(1.);

		progress->setValue(dProgress * t_real(1000));


		std::ostringstream ostrCts, ostrExp, ostrProg;
		ostrCts.precision(g_iPrecGfx);
		ostrExp.precision(g_iPrecGfx);
		ostrProg.precision(g_iPrecGfx);

		ostrCts << std::fixed << unsigned(m_dCounter) << " +- " << std::sqrt(m_dCounter);
		ostrExp << std::fixed << unsigned(std::round(dExpCtr)) << " (" << dExpCtr << ")";
		ostrProg << std::fixed << m_dTimer << " of " << m_dPreset;

		editCts->setText(ostrCts.str().c_str());
		editExpected->setText(ostrExp.str().c_str());
		editTime->setText(ostrProg.str().c_str());
	}
}

/**
 * updates the live plot based on the xml-formatted data in strVals
 */
void ScanMonDlg::UpdatePlot(const std::string& strVals)
{
	if(!m_plotwrap || !m_plotwrap->GetPlot())
		return;

	std::istringstream istr(strVals);
	tl::Prop<std::string, 0> propScan;
	if(!propScan.Load(istr, tl::PropType::XML))
		return;

	int ix = propScan.Query<int>("scan/main_var", -1);
	if(ix < 0) return;
	std::string strx = "x_" + tl::var_to_str(ix);

	std::string strXName = propScan.Query<std::string>("scan/vars/" + strx, "x");
	std::string strYName = propScan.Query<std::string>("scan/vars/y_0", "Counts");
	std::string strX = propScan.Query<std::string>("scan/data/" + strx);
	std::string strY = propScan.Query<std::string>("scan/data/y_0");

	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, strXName.c_str());
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, strYName.c_str());

	std::vector<t_real> vecX, vecY;
	tl::get_tokens<t_real>(strX, std::string(" \t;,"), vecX);
	tl::get_tokens<t_real>(strY, std::string(" \t;,"), vecY);

	std::size_t iMinLen = std::min(vecX.size(), vecY.size());
	vecX.resize(iMinLen);
	vecY.resize(iMinLen);

	if(!tl::vec_equal(vecX, m_vecX, g_dEps) || !tl::vec_equal(vecY, m_vecY, g_dEps))
	{
		m_vecX = vecX;
		m_vecY = vecY;

		set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 0, 0);
		set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 1, 0);

		set_zoomer_base(m_plotwrap->GetZoomer(), m_vecX, m_vecY);
		m_plotwrap->GetPlot()->replot();
	}
}

void ScanMonDlg::ClearPlot()
{
	m_vecX.clear();
	m_vecY.clear();

	set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 0, 0);
	set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 1, 0);

	m_plotwrap->GetPlot()->replot();
}


#include "ScanMonDlg.moc"
