/**
 * Scattering Triangle Tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date mar-2014
 * @license GPLv2
 */

#include "recip3d.h"
#include "tlibs/math/geo.h"
#include <QGridLayout>


using t_real = t_real_glob;
using t_vec = ublas::vector<t_real>;
using t_mat = ublas::matrix<t_real>;


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
			resize(800, 600);
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

	m_vecQ_rlu = m_vecQ = tl::ublas::zero_vector<t_real>(3);

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
	const t_vec &vec0 = plane.GetDir0(),
		&vec1 = plane.GetDir1(),
		&vecNorm = plane.GetNorm();

	static const std::vector<t_real> vecColPeak = { 0., 0., 1., 0.7 };
	static const std::vector<t_real> vecColPeakPlane = { 1., 0., 0., 0.7 };
	static const std::vector<t_real> vecCol000 = { 0., 1., 0., 0.7 };
	static const std::vector<t_real> vecColCurQ = { 0., 0., 0., 1. };
	static const std::vector<t_real> vecColScatPlane = { 0., 0., 0.75, 0.15 };
	static const std::vector<t_real> vecColLine = { 0., 0., 0., 1. };

	bool bShowScatPlane = 1;
	bool bShowCurQ = 1;

	std::size_t iObjCnt = (unsigned int)((m_dMaxPeaks*2 + 1)*
		(m_dMaxPeaks*2 + 1) * (m_dMaxPeaks*2 + 1));
	if(bShowScatPlane) iObjCnt += 2;
	if(bShowCurQ) ++iObjCnt;

	m_pPlot->SetEnabled(0);
	m_pPlot->clear();
	m_pPlot->SetObjectCount(iObjCnt);

	std::size_t iCurObjIdx = 0;
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
				t_vec vecPeak = recip.GetPos(h,k,l);
				for(int i=0; i<3; ++i)
				{
					vecMin[i] = std::min(vecPeak[i], vecMin[i]);
					vecMax[i] = std::max(vecPeak[i], vecMax[i]);
				}

				t_real dDist = 0.;
				t_vec vecDropped = plane.GetDroppedPerp(vecPeak, &dDist);

				const std::vector<t_real> *pvecColor = &vecColPeak;
				if(tl::float_equal<t_real>(dDist, 0., m_dPlaneDistTolerance))
				{
					// (000) peak?
					if(tl::float_equal<t_real>(h, 0., g_dEps) &&
						tl::float_equal<t_real>(k, 0., g_dEps) &&
						tl::float_equal<t_real>(l, 0., g_dEps))
					{
						pvecColor = &vecCol000;
					}
					else
					{
						pvecColor = &vecColPeakPlane;
					}

					bInScatteringPlane = 1;
				}

				m_pPlot->PlotSphere(vecPeak, 0.05, iCurObjIdx);
				m_pPlot->SetObjectColor(iCurObjIdx, *pvecColor);

				std::ostringstream ostrLab;
				ostrLab << "(" << ih << " " << ik << " " << il << ")";
				m_pPlot->SetObjectLabel(iCurObjIdx, ostrLab.str());

				++iCurObjIdx;
			}


	// scattering plane
	if(bShowScatPlane)
	{
		std::vector<t_vec> vecVerts = 
		{
			-vec0*m_dMaxPeaks - vec1*m_dMaxPeaks,
			-vec0*m_dMaxPeaks + vec1*m_dMaxPeaks,
			 vec0*m_dMaxPeaks + vec1*m_dMaxPeaks,
			 vec0*m_dMaxPeaks - vec1*m_dMaxPeaks
		};

		m_pPlot->PlotPoly(vecVerts, vecNorm, iCurObjIdx);
		m_pPlot->SetObjectColor(iCurObjIdx, vecColScatPlane);
		m_pPlot->SetObjectCull(iCurObjIdx, 0);
		++iCurObjIdx;

		m_pPlot->PlotLines(vecVerts, 2., iCurObjIdx);
		m_pPlot->SetObjectColor(iCurObjIdx, vecColLine);
		++iCurObjIdx;
	}


	// current Q position
	if(bShowCurQ)
	{
		m_pPlot->PlotSphere(m_vecQ, 0.04, iCurObjIdx);
		m_pPlot->SetObjectColor(iCurObjIdx, vecColCurQ);
		m_pPlot->SetObjectAnimation(iCurObjIdx, 1);

		// current Q label
		std::ostringstream ostrTip;
		ostrTip.precision(g_iPrecGfx);
		ostrTip << "Q = (" << m_vecQ_rlu[0] << ", " << m_vecQ_rlu[1] << ", " << m_vecQ_rlu[2] << ") rlu";
		ostrTip << "\nQ = (" << m_vecQ[0] << ", " << m_vecQ[1] << ", " << m_vecQ[2] << ") 1/A";
		m_pPlot->SetObjectLabel(iCurObjIdx, ostrTip.str());

		m_iQIdx = iCurObjIdx;
		++iCurObjIdx;
	}

	m_pPlot->SetObjectCount(iCurObjIdx);	// actual count (some peaks forbidden by sg)
	m_pPlot->SetMinMax(vecMin, vecMax);
	m_pPlot->SetEnabled(1);
}


/**
 * scattering triangle changed
 */
void Recip3DDlg::RecipParamsChanged(const RecipParams& recip)
{
	m_vecQ = tl::make_vec({ -recip.Q[0], -recip.Q[1], -recip.Q[2] });
	m_vecQ_rlu = tl::make_vec({ -recip.Q_rlu[0], -recip.Q_rlu[1], -recip.Q_rlu[2] });

	// if a 3d object is already assigned, update it
	if(m_pPlot && m_iQIdx)
	{
		m_pPlot->SetEnabled(0);

		m_pPlot->PlotSphere(m_vecQ, 0.04, *m_iQIdx);

		// current Q label
		std::ostringstream ostrTip;
		ostrTip.precision(g_iPrecGfx);
		ostrTip << "Q = (" << m_vecQ_rlu[0] << ", " << m_vecQ_rlu[1] << ", " << m_vecQ_rlu[2] << ") rlu";
		ostrTip << "\nQ = (" << m_vecQ[0] << ", " << m_vecQ[1] << ", " << m_vecQ[2] << ") 1/A";
		m_pPlot->SetObjectLabel(*m_iQIdx, ostrTip.str());

		m_pPlot->SetEnabled(1);
	}
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
