/**
 * Scattering Triangle Tool
 * @author tweber
 * @date mar-2014
 * @license GPLv2
 */

#include "recip3d.h"
#include "tlibs/math/geo.h"
#include <QGridLayout>


using t_real = t_real_glob;

Recip3DDlg::Recip3DDlg(QWidget* pParent, QSettings *pSettings)
	: QDialog(pParent, Qt::Tool), m_pSettings(pSettings),
	m_pStatus(new QStatusBar(this)),
	m_pPlot(new PlotGl(this, pSettings, 0.25))
{
	m_pPlot->SetEnabled(0);

	setWindowTitle("Reciprocal Space");
	m_pStatus->setSizeGripEnabled(1);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("recip3d/geo"))
			restoreGeometry(m_pSettings->value("recip3d/geo").toByteArray());
		else
			resize(640, 480);
	}

	m_pPlot->SetPrec(g_iPrecGfx);
	m_pPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QGridLayout *gridLayout = new QGridLayout(this);
	gridLayout->setContentsMargins(4, 4, 4, 4);
	gridLayout->addWidget(m_pPlot, 0, 0, 1, 1);
	gridLayout->addWidget(m_pStatus, 1, 0, 1, 1);

	m_pPlot->AddHoverSlot([this](const PlotObjGl* pObj)
	{
		std::string strStatus;
		if(pObj)
			strStatus = pObj->strLabel;
		tl::find_all_and_replace<std::string>(strStatus, "\n", ", ");

		if(strStatus.length())
			m_pStatus->showMessage(strStatus.c_str());
		else
			m_pStatus->clearMessage();
	});

	m_pPlot->SetLabels("a", "b", "c");
	m_pPlot->SetEnabled(1);
}

Recip3DDlg::~Recip3DDlg()
{
	if(m_pPlot)
	{
		delete m_pPlot;
		m_pPlot = 0;
	}
}


void Recip3DDlg::CalcPeaks(const LatticeCommon<t_real_glob>& recipcommon)
{
	const tl::Lattice<t_real>& recip = recipcommon.recip;
	const SpaceGroup<t_real>* pSpaceGroup = recipcommon.pSpaceGroup;
	const tl::Plane<t_real>& plane = recipcommon.plane;

	const unsigned int iObjCnt = (unsigned int)((m_dMaxPeaks*2 + 1)*
		(m_dMaxPeaks*2 + 1) * (m_dMaxPeaks*2 + 1));

	m_pPlot->SetEnabled(0);
	m_pPlot->clear();
	m_pPlot->SetObjectCount(iObjCnt);

	std::size_t iPeakIdx = 0;
	const t_real dLimMax = std::numeric_limits<t_real>::max();

	std::vector<t_real> vecMin = {dLimMax, dLimMax, dLimMax},
		vecMax = {-dLimMax, -dLimMax, -dLimMax};

	for(t_real h=-m_dMaxPeaks; h<=m_dMaxPeaks; h+=1.)
		for(t_real k=-m_dMaxPeaks; k<=m_dMaxPeaks; k+=1.)
			for(t_real l=-m_dMaxPeaks; l<=m_dMaxPeaks; l+=1.)
			{
				int ih = int(h), ik = int(k), il = int(l);
				if(pSpaceGroup)
				{
					if(!pSpaceGroup->HasReflection(ih, ik, il))
						continue;
				}

				bool bInScatteringPlane = 0;
				ublas::vector<t_real> vecPeak = recip.GetPos(h,k,l);
				for(unsigned int i=0; i<3; ++i)
				{
					vecMin[i] = std::min(vecPeak[i], vecMin[i]);
					vecMax[i] = std::max(vecPeak[i], vecMax[i]);
				}

				t_real dDist = 0.;
				ublas::vector<t_real> vecDropped = plane.GetDroppedPerp(vecPeak, &dDist);

				std::vector<t_real> vecColor({0., 0., 1., 0.7});
				if(tl::float_equal<t_real>(dDist, 0., m_dPlaneDistTolerance))
				{
					bool bIsDirectBeam = 0;
					if(tl::float_equal<t_real>(h, 0.) &&
						tl::float_equal<t_real>(k, 0.) &&
						tl::float_equal<t_real>(l, 0.))
						bIsDirectBeam = 1;

					if(bIsDirectBeam)
						vecColor = std::vector<t_real>{0., 1., 0., 0.7};
					else
						vecColor = std::vector<t_real>{1., 0., 0., 0.7};

					bInScatteringPlane = 1;
				}

				m_pPlot->PlotSphere(vecPeak, 0.1, iPeakIdx);
				m_pPlot->SetObjectColor(iPeakIdx, vecColor);

				std::ostringstream ostrLab;
				ostrLab << "(" << ih << " " << ik << " " << il << ")";
				m_pPlot->SetObjectLabel(iPeakIdx, ostrLab.str());

				++iPeakIdx;
			}

	m_pPlot->SetObjectCount(iPeakIdx);	// actual count (some peaks forbidden by sg)
	m_pPlot->SetMinMax(vecMin, vecMax);
	m_pPlot->SetEnabled(1);
}

void Recip3DDlg::closeEvent(QCloseEvent *pEvt)
{
	if(m_pSettings)
		m_pSettings->setValue("recip3d/geo", saveGeometry());
	QDialog::closeEvent(pEvt);
}

void Recip3DDlg::hideEvent(QHideEvent *pEvt)
{
	if(m_pPlot) m_pPlot->SetEnabled(0);
	QDialog::hideEvent(pEvt);
}
void Recip3DDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
	if(m_pPlot) m_pPlot->SetEnabled(1);
}


#include "recip3d.moc"
