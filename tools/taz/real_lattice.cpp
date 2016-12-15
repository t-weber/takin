/**
 * Real crystal lattice
 * @author tweber
 * @date 2014 - 2016
 * @license GPLv2
 */

#include "tlibs/helper/flags.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/math/atoms.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/log/log.h"
#include "libs/globals.h"
#include "real_lattice.h"

#include <QToolTip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <tuple>


using t_real = t_real_glob;
using t_vec = ublas::vector<t_real>;
using t_mat = ublas::matrix<t_real>;


#define REAL_LATTICE_NODE_TYPE_KEY	0

enum LatticeNodeType
{
	NODE_REAL_LATTICE,
	NODE_REAL_LATTICE_ATOM,

	NODE_REAL_LATTICE_OTHER
};



// --------------------------------------------------------------------------------

LatticePoint::LatticePoint()
{
	//setCursor(Qt::ArrowCursor);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
}

QRectF LatticePoint::boundingRect() const
{
	return QRectF(-35., -10., 70., 50.);
}

void LatticePoint::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setFont(g_fontGfx);
	painter->setBrush(m_color);
	painter->drawEllipse(QRectF(-3., -3., 6., 6.));

	if(m_strLabel != "")
	{
		painter->setPen(m_color);
		QRectF rect = boundingRect();
		rect.setTop(rect.top()+16.5);
		//painter->drawRect(rect);
		painter->drawText(rect, Qt::AlignHCenter|Qt::AlignTop, m_strLabel);
	}
}

// --------------------------------------------------------------------------------

LatticeAtom::LatticeAtom()
{
	//setCursor(Qt::ArrowCursor);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
}

QRectF LatticeAtom::boundingRect() const
{
	return QRectF(-35., -10., 70., 50.);
}

void LatticeAtom::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setFont(g_fontGfx);
	painter->setBrush(m_color);
	painter->setPen(Qt::darkCyan);
	painter->drawEllipse(QRectF(-3., -3., 6., 6.));

	if(m_strElem != "")
	{
		painter->setPen(m_color);
		QRectF rect = boundingRect();
		rect.setTop(rect.top()+16.5);
		//painter->drawRect(rect);
		painter->drawText(rect, Qt::AlignHCenter|Qt::AlignTop, m_strElem.c_str());
	}
}


// --------------------------------------------------------------------------------


RealLattice::RealLattice(LatticeScene& scene)
	: m_scene(scene)
{
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setAcceptedMouseButtons(0);
	m_bReady = 1;
}

RealLattice::~RealLattice()
{
	m_bReady = 0;
	ClearPeaks();
}

QRectF RealLattice::boundingRect() const
{
	return QRectF(-1000.*m_dZoom, -1000.*m_dZoom, 2000.*m_dZoom, 2000.*m_dZoom);
}

void RealLattice::SetZoom(t_real dZoom)
{
	m_dZoom = dZoom;
	m_scene.update();
}

void RealLattice::SetWSVisible(bool bVisible)
{
	m_bShowWS = bVisible;
	this->update();
}

void RealLattice::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setFont(g_fontGfx);

	// Wigner-Seitz cell
	if(m_bShowWS && m_ws.IsValid())
	{
		QPen penOrg = painter->pen();
		painter->setPen(Qt::lightGray);

		const t_vec& vecCentral = m_ws.GetCentralReflex() * m_dScaleFactor*m_dZoom;
		//std::cout << vecCentral << std::endl;
		for(const LatticePoint* pPeak : m_vecPeaks)
		{
			QPointF peakPos = pPeak->pos();
			peakPos *= m_dZoom;

			const tl::Brillouin2D<t_real>::t_vertices<t_real>& verts = m_ws.GetVertices();
			for(const tl::Brillouin2D<t_real>::t_vecpair<t_real>& vertpair : verts)
			{
				const t_vec& vec1 = vertpair.first * m_dScaleFactor * m_dZoom;
				const t_vec& vec2 = vertpair.second * m_dScaleFactor * m_dZoom;

				QPointF pt1 = vec_to_qpoint(vec1 - vecCentral) + peakPos;
				QPointF pt2 = vec_to_qpoint(vec2 - vecCentral) + peakPos;

				QLineF lineBZ(pt1, pt2);
				painter->drawLine(lineBZ);
			}
		}

		painter->setPen(penOrg);
	}
}

void RealLattice::CalcPeaks(const LatticeCommon<t_real>& latticecommon)
{
	ClearPeaks();
	m_kdLattice.Unload();
	m_lattice = latticecommon.lattice;
	m_matPlane = latticecommon.matPlaneReal;
	m_matPlane_inv = latticecommon.matPlaneReal_inv;

	// central peak for WS cell calculation
	ublas::vector<int> veciCent = tl::make_vec({0.,0.,0.});

	const std::string strAA = tl::get_spec_char_utf8("AA");

	// --------------------------------------------------------------------
	// atom positions in unit cell
	std::vector<QColor> colors = {QColor(127,0,0), QColor(0,127,0), QColor(0,0,127),
		QColor(127,127,0), QColor(0,127,127), QColor(127,0,127)};

	for(std::size_t iAtom=0; iAtom<latticecommon.vecAllAtoms.size(); ++iAtom)
	{
		const std::string& strElem = latticecommon.vecAllNames[iAtom];
		const t_vec& vecThisAtom = latticecommon.vecAllAtoms[iAtom];
		const t_vec& vecThisAtomFrac = latticecommon.vecAllAtomsFrac[iAtom];
		std::size_t iCurAtomType = latticecommon.vecAllAtomTypes[iAtom];

		LatticeAtom *pAtom = new LatticeAtom();
		m_vecAtoms.push_back(pAtom);

		pAtom->m_strElem = strElem;
		pAtom->m_vecPos = std::move(vecThisAtom);
		pAtom->m_vecProj = latticecommon.planeReal.GetDroppedPerp(pAtom->m_vecPos/*, &pAtom->m_dProjDist*/);
		pAtom->m_dProjDist = latticecommon.planeReal.GetDist(pAtom->m_vecPos);

		t_vec vecCoord = ublas::prod(m_matPlane_inv, pAtom->m_vecProj);
		t_real dX = vecCoord[0], dY = -vecCoord[1];

		pAtom->setPos(dX * m_dScaleFactor, dY * m_dScaleFactor);
		pAtom->setData(REAL_LATTICE_NODE_TYPE_KEY, NODE_REAL_LATTICE_ATOM);

		std::ostringstream ostrTip;
		ostrTip.precision(g_iPrecGfx);
		ostrTip << pAtom->m_strElem;
		ostrTip << "\n("
			<< vecThisAtomFrac[0] << ", "
			<< vecThisAtomFrac[1] << ", "
			<< vecThisAtomFrac[2] << ") frac";
		ostrTip << "\n("
			<< vecThisAtom[0] << ", "
			<< vecThisAtom[1] << ", "
			<< vecThisAtom[2] << ") " << strAA;
		ostrTip << "\nDistance to Plane: " << pAtom->m_dProjDist << " " << strAA;
		pAtom->setToolTip(QString::fromUtf8(ostrTip.str().c_str(), ostrTip.str().length()));
		pAtom->SetColor(colors[iCurAtomType % colors.size()]);

		m_scene.addItem(pAtom);
	}
	// --------------------------------------------------------------------



	std::list<std::vector<t_real>> lstPeaksForKd;

	for(int ih=-m_iMaxPeaks; ih<=m_iMaxPeaks; ++ih)
		for(int ik=-m_iMaxPeaks; ik<=m_iMaxPeaks; ++ik)
			for(int il=-m_iMaxPeaks; il<=m_iMaxPeaks; ++il)
			{
				const t_real h = t_real(ih), k = t_real(ik), l = t_real(il);
				t_vec vecPeak = m_lattice.GetPos(h,k,l);

				// add peak in A and in fractional units
				lstPeaksForKd.push_back(std::vector<t_real>{vecPeak[0],vecPeak[1],vecPeak[2], h,k,l});

				t_real dDist = 0.;
				t_vec vecDropped = latticecommon.planeReal.GetDroppedPerp(vecPeak, &dDist);

				if(tl::float_equal<t_real>(dDist, 0., m_dPlaneDistTolerance))
				{
					t_vec vecCoord = ublas::prod(m_matPlane_inv, vecDropped);
					t_real dX = vecCoord[0], dY = -vecCoord[1];

					LatticePoint *pPeak = new LatticePoint();
					if(ih==0 && ik==0 && il==0)
						pPeak->SetColor(Qt::darkGreen);
					pPeak->setPos(dX * m_dScaleFactor, dY * m_dScaleFactor);
					pPeak->setData(REAL_LATTICE_NODE_TYPE_KEY, NODE_REAL_LATTICE);

					std::ostringstream ostrTip;
					ostrTip.precision(g_iPrecGfx);

					ostrTip << "(" << ih << " " << ik << " " << il << ")";
					pPeak->SetLabel(ostrTip.str().c_str());

					tl::set_eps_0(vecPeak, g_dEps);
					ostrTip << " frac\n";
					ostrTip << "("
							<< vecPeak[0] << ", "
							<< vecPeak[1] << ", "
							<< vecPeak[2] << ") " << strAA;
					//ostrTip << "\ndistance to plane: " << dDist << " " << strAA;
					pPeak->setToolTip(QString::fromUtf8(ostrTip.str().c_str(), ostrTip.str().length()));

					m_vecPeaks.push_back(pPeak);
					m_scene.addItem(pPeak);


					// Wigner-Seitz cell
					if(ih==veciCent[0] && ik==veciCent[1] && il==veciCent[2])
					{
						t_vec vecCentral = tl::make_vec({dX, dY});
						//log_debug("Central ", ih, ik, il, ": ", vecCentral);
						m_ws.SetCentralReflex(vecCentral);
					}
					// TODO: check if 2 next neighbours is sufficient for all space groups
					else if(std::abs(ih-veciCent[0])<=2
							&& std::abs(ik-veciCent[1])<=2
							&& std::abs(il-veciCent[2])<=2)
					{
						t_vec vecN = tl::make_vec({dX, dY});
						//log_debug("Reflex: ", vecN);
						m_ws.AddReflex(vecN);
					}
				}
			}

	m_ws.CalcBZ();
	//for(LatticePoint* pPeak : m_vecPeaks)
	//	pPeak->SetBZ(&m_ws);

	m_kdLattice.Load(lstPeaksForKd, 3);

	this->update();
}

t_vec RealLattice::GetHKLFromPlanePos(t_real x, t_real y) const
{
	if(!HasPeaks())
		return t_vec();

	t_vec vec = x*tl::get_column(m_matPlane, 0)
		+ y*tl::get_column(m_matPlane, 1);
	return m_lattice.GetHKL(vec);
}

void RealLattice::ClearPeaks()
{
	m_ws.Clear();

	for(LatticePoint*& pPeak : m_vecPeaks)
	{
		if(pPeak)
		{
			m_scene.removeItem(pPeak);
			delete pPeak;
			pPeak = nullptr;
		}
	}
	m_vecPeaks.clear();

	for(LatticeAtom*& pAtom : m_vecAtoms)
	{
		if(pAtom)
		{
			m_scene.removeItem(pAtom);
			delete pAtom;
			pAtom = nullptr;
		}
	}
	m_vecAtoms.clear();
}

// --------------------------------------------------------------------------------


LatticeScene::LatticeScene(QObject *pParent)
	: QGraphicsScene(pParent), m_pLatt(new RealLattice(*this))
{
	this->addItem(m_pLatt);
}

LatticeScene::~LatticeScene()
{
	delete m_pLatt;
}

void LatticeScene::scaleChanged(t_real dTotalScale)
{
	if(!m_pLatt) return;
	m_pLatt->SetZoom(dTotalScale);
}

void LatticeScene::mousePressEvent(QGraphicsSceneMouseEvent *pEvt)
{
	m_bMousePressed = 1;
	QGraphicsScene::mousePressEvent(pEvt);
}


#ifdef USE_GIL

#include "tlibs/gfx/gil.h"
namespace gil = boost::gil;

bool LatticeScene::ExportWSAccurate(const char* pcFile) const
{
	if(!m_pLatt) return false;

	const int iW = 720;
	const int iH = 720;

	gil::rgb8_view_t view;
	std::vector<gil::rgb8_pixel_t> vecPix;
	tl::create_imgview(iW, iH, vecPix, view);

	const int iMaxPeaks = m_pLatt->GetMaxPeaks();
	int iXMid = sceneRect().left() + (sceneRect().right()-sceneRect().left())/2;
	int iYMid = sceneRect().top() + (sceneRect().bottom()-sceneRect().top())/2;

	int _iY=0;
	for(int iY=iYMid-iH/2; iY<iYMid+iH/2; ++iY, ++_iY)
	{
		auto iterRow = view.row_begin(_iY);

		int _iX=0;
		t_real dY = iY;
		for(int iX=iXMid-iW/2; iX<iXMid+iW/2; ++iX, ++_iX)
		{
			t_real dX = iX;
			t_vec vecHKL = m_pLatt->GetHKLFromPlanePos(dX, -dY);
			if(vecHKL.size()!=3) return false;
			vecHKL /= m_pLatt->GetScaleFactor();

			const std::vector<t_real>* pvecNearest = nullptr;
			const tl::Kd<t_real>& kd = m_pLatt->GetKdLattice();
			t_vec vecHKLA = m_pLatt->GetRealLattice().GetPos(vecHKL[0], vecHKL[1], vecHKL[2]);

			if(kd.GetRootNode())
			{
				std::vector<t_real> stdvecHKL{vecHKLA[0], vecHKLA[1], vecHKLA[2]};
				pvecNearest = &kd.GetNearestNode(stdvecHKL);
			}

			if(!pvecNearest) return false;
			t_real dDist = ublas::norm_2(tl::make_vec({(*pvecNearest)[0], (*pvecNearest)[1], (*pvecNearest)[2]}) - vecHKLA);

			bool bIsDirectBeam = 0;
			if(tl::float_equal<t_real>((*pvecNearest)[3], 0.) && tl::float_equal<t_real>((*pvecNearest)[4], 0.) && tl::float_equal<t_real>((*pvecNearest)[5], 0.))
				bIsDirectBeam = 1;

			int iR = ((*pvecNearest)[3]+iMaxPeaks) * 255 / (iMaxPeaks*2);
			int iG = ((*pvecNearest)[4]+iMaxPeaks) * 255 / (iMaxPeaks*2);
			int iB = ((*pvecNearest)[5]+iMaxPeaks) * 255 / (iMaxPeaks*2);
			t_real dBraggAmp = tl::gauss_model<t_real>(dDist, 0., 0.01, 255., 0.);
			iR += bIsDirectBeam ? -(unsigned int)dBraggAmp : (unsigned int)dBraggAmp;
			iG += bIsDirectBeam ? -(unsigned int)dBraggAmp : (unsigned int)dBraggAmp;
			iB += bIsDirectBeam ? -(unsigned int)dBraggAmp : (unsigned int)dBraggAmp;

			iR = tl::clamp(iR, 0, 255);
			iG = tl::clamp(iG, 0, 255);
			iB = tl::clamp(iB, 0, 255);

			iterRow[_iX] = gil::rgb8_pixel_t((unsigned char)iR, (unsigned char)iG, (unsigned char)iB);
		}

		//tl::log_info("BZ export: Line ", _iY+1, " of ", iH);
	}

	if(!tl::save_view(pcFile, &view))
	{
		tl::log_err("Cannot write image \"", pcFile, "\".");
		return false;
	}

	return true;
}

#else
bool LatticeScene::ExportWSAccurate(const char* pcFile) const { return 0; }
#endif


void LatticeScene::drawBackground(QPainter* pPainter, const QRectF& rect)
{
	QGraphicsScene::drawBackground(pPainter, rect);
	// TODO: draw accurate WS cell
}

void LatticeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *pEvt)
{
	bool bHandled = 0;
	bool bAllowed = 1;


	// tooltip
	if(m_pLatt)
	{
		const t_real dX = pEvt->scenePos().x()/m_pLatt->GetScaleFactor();
		const t_real dY = -pEvt->scenePos().y()/m_pLatt->GetScaleFactor();

		t_vec vecHKL = m_pLatt->GetHKLFromPlanePos(dX, dY);
		tl::set_eps_0(vecHKL, g_dEps);

		if(vecHKL.size()==3)
		{
			//std::ostringstream ostrPos;
			//ostrPos << "(" << vecHKL[0] << ", " << vecHKL[1] << ", " << vecHKL[2]  << ")";
			//QToolTip::showText(pEvt->screenPos(), ostrPos.str().c_str());

			const std::vector<t_real>* pvecNearest = nullptr;

			const tl::Kd<t_real>& kd = m_pLatt->GetKdLattice();
			const tl::Lattice<t_real>& lattice = m_pLatt->GetRealLattice();
			t_vec vecHKLA = lattice.GetPos(vecHKL[0], vecHKL[1], vecHKL[2]);

			if(kd.GetRootNode())
			{
				std::vector<t_real> stdvecHKL{vecHKLA[0], vecHKLA[1], vecHKLA[2]};
				pvecNearest = &kd.GetNearestNode(stdvecHKL);
				if(pvecNearest->size() < 6)
				{
					pvecNearest = nullptr;
					tl::log_warn("Invalid WS node.");
				}
			}

			emit coordsChanged(vecHKL[0], vecHKL[1], vecHKL[2],
				pvecNearest != nullptr,
				pvecNearest?(*pvecNearest)[3]:0.,
				pvecNearest?(*pvecNearest)[4]:0.,
				pvecNearest?(*pvecNearest)[5]:0.);
		}
	}


	// node dragging
	if(m_bMousePressed)
	{
		QGraphicsItem* pCurItem = mouseGrabberItem();
		if(pCurItem)
		{
			const int iNodeType = pCurItem->data(REAL_LATTICE_NODE_TYPE_KEY).toInt();

			// nothing there yet...
		}
	}

	if(!bHandled && bAllowed)
		QGraphicsScene::mouseMoveEvent(pEvt);
}

void LatticeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvt)
{
	m_bMousePressed = 0;

	QGraphicsScene::mouseReleaseEvent(pEvt);
}

void LatticeScene::keyPressEvent(QKeyEvent *pEvt)
{
	if(pEvt->key() == Qt::Key_Control)
		m_bSnap = 1;

	QGraphicsScene::keyPressEvent(pEvt);
}

void LatticeScene::keyReleaseEvent(QKeyEvent *pEvt)
{
	if(pEvt->key() == Qt::Key_Control)
		m_bSnap = 0;

	QGraphicsScene::keyReleaseEvent(pEvt);
}


// --------------------------------------------------------------------------------


LatticeView::LatticeView(QWidget* pParent)
	: QGraphicsView(pParent)
{
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
		QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setMouseTracking(1);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

LatticeView::~LatticeView()
{}

void LatticeView::wheelEvent(QWheelEvent *pEvt)
{
#if QT_VER>=5
	const t_real dDelta = pEvt->angleDelta().y()/8. / 150.;
#else
	const t_real dDelta = pEvt->delta()/8. / 150.;
#endif

	t_real dScale = std::pow(2., dDelta);
	this->scale(dScale, dScale);
	m_dTotalScale *= dScale;
	emit scaleChanged(m_dTotalScale);
}


#include "real_lattice.moc"
