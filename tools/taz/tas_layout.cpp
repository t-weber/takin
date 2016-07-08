/*
 * TAS layout
 * @author tweber
 * @date feb-2014
 * @license GPLv2
 */

#include "tas_layout.h"
#include "tlibs/string/spec_char.h"
#include <iostream>

using t_real = t_real_glob;
using t_vec = ublas::vector<t_real>;
using t_mat = ublas::matrix<t_real>;


static inline bool flip_text(t_real _dAngle)
{
	t_real dAngle = std::fmod(_dAngle, 360.);
	return std::abs(dAngle) > 90. && std::abs(dAngle) < 270.;
}


TasLayoutNode::TasLayoutNode(TasLayout* pSupItem) : m_pParentItem(pSupItem)
{
	setFlag(QGraphicsItem::ItemSendsGeometryChanges);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setCursor(Qt::CrossCursor);
}

QRectF TasLayoutNode::boundingRect() const
{
	return QRectF(-5., -5., 10., 10.);
}

void TasLayoutNode::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->drawEllipse(QRectF(-2., -2., 4., 4.));
}

QVariant TasLayoutNode::itemChange(GraphicsItemChange change, const QVariant &val)
{
	//std::cout << change << std::endl;
	QVariant var = QGraphicsItem::itemChange(change, val);

	if(change == QGraphicsItem::ItemPositionHasChanged)
		m_pParentItem->nodeMoved(this);

	return var;
}

// --------------------------------------------------------------------------------

TasLayout::TasLayout(TasLayoutScene& scene) : m_scene(scene)
{
	setFlag(QGraphicsItem::ItemIgnoresTransformations);

	m_pSrc = new TasLayoutNode(this);
	m_pMono = new TasLayoutNode(this);
	m_pSample = new TasLayoutNode(this);
	m_pAna = new TasLayoutNode(this);
	m_pDet = new TasLayoutNode(this);

	m_pSrc->setToolTip("Source");
	m_pMono->setToolTip("Monochromator");
	m_pSample->setToolTip("Sample");
	m_pAna->setToolTip("Analyser");
	m_pDet->setToolTip("Detector");

	m_pSrc->setPos(200., m_dLenMonoSample*m_dScaleFactor);
	m_pMono->setPos(0., m_dLenMonoSample*m_dScaleFactor);
	m_pSample->setPos(0., 0.);
	m_pAna->setPos(-m_dLenSampleAna*m_dScaleFactor, 0.);
	m_pDet->setPos(-100., -m_dLenAnaDet*m_dScaleFactor);

	AllowMouseMove(1);

	scene.addItem(m_pSrc);
	scene.addItem(m_pMono);
	scene.addItem(m_pSample);
	scene.addItem(m_pAna);
	scene.addItem(m_pDet);

	setAcceptedMouseButtons(0);
	m_bReady = 1;
}

TasLayout::~TasLayout()
{
	m_bReady = 0;

	delete m_pSrc;
	delete m_pMono;
	delete m_pSample;
	delete m_pAna;
	delete m_pDet;
}

void TasLayout::AllowMouseMove(bool bAllow)
{
	//m_pSrc->setFlag(QGraphicsItem::ItemIsMovable, bAllow);
	m_pMono->setFlag(QGraphicsItem::ItemIsMovable, bAllow);
	m_pSample->setFlag(QGraphicsItem::ItemIsMovable, bAllow);
	m_pAna->setFlag(QGraphicsItem::ItemIsMovable, bAllow);
	m_pDet->setFlag(QGraphicsItem::ItemIsMovable, bAllow);
}

void TasLayout::nodeMoved(const TasLayoutNode *pNode)
{
	if(!m_bReady) return;

	static bool bAllowUpdate = 1;
	if(!bAllowUpdate) return;

	const t_vec vecSrc = qpoint_to_vec(mapFromItem(m_pSrc, 0, 0));
	const t_vec vecMono = qpoint_to_vec(mapFromItem(m_pMono, 0, 0));
	const t_vec vecSample = qpoint_to_vec(mapFromItem(m_pSample, 0, 0));
	const t_vec vecAna = qpoint_to_vec(mapFromItem(m_pAna, 0, 0));
	const t_vec vecDet = qpoint_to_vec(mapFromItem(m_pDet, 0, 0));

	bAllowUpdate = 0;
	if(pNode==m_pSample)
	{
		//tl::log_debug("Sample node moved.");

		t_real dTwoTheta = m_dTwoTheta;
		t_real dAnaTwoTheta = m_dAnaTwoTheta;

		t_vec vecSrcMono = vecMono-vecSrc;
		vecSrcMono /= ublas::norm_2(vecSrcMono);

		t_vec vecMonoSample = vecSample-vecMono;
		if(m_bAllowChanges)
			m_dLenMonoSample = ublas::norm_2(vecMonoSample)/m_dScaleFactor;
		vecMonoSample /= ublas::norm_2(vecMonoSample);

		if(m_bAllowChanges)
		{
			m_dMonoTwoTheta = -(tl::vec_angle(vecMonoSample) - tl::vec_angle(vecSrcMono));
			if(m_dMonoTwoTheta < -tl::get_pi<t_real>()) m_dMonoTwoTheta += 2.*tl::get_pi<t_real>();
			if(m_dMonoTwoTheta > tl::get_pi<t_real>()) m_dMonoTwoTheta -= 2.*tl::get_pi<t_real>();
		}


		t_vec vecSampleAna =
				ublas::prod(tl::rotation_matrix_2d(-dTwoTheta), vecMonoSample);
		vecSampleAna /= ublas::norm_2(vecSampleAna);
		vecSampleAna *= m_dLenSampleAna*m_dScaleFactor;

		t_vec vecAnaNew = vecSample + vecSampleAna;
		m_pAna->setPos(vec_to_qpoint(vecAnaNew));



		vecSampleAna /= ublas::norm_2(vecSampleAna);

		t_vec vecAnaDet =
				ublas::prod(tl::rotation_matrix_2d(-dAnaTwoTheta), vecSampleAna);
		vecAnaDet /= ublas::norm_2(vecAnaDet);
		vecAnaDet *= m_dLenAnaDet*m_dScaleFactor;

		m_pDet->setPos(vec_to_qpoint(vecAnaNew+vecAnaDet));


		TriangleOptions opts;
		opts.bChangedMonoTwoTheta = 1;
		opts.bChangedTwoTheta = 1;
		opts.bChangedAnaTwoTheta = 1;
		opts.dMonoTwoTheta = m_dMonoTwoTheta;
		opts.dTwoTheta = dTwoTheta;
		opts.dAnaTwoTheta = dAnaTwoTheta;
		m_scene.emitUpdate(opts);
	}
	else if(pNode==m_pMono)
	{
		//tl::log_debug("Mono node moved.");

		t_vec vecSrcMono = vecMono-vecSrc;
		vecSrcMono /= ublas::norm_2(vecSrcMono);

		t_vec vecMonoSample =
			ublas::prod(tl::rotation_matrix_2d(-m_dMonoTwoTheta), vecSrcMono);
		vecMonoSample /= ublas::norm_2(vecMonoSample);
		vecMonoSample *= m_dLenMonoSample*m_dScaleFactor;

		t_vec vecSampleNew = vecMono + vecMonoSample;
		m_pSample->setPos(vec_to_qpoint(vecSampleNew));


		t_vec vecSampleAna =
			ublas::prod(tl::rotation_matrix_2d(-m_dTwoTheta), vecMonoSample);
		//t_vec vecSampleAna = vecAna - vecSample;
		vecSampleAna /= ublas::norm_2(vecSampleAna);
		vecSampleAna *= m_dLenSampleAna*m_dScaleFactor;

		t_vec vecAnaNew = vecSampleNew + vecSampleAna;
		m_pAna->setPos(vec_to_qpoint(vecAnaNew));


		t_vec vecAnaDet =
			ublas::prod(tl::rotation_matrix_2d(-m_dAnaTwoTheta), vecSampleAna);
		vecAnaDet /= ublas::norm_2(vecAnaDet);
		vecAnaDet *= m_dLenAnaDet*m_dScaleFactor;

		m_pDet->setPos(vec_to_qpoint(vecAnaNew + vecAnaDet));
	}
	else if(pNode==m_pDet)
	{
		//tl::log_debug("Det node moved.");

		t_vec vecSampleAna = vecAna - vecSample;
		vecSampleAna /= ublas::norm_2(vecSampleAna);

		t_vec vecAnaDet = vecDet-vecAna;
		if(m_bAllowChanges)
			m_dLenAnaDet = ublas::norm_2(vecAnaDet)/m_dScaleFactor;
		vecAnaDet /= ublas::norm_2(vecAnaDet);

		if(m_bAllowChanges)
		{
			m_dAnaTwoTheta = -(tl::vec_angle(vecAnaDet) - tl::vec_angle(vecSampleAna));
			if(m_dAnaTwoTheta < -tl::get_pi<t_real>()) m_dAnaTwoTheta += 2.*tl::get_pi<t_real>();
			if(m_dAnaTwoTheta > tl::get_pi<t_real>()) m_dAnaTwoTheta -= 2.*tl::get_pi<t_real>();
		}

		//std::cout << m_dAnaTwoTheta/M_PI*180. << std::endl;

		TriangleOptions opts;
		opts.bChangedAnaTwoTheta = 1;
		opts.dAnaTwoTheta = m_dAnaTwoTheta;
		m_scene.emitUpdate(opts);
	}

	if(/*pNode==m_pMono ||*/ pNode==m_pAna)
	{
		//tl::log_debug("Ana node moved.");

		t_vec vecSampleAna = vecAna-vecSample;
		if(pNode==m_pAna && m_bAllowChanges)
			m_dLenSampleAna = ublas::norm_2(vecSampleAna)/m_dScaleFactor;
		vecSampleAna /= ublas::norm_2(vecSampleAna);

		t_vec vecAnaDet =
			ublas::prod(tl::rotation_matrix_2d(-m_dAnaTwoTheta), vecSampleAna);
		vecAnaDet /= ublas::norm_2(vecAnaDet);
		vecAnaDet *= m_dLenAnaDet*m_dScaleFactor;

		m_pDet->setPos(vec_to_qpoint(vecAna+vecAnaDet));

		t_vec vecMonoSample = vecSample - vecMono;
		vecMonoSample /= ublas::norm_2(vecMonoSample);

		if(m_bAllowChanges)
		{
			m_dTwoTheta = -(tl::vec_angle(vecSampleAna) - tl::vec_angle(vecMonoSample));
			if(m_dTwoTheta < -tl::get_pi<t_real>()) m_dTwoTheta += 2.*tl::get_pi<t_real>();
			if(m_dTwoTheta > tl::get_pi<t_real>()) m_dTwoTheta -= 2.*tl::get_pi<t_real>();
		}

		TriangleOptions opts;
		opts.bChangedTwoTheta = 1;
		opts.dTwoTheta = m_dTwoTheta;
		m_scene.emitUpdate(opts);
	}

	bAllowUpdate = 1;
	this->update();
	m_scene.emitAllParams();
}

QRectF TasLayout::boundingRect() const
{
	return QRectF(-1000.*m_dZoom, -1000.*m_dZoom,
		2000.*m_dZoom, 2000.*m_dZoom);
}

void TasLayout::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const bool bDisplayLengths = 0;
	painter->setFont(g_fontGfx);

	QPointF ptSrc = mapFromItem(m_pSrc, 0, 0) * m_dZoom;
	QPointF ptMono = mapFromItem(m_pMono, 0, 0) * m_dZoom;
	QPointF ptSample = mapFromItem(m_pSample, 0, 0) * m_dZoom;
	QPointF ptAna = mapFromItem(m_pAna, 0, 0) * m_dZoom;
	QPointF ptDet = mapFromItem(m_pDet, 0, 0) * m_dZoom;

	QLineF lineSrcMono(ptSrc, ptMono);
	QLineF lineKi(ptMono, ptSample);
	QLineF lineKf(ptSample, ptAna);
	QLineF lineAnaDet(ptAna, ptDet);

	QPen penOrig = painter->pen();

	painter->drawLine(lineSrcMono);
	painter->drawLine(lineKi);
	painter->drawLine(lineKf);
	painter->drawLine(lineAnaDet);


	// write lengths
	QPointF ptMidKi = ptMono + (ptSample-ptMono)/2.;
	QPointF ptMidKf = ptSample + (ptAna-ptSample)/2.;
	QPointF ptMidAnaDet = ptAna + (ptDet-ptAna)/2.;

	if(bDisplayLengths)
	{
		std::ostringstream ostrLenKi, ostrLenKf, ostrLenAnaDet;
		ostrLenKi.precision(g_iPrecGfx);
		ostrLenKf.precision(g_iPrecGfx);
		ostrLenAnaDet.precision(g_iPrecGfx);

		ostrLenKi << m_dLenMonoSample << " cm";
		ostrLenKf << m_dLenSampleAna << " cm";
		ostrLenAnaDet << m_dLenAnaDet << " cm";

		painter->drawText(ptMidKi, ostrLenKi.str().c_str());
		painter->drawText(ptMidKf, ostrLenKf.str().c_str());
		painter->drawText(ptMidAnaDet, ostrLenAnaDet.str().c_str());
	}



	t_vec vecSrc = qpoint_to_vec(ptSrc);
	t_vec vecMono = qpoint_to_vec(ptMono);
	t_vec vecSample = qpoint_to_vec(ptSample);
	t_vec vecAna = qpoint_to_vec(ptAna);
	t_vec vecDet = qpoint_to_vec(ptDet);

	t_vec vecSrcMono = vecMono-vecSrc;
	t_vec vecMonoSample = vecSample-vecMono;
	t_vec vecSampleAna = vecAna-vecSample;
	t_vec vecAnaDet = vecDet-vecAna;

	t_real dThetas[] = {-m_dMonoTwoTheta/t_real(2.), -m_dAnaTwoTheta/t_real(2.), -m_dTheta};
	std::vector<const t_vec*> vecPos = {&vecMono, &vecAna, &vecSample};
	std::vector<const t_vec*> vecDirs = {&vecSrcMono, &vecSampleAna, &vecMonoSample};
	QColor colThs[] = {Qt::gray, Qt::gray, Qt::red};
	const char* pcComp[] = {"M", "A", "S"};

	QLineF lineRot[3];
	QPointF ptThP[3];

	// mono/ana/sample theta rotation
	for(std::size_t iTh=0; iTh<sizeof(dThetas)/sizeof(*dThetas); ++iTh)
	{
		t_vec vecRotDir =
			ublas::prod(tl::rotation_matrix_2d(dThetas[iTh]), *vecDirs[iTh]);
		vecRotDir /= ublas::norm_2(vecRotDir);
		vecRotDir *= m_dLenSample*m_dScaleFactor;

		QPointF ptThM = vec_to_qpoint(*vecPos[iTh] - vecRotDir*m_dZoom);
		ptThP[iTh] = vec_to_qpoint(*vecPos[iTh] + vecRotDir*m_dZoom);
		lineRot[iTh] = QLineF(ptThM, ptThP[iTh]);

		QPen pen(colThs[iTh]);
		pen.setWidthF(1.5);
		painter->setPen(pen);
		painter->drawLine(lineRot[iTh]);


		// component names
		painter->setPen(penOrig);
		painter->save();
			painter->translate(vec_to_qpoint(*vecPos[iTh]));
			t_real dCompAngle = 180. + tl::r2d(tl::vec_angle(vecRotDir));
			painter->rotate(dCompAngle);
			painter->translate(-4., 16.);
			if(flip_text(dCompAngle))
			{
				painter->translate(4., -8.);
				painter->rotate(180.);
			}
			painter->drawText(QPointF(0., 0.), pcComp[iTh]);
		painter->restore();
	}

	painter->drawText(ptMono - vec_to_qpoint(vecSrcMono*1.1), "R");
	painter->drawText(ptAna + vec_to_qpoint(vecAnaDet*1.1), "D");


	// dashed extended lines
	painter->setPen(Qt::DashLine);
	QLineF lineSrcMono_ext(ptMono, ptMono + (ptMono-ptSrc)/2.);
	QLineF lineki_ext(ptSample, ptSample + (ptSample-ptMono)/2.);
	QLineF linekf_ext(ptAna, ptAna + (ptAna-ptSample)/2.);

	painter->drawLine(lineSrcMono_ext);
	painter->drawLine(lineki_ext);
	painter->drawLine(linekf_ext);



	QLineF *plineQ = nullptr;
	QPointF *pptQ = nullptr;
	// Q vector direction visible?
	if(this->m_bRealQVisible)
	{
		//log_info("angle kiQ: ", m_dAngleKiQ/M_PI*180.);
		const t_real &dAngleKiQ = m_dAngleKiQ;
		t_mat matRotQ = tl::rotation_matrix_2d(dAngleKiQ);
		t_vec vecKi = vecSample-vecMono;
		t_vec vecQ = ublas::prod(matRotQ, vecKi);
		vecQ /= ublas::norm_2(vecQ);
		vecQ *= (m_dLenMonoSample + m_dLenSampleAna)/2.;	// some arbitrary length
		vecQ *= m_dScaleFactor * m_dZoom;

		pptQ = new QPointF(vec_to_qpoint(vecSample + vecQ));
		plineQ = new QLineF(ptSample, *pptQ);

		painter->setPen(Qt::red);
		painter->drawLine(*plineQ);
		painter->save();
			painter->translate(ptSample);
			const t_real dQAngle = -plineQ->angle();
			painter->rotate(dQAngle);
			painter->translate(QPointF(plineQ->length()/2.,12.));
			if(flip_text(dQAngle))
				painter->rotate(180.);
			painter->drawText(QPointF(0,0), "Q");
		painter->restore();
	}
	painter->setPen(penOrig);


	// angle arcs
	const QLineF* pLines1[] = {&lineSrcMono, &lineKi, &lineKf, &lineRot[2]};
	const QLineF* pLines2[] = {&lineKi, &lineKf, &lineAnaDet, &lineKi};
	const QPointF* pPoints[] = {&ptMono, &ptSample, &ptAna, &ptSample};
	const QPointF* pPoints_ext[] = {&ptSrc, &ptMono, &ptSample, &ptThP[2]};
	const t_real dAngles[] = {m_dMonoTwoTheta, m_dTwoTheta, m_dAnaTwoTheta, -m_dTheta};
	const t_real dAngleOffs[] = {0., 0., 0., 180.};

	QPen pen1(Qt::blue);
	QPen pen2(Qt::red);
	QPen* arcPens[] = {&pen1, &pen1, &pen1, &pen2};

	const std::wstring& strDEG = tl::get_spec_char_utf16("deg");

	for(std::size_t i=0; i<sizeof(pPoints)/sizeof(*pPoints); ++i)
	{
		t_real dArcSize = (pLines1[i]->length() + pLines2[i]->length()) / 2. / 3.;
		t_real dBeginArcAngle = pLines1[i]->angle() + dAngleOffs[i];
		t_real dArcAngle = tl::r2d(dAngles[i]);

		std::wostringstream ostrAngle;
		ostrAngle.precision(g_iPrecGfx);
		if(!tl::is_nan_or_inf<t_real>(dArcAngle))
		{
			ostrAngle << std::fabs(dArcAngle) << strDEG;
		}
		else
		{
			dArcAngle = t_real(180);
			ostrAngle << "invalid";
		}

		painter->setPen(*arcPens[i]);
		painter->drawArc(QRectF(pPoints[i]->x()-dArcSize/2., pPoints[i]->y()-dArcSize/2.,
			dArcSize, dArcSize), dBeginArcAngle*16., dArcAngle*16.);

		bool bFlip = dAngleOffs[i] > 90.;
		t_real dTotalAngle = -dBeginArcAngle-dArcAngle*0.5 + 180.;
		if(bFlip) dTotalAngle += 180.;
		t_real dTransScale = bFlip ? -40. : 80.;
		dTransScale *= m_dZoom;
		painter->save();
			painter->translate(*pPoints[i]);
			painter->rotate(dTotalAngle);
			painter->translate(-dTransScale, +4.);
			if(flip_text(dTotalAngle))
			{
				if(bFlip)
					painter->translate(-dTransScale, -8.);
				else
					painter->translate(dTransScale*0.5, -8.);
				painter->rotate(180.);
			}
			painter->drawText(QPointF(0.,0.), QString::fromWCharArray(ostrAngle.str().c_str()));
		painter->restore();
	}

	painter->setPen(penOrig);


	// arrow heads
	const QLineF* pLines_arrow[] = {&lineKi, &lineKf, plineQ, &lineSrcMono, &lineAnaDet};
	const QPointF* pPoints_arrow[] = {&ptSample, &ptAna, pptQ, &ptMono, &ptDet};
	QColor colArrowHead[] = {Qt::black, Qt::black, Qt::red, Qt::gray, Qt::gray};
	for(std::size_t i=0; i<sizeof(pLines_arrow)/sizeof(*pLines_arrow); ++i)
	{
		if(!pLines_arrow[i] || !pPoints_arrow[i])
			continue;

		t_real dAng = tl::d2r(pLines_arrow[i]->angle() - 90.);
		t_real dC = std::cos(dAng);
		t_real dS = std::sin(dAng);

		t_real dTriagX = 5., dTriagY = 10.;
		QPointF ptTriag1 = *pPoints_arrow[i] + QPointF(dTriagX*dC + dTriagY*dS, -dTriagX*dS + dTriagY*dC);
		QPointF ptTriag2 = *pPoints_arrow[i] + QPointF(-dTriagX*dC + dTriagY*dS, dTriagX*dS + dTriagY*dC);

		QPainterPath triag;
		triag.moveTo(*pPoints_arrow[i]);
		triag.lineTo(ptTriag1);
		triag.lineTo(ptTriag2);

		painter->setPen(colArrowHead[i]);
		painter->fillPath(triag, colArrowHead[i]);
	}

	painter->setPen(penOrig);

	if(plineQ) delete plineQ;
	if(pptQ) delete pptQ;
}


void TasLayout::SetSampleTwoTheta(t_real dAngle)
{
	m_dTwoTheta = dAngle;
	//std::cout << m_dTwoTheta/M_PI*180. << std::endl;

	t_vec vecMono = qpoint_to_vec(mapFromItem(m_pMono, 0, 0));
	t_vec vecSample = qpoint_to_vec(mapFromItem(m_pSample, 0, 0));
	t_vec vecAna = qpoint_to_vec(mapFromItem(m_pAna, 0, 0));

	t_vec vecKi = vecSample - vecMono;
	vecKi /= ublas::norm_2(vecKi);
	t_real dLenKf = ublas::norm_2(vecAna-vecSample);

	//std::cout << dAngle/M_PI*180. << std::endl;
	t_vec vecKf = ublas::prod(tl::rotation_matrix_2d(-dAngle), vecKi);
	vecKf /= ublas::norm_2(vecKf);
	vecKf *= dLenKf;

	m_pAna->setPos(vec_to_qpoint(vecSample + vecKf));

	nodeMoved(m_pSample);
	nodeMoved(m_pAna);
}

void TasLayout::SetSampleTheta(t_real dAngle)
{
	m_dTheta = dAngle;
	nodeMoved();
}

void TasLayout::SetMonoTwoTheta(t_real dAngle)
{
	m_dMonoTwoTheta = dAngle;
	nodeMoved(m_pMono);
}

void TasLayout::SetAnaTwoTheta(t_real dAngle)
{
	m_dAnaTwoTheta = dAngle;
	nodeMoved(m_pAna);
}

void TasLayout::SetAngleKiQ(t_real dAngle)
{
	m_dAngleKiQ = dAngle;
	nodeMoved();
}

void TasLayout::SetRealQVisible(bool bVisible)
{
	m_bRealQVisible = bVisible;
	this->update();
}

void TasLayout::SetZoom(t_real dZoom)
{
	m_dZoom = dZoom;
	m_scene.update();
}

std::vector<TasLayoutNode*> TasLayout::GetNodes()
{
	return std::vector<TasLayoutNode*>
			{ m_pSrc, m_pMono, m_pSample, m_pAna, m_pDet };
}

std::vector<std::string> TasLayout::GetNodeNames() const
{
	return std::vector<std::string>
		{ "source", "monochromator", "sample", "analyser", "detector" };
}

// --------------------------------------------------------------------------------


TasLayoutScene::TasLayoutScene(QObject *pParent) : QGraphicsScene(pParent)
{
	m_pTas = new TasLayout(*this);
	this->addItem(m_pTas);
}

TasLayoutScene::~TasLayoutScene()
{
	delete m_pTas;
}

void TasLayoutScene::emitAllParams()
{
	if(!m_pTas || !m_pTas->IsReady() || m_bDontEmitChange)
		return;

	RealParams parms;
	parms.dAnaTT = m_pTas->GetAnaTwoTheta();
	parms.dAnaT = m_pTas->GetAnaTheta();
	parms.dMonoTT = m_pTas->GetMonoTwoTheta();
	parms.dMonoT = m_pTas->GetMonoTheta();
	parms.dSampleTT = m_pTas->GetSampleTwoTheta();
	parms.dSampleT = m_pTas->GetSampleTheta();

	parms.dLenMonoSample = m_pTas->GetLenMonoSample();
	parms.dLenSampleAna = m_pTas->GetLenSampleAna();
	parms.dLenAnaDet = m_pTas->GetLenAnaDet();

	//log_info(parms.dSampleT/M_PI*180.);
	//log_debug("tas: emitAllParams");
	emit paramsChanged(parms);
}

void TasLayoutScene::triangleChanged(const TriangleOptions& opts)
{
	if(!m_pTas || !m_pTas->IsReady())
		return;

	//tl::log_debug("triangle changed");
	m_bDontEmitChange = 1;
	m_pTas->AllowChanges(0);

	if(opts.bChangedMonoTwoTheta)
		m_pTas->SetMonoTwoTheta(opts.dMonoTwoTheta);
	if(opts.bChangedAnaTwoTheta)
		m_pTas->SetAnaTwoTheta(opts.dAnaTwoTheta);
	if(opts.bChangedTheta)
		m_pTas->SetSampleTheta(opts.dTheta);
	if(opts.bChangedTwoTheta)
		m_pTas->SetSampleTwoTheta(opts.dTwoTheta);

	//if(opts.bChangedAngleKiQ)
	//	m_pTas->SetAngleKiQ(opts.dAngleKiQ);

	m_pTas->AllowChanges(1);
	m_bDontEmitChange = 0;
}

void TasLayoutScene::recipParamsChanged(const RecipParams& params)
{
	m_pTas->SetAngleKiQ(params.dKiQ);
}

void TasLayoutScene::emitUpdate(const TriangleOptions& opts)
{
	if(!m_pTas || !m_pTas->IsReady() || m_bDontEmitChange)
		return;

	emit tasChanged(opts);
}

void TasLayoutScene::scaleChanged(t_real dTotalScale)
{
	if(!m_pTas) return;
	m_pTas->SetZoom(dTotalScale);
}

void TasLayoutScene::mousePressEvent(QGraphicsSceneMouseEvent *pEvt)
{
	QGraphicsScene::mousePressEvent(pEvt);
	emit nodeEvent(1);
}

void TasLayoutScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvt)
{
	QGraphicsScene::mouseReleaseEvent(pEvt);
	emit nodeEvent(0);
}


// --------------------------------------------------------------------------------


TasLayoutView::TasLayoutView(QWidget* pParent) : QGraphicsView(pParent)
{
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
		QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

TasLayoutView::~TasLayoutView()
{}

void TasLayoutView::wheelEvent(QWheelEvent *pEvt)
{
#if QT_VER>=5
	const t_real dDelta = pEvt->angleDelta().y()/8. / 150.;
#else
	const t_real dDelta = pEvt->delta()/8. / 150.;
#endif

	const t_real dScale = std::pow(2., dDelta);
	this->scale(dScale, dScale);
	m_dTotalScale *= dScale;
	emit scaleChanged(m_dTotalScale);
}


#include "tas_layout.moc"
