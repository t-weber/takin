/**
 * gl plotter
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 19-may-2013
 * @license GPLv2
 */

#include "plotgl.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/math.h"
#include "tlibs/string/string.h"
#include "tlibs/helper/flags.h"

#include <glu.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <numeric>

#define RENDER_FPS 40


using t_real = t_real_glob;
using t_mat4 = tl::t_mat4_gen<t_real>;
using t_mat3 = tl::t_mat3_gen<t_real>;
using t_vec4 = tl::t_vec4_gen<t_real>;
using t_vec3 = tl::t_vec3_gen<t_real>;


#if QT_VER>=5
	#define POS_F localPos
#else
	#define POS_F posF
#endif


// ----------------------------------------------------------------------------


static inline void sleep_nano(long ns)
{
	timespec ts;
	ts.tv_nsec = ns;
	ts.tv_sec = 0;
	nanosleep(&ts, 0);
}


// ----------------------------------------------------------------------------


PlotGl::PlotGl(QWidget* pParent, QSettings *pSettings, t_real dMouseScale)
	: t_qglwidget(pParent), m_pSettings(pSettings),
		m_bEnabled(true), m_mutex(QMutex::Recursive), m_mutex_resize(QMutex::Recursive),
		m_matProj(tl::unit_matrix<t_mat4>(4)), m_matView(tl::unit_matrix<t_mat4>(4))
{
	QGLFormat form = format();
	form.setSampleBuffers(1);
	setFormat(form);

	m_dMouseRot[0] = m_dMouseRot[1] = 0.;
	m_dMouseScale = dMouseScale;
	updateViewMatrix();

	setAutoBufferSwap(false);
	//setUpdatesEnabled(0);
	doneCurrent();
#if QT_VER>=5
	context()->moveToThread((QThread*)this);
#endif
	start();		// render thread
}

PlotGl::~PlotGl()
{
	SetEnabled(0);
	m_bRenderThreadActive = 0;
	wait();
}


// ----------------------------------------------------------------------------


void PlotGl::SetEnabled(bool b)
{
	m_bEnabled.store(b);
}

void PlotGl::SetColor(t_real r, t_real g, t_real b, t_real a)
{
	t_real pfCol[] = {r, g, b, a};
	tl::gl_traits<t_real>::SetMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, pfCol);
}

void PlotGl::SetColor(std::size_t iIdx)
{
	static const t_real cols[4][4] =
	{
		{ 0., 0., 1., 0.7 },
		{ 0., 0.5, 0., 0.7 },
		{ 1., 0., 0., 0.7 },
		{ 0., 0., 0., 0.7 }
	};

	tl::gl_traits<t_real>::SetMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols[iIdx % 4]);
}


void PlotGl::initializeGLThread()
{
	glClearColor(1.,1.,1.,0.);
	glShadeModel(GL_SMOOTH);
	//glShadeModel(GL_FLAT);

	glClearDepth(1.);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	//glEnable(GL_POLYGON_SMOOTH);
#ifdef USE_MULTI_TEXTURES
	glEnable(GL_MULTISAMPLE);
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
#endif
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	const t_real vecLightColDiff[] = { 1., 1., 1., 1. };
	const t_real vecLightColAmb[] = { 0.25, 0.25, 0.25, 1. };
	const t_real vecLight0[] = { 1., 1., 1., 0. };
	tl::gl_traits<t_real>::SetLight(GL_LIGHT0, GL_AMBIENT, vecLightColAmb);
	tl::gl_traits<t_real>::SetLight(GL_LIGHT0, GL_DIFFUSE, vecLightColDiff);
	tl::gl_traits<t_real>::SetLight(GL_LIGHT0, GL_POSITION, vecLight0);


	unsigned int iLOD = 32;
	for(std::size_t iSphere=0; iSphere<sizeof(m_iLstSphere)/sizeof(*m_iLstSphere); ++iSphere)
	{
		m_iLstSphere[iSphere] = glGenLists(1);

		GLUquadricObj *pQuadSphere = gluNewQuadric();
		gluQuadricDrawStyle(pQuadSphere, GLU_FILL /*GLU_LINE*/);
		gluQuadricNormals(pQuadSphere, GLU_SMOOTH);
		glNewList(m_iLstSphere[iSphere], GL_COMPILE);
			gluSphere(pQuadSphere, 1., iLOD, iLOD);
		glEndList();
		gluDeleteQuadric(pQuadSphere);

		iLOD *= 0.8;
	}

#ifdef USE_MULTI_TEXTURES
	glActiveTexture(GL_TEXTURE0);
#endif

	if(g_strFontGL == "") g_strFontGL = DEF_FONT;
	if(g_iFontGLSize <= 0) g_iFontGLSize = DEF_FONT_SIZE;
	m_pFont = new tl::GlFontMap<t_real>(g_strFontGL.c_str(), g_iFontGLSize);

#if QT_VER>=5
	QWidget::
#endif
	setMouseTracking(1);
}


void PlotGl::freeGLThread()
{
	SetEnabled(0);
#if QT_VER>=5
	QWidget::
#endif
	setMouseTracking(0);

	for(std::size_t iSphere=0; iSphere<sizeof(m_iLstSphere)/sizeof(*m_iLstSphere); ++iSphere)
		glDeleteLists(m_iLstSphere[iSphere], 1);

	if(m_pFont) { delete m_pFont; m_pFont = nullptr; }
}


void PlotGl::resizeGLThread(int w, int h)
{
	makeCurrent();

	if(w <= 0) w = 1;
	if(h <= 0) h = 1;
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	if(m_bPerspective)
		m_matProj = tl::perspective_matrix(m_dFOV, t_real(w)/t_real(h), 0.1, 100.);
	else
		m_matProj = tl::ortho_matrix(-1.,1.,-1.,1.,0.1,100.);
	t_real glmat[16]; tl::to_gl_array(m_matProj, glmat);
	glLoadMatrixd(glmat);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/**
 * average distance of object from camera position
 */
t_real PlotGl::GetCamObjDist(const PlotObjGl& obj) const
{
	ublas::vector<t_real> vecPos;
	t_real dShiftPt = 0.;

	if(obj.plttype == PLOT_POLY || obj.plttype == PLOT_LINES)
	{
		// mean position
		vecPos = tl::mean_value(obj.vecVertices);
		//if(obj.plttype == PLOT_LINES)
		//	dShiftPt = obj.dLineWidth*0.25;
	}
	else if(obj.plttype == PLOT_SPHERE || obj.plttype == PLOT_ELLIPSOID)
	{
		vecPos = obj.vecPos;
		// point on sphere closest to camera
		dShiftPt = ublas::norm_2(obj.vecScale);
	}
	else
	{
		return t_real(0);
	}

	// shift position towards camera
	if(!tl::float_equal(dShiftPt, t_real(0)))
	{
		ublas::vector<t_real> vecCamObj = m_vecCam - vecPos;
		vecCamObj /= ublas::norm_2(vecCamObj);
		vecPos += vecCamObj*dShiftPt;
	}

	if(vecPos.size() != m_vecCam.size())
		return t_real(0);
	return ublas::norm_2(vecPos - m_vecCam);
}


/**
 * get sorted object indices based on distance to camera
 */
std::vector<std::size_t> PlotGl::GetObjSortOrder() const
{
	std::vector<std::size_t> vecIdx(m_vecObjs.size());
	std::iota(vecIdx.begin(), vecIdx.end(), 0);

	// sort indices based on obj distance
	std::stable_sort(vecIdx.begin(), vecIdx.end(),
		[this](const std::size_t& iIdx0, const std::size_t& iIdx1) -> bool
		{
			if(iIdx0 < m_vecObjs.size() && iIdx1 < m_vecObjs.size())
			{
				t_real tDist0 = GetCamObjDist(m_vecObjs[iIdx0]);
				t_real tDist1 = GetCamObjDist(m_vecObjs[iIdx1]);
				return m_bDoZTest ? tDist0 < tDist1 : tDist0 >= tDist1;
			}
			else
			{
				tl::log_err("Possible corruption in GL object list.");
				return false;
			}
		});

	return vecIdx;
}


/**
 * advance time
 */
void PlotGl::tickThread(t_real dTime)
{}


/**
 * main render function
 */
void PlotGl::paintGLThread()
{
	makeCurrent();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bool bEnabled = m_bEnabled.load();
	if(!bEnabled) return;

	glMatrixMode(GL_MODELVIEW);
	t_real glmat[16];
	{
		std::lock_guard<QMutex> _lck(m_mutex);
		tl::to_gl_array(m_matView, glmat);
	}
	glLoadMatrixd(glmat);


	// camera position
	tl::Line<t_real> rayMid = tl::screen_ray(t_real(0.5), t_real(0.5), m_matProj, m_matView);
	m_vecCam = rayMid.GetX0();


	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDisable(GL_TEXTURE_2D);

	if(m_bDoZTest)
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
	}


	// draw axes
	glPushMatrix();
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glLineWidth(2.);
		tl::gl_traits<t_real>::SetColor(0., 0., 0.);

		const t_real dAxisScale = 1.8;
		glBegin(GL_LINES);
			tl::gl_traits<t_real>::SetVertex(m_dXMin*dAxisScale, 0., 0.);
			tl::gl_traits<t_real>::SetVertex(m_dXMax*dAxisScale, 0., 0.);
			tl::gl_traits<t_real>::SetVertex(0., m_dYMin*dAxisScale, 0.);
			tl::gl_traits<t_real>::SetVertex(0., m_dYMax*dAxisScale, 0.);
			tl::gl_traits<t_real>::SetVertex(0., 0., m_dZMin*dAxisScale);
			tl::gl_traits<t_real>::SetVertex(0., 0., m_dZMax*dAxisScale);
		glEnd();
	glPopAttrib();
	glPopMatrix();


	std::unique_lock<QMutex> _lck(m_mutex);

	// draw objects
	for(std::size_t iObjIdx : GetObjSortOrder())
	{
		if(iObjIdx >= m_vecObjs.size())
			continue;
		const PlotObjGl& obj = m_vecObjs[iObjIdx];

		int iLOD = 0;
		bool bIsSphereLikeObj = 0;

		bool bColorSet = 0;
		if(obj.bSelected)
		{
			SetColor(0.25, 0.25, 0.25, 0.9);
			bColorSet = 1;
		}
		else
		{
			if(obj.vecColor.size())
				SetColor(obj.vecColor[0], obj.vecColor[1], obj.vecColor[2], obj.vecColor[3]);
			else
				SetColor(iObjIdx);
		}


		glPushMatrix();
		if(obj.plttype == PLOT_SPHERE)
		{
			tl::gl_traits<t_real>::SetTranslate(obj.vecPos[0], obj.vecPos[1], obj.vecPos[2]);
			tl::gl_traits<t_real>::SetScale(obj.vecScale[0], obj.vecScale[0], obj.vecScale[0]);

			bIsSphereLikeObj = 1;
		}
		else if(obj.plttype == PLOT_ELLIPSOID)
		{
			tl::gl_traits<t_real>::SetTranslate(obj.vecPos[0], obj.vecPos[1], obj.vecPos[2]);

			t_real dMatRot[] = {obj.vecRotMat[0], obj.vecRotMat[1], obj.vecRotMat[2], 0.,
				obj.vecRotMat[3], obj.vecRotMat[4], obj.vecRotMat[5], 0.,
				obj.vecRotMat[6], obj.vecRotMat[7], obj.vecRotMat[8], 0.,
				0., 0., 0., 1. };
			glMultMatrixd(dMatRot);
			tl::gl_traits<t_real>::SetScale(obj.vecScale[0], obj.vecScale[1], obj.vecScale[2]);

			bIsSphereLikeObj = 1;
		}
		else if(obj.plttype == PLOT_POLY)
		{
			glBegin(GL_POLYGON);
				tl::gl_traits<t_real>::SetNorm(obj.vecNorm[0], obj.vecNorm[1], obj.vecNorm[2]);
				for(const ublas::vector<t_real>& vec : obj.vecVertices)
					tl::gl_traits<t_real>::SetVertex(vec[0], vec[1], vec[2]);
			glEnd();
		}
		else if(obj.plttype == PLOT_LINES)
		{
			glLineWidth(obj.dLineWidth);
			glBegin(GL_LINE_LOOP);
				for(const ublas::vector<t_real>& vec : obj.vecVertices)
					tl::gl_traits<t_real>::SetVertex(vec[0], vec[1], vec[2]);
 			glEnd();
		}
		else
		{
			tl::log_warn("Unknown plot object.");
		}


		if(bIsSphereLikeObj)
		{
			if(obj.bUseLOD)
			{
				t_real dLenDist = tl::gl_proj_sphere_size(/*dRadius*/1.);
				iLOD = dLenDist * 50.;

				if(iLOD >= int(sizeof(m_iLstSphere)/sizeof(*m_iLstSphere)))
					iLOD = sizeof(m_iLstSphere)/sizeof(*m_iLstSphere)-1;
				if(iLOD < 0)
					iLOD = 0;

				iLOD = sizeof(m_iLstSphere)/sizeof(*m_iLstSphere) - iLOD - 1;
			}

			glCallList(m_iLstSphere[iLOD]);
		}


		// draw label of selected object
		if(obj.bSelected && obj.strLabel.length() && m_pFont && m_pFont->IsOk())
		{
			glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT);

			m_pFont->BindTexture();
			tl::gl_traits<t_real>::SetColor(0., 0., 0., 1.);
			m_pFont->DrawText(0., 0., 0., obj.strLabel);

			glPopAttrib();
		}
		glPopMatrix();
	}
	_lck.unlock();


	// draw axis labels
	if(m_pFont && m_pFont->IsOk())
	{
		glPushMatrix();
		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT);
		m_pFont->BindTexture();

		tl::gl_traits<t_real>::SetColor(0., 0., 1., 1.);
		m_pFont->DrawText(m_dXMax*dAxisScale, 0., 0., m_strLabels[0].toStdString());
		m_pFont->DrawText(0., m_dYMax*dAxisScale , 0., m_strLabels[1].toStdString());
		m_pFont->DrawText(0., 0., m_dZMax*dAxisScale , m_strLabels[2].toStdString());

		if(m_bDrawMinMax)
		{
			tl::gl_traits<t_real>::SetColor(0., 0., 0., 1.);
			m_pFont->DrawText(m_dXMin, 0., 0., tl::var_to_str(m_dXMin+m_dXMinMaxOffs, m_iPrec));
			m_pFont->DrawText(m_dXMax, 0., 0., tl::var_to_str(m_dXMax+m_dXMinMaxOffs, m_iPrec));
			m_pFont->DrawText(0., m_dYMin, 0., tl::var_to_str(m_dYMin+m_dYMinMaxOffs, m_iPrec));
			m_pFont->DrawText(0., m_dYMax, 0., tl::var_to_str(m_dYMax+m_dYMinMaxOffs, m_iPrec));
			m_pFont->DrawText(0., 0., m_dZMin, tl::var_to_str(m_dZMin+m_dZMinMaxOffs, m_iPrec));
			m_pFont->DrawText(0., 0., m_dZMax, tl::var_to_str(m_dZMax+m_dZMinMaxOffs, m_iPrec));
		}
		glPopAttrib();
		glPopMatrix();
	}

	swapBuffers();
}


void PlotGl::run()
{
	static std::atomic<std::size_t> iThread(0);
	std::size_t iThisThread = ++iThread;
	tl::log_debug("GL thread ", iThisThread, " started.");

	makeCurrent();
	initializeGLThread();

	t_real dTime = 0.;
	while(m_bRenderThreadActive)
	{
		if(m_size.bDoResize)
		{
			// mutex for protection of the matrix modes
			std::lock_guard<QMutex> _lck(m_mutex_resize);
			resizeGLThread(m_size.iW, m_size.iH);

			m_size.bDoResize = false;
		}

		if(isVisible())
		{
			std::lock_guard<QMutex> _lck(m_mutex_resize);
			tickThread(dTime);
			paintGLThread();
		}

		long fps = isVisible() ? RENDER_FPS : (RENDER_FPS/10);
		long lns = long(1e9) / fps;
		sleep_nano(lns);
		dTime += t_real(lns) * 1e-9;
	}

	doneCurrent();
	freeGLThread();
	tl::log_debug("GL thread ", iThisThread, " ended.");
}


// ----------------------------------------------------------------------------


void PlotGl::paintEvent(QPaintEvent *)
{}

void PlotGl::resizeEvent(QResizeEvent *pEvt)
{
	if(!pEvt) return;

	std::lock_guard<QMutex> _lck(m_mutex_resize);
	m_size.iW = pEvt->size().width();
	m_size.iH = pEvt->size().height();
	m_size.bDoResize = true;
}


// ----------------------------------------------------------------------------

void PlotGl::clear()
{
	std::lock_guard<QMutex> _lck(m_mutex);
	m_vecObjs.clear();
}

void PlotGl::SetObjectColor(std::size_t iObjIdx, const std::vector<t_real>& vecCol)
{
	std::lock_guard<QMutex> _lck(m_mutex);

	if(m_vecObjs.size() <= iObjIdx)
		return;
	m_vecObjs[iObjIdx].vecColor = vecCol;
}

void PlotGl::SetObjectLabel(std::size_t iObjIdx, const std::string& strLab)
{
	std::lock_guard<QMutex> _lck(m_mutex);

	if(m_vecObjs.size() <= iObjIdx)
		return;
	m_vecObjs[iObjIdx].strLabel = strLab;
}

void PlotGl::SetObjectUseLOD(std::size_t iObjIdx, bool bLOD)
{
	std::lock_guard<QMutex> _lck(m_mutex);

	if(m_vecObjs.size() <= iObjIdx)
		return;
	m_vecObjs[iObjIdx].bUseLOD = bLOD;
}


// ----------------------------------------------------------------------------


void PlotGl::PlotSphere(const ublas::vector<t_real>& vecPos,
	t_real dRadius, int iObjIdx)
{
	if(iObjIdx < 0)
	{
		clear();
		iObjIdx = 0;
	}

	std::lock_guard<QMutex> _lck(m_mutex);

	if(iObjIdx >= int(m_vecObjs.size()))
		m_vecObjs.resize(iObjIdx+1);
	PlotObjGl& obj = m_vecObjs[iObjIdx];

	obj.plttype = PLOT_SPHERE;
	obj.vecPos = vecPos;
	obj.vecScale = tl::make_vec<ublas::vector<t_real>>({ dRadius, dRadius, dRadius });
}


void PlotGl::PlotEllipsoid(const ublas::vector<t_real>& widths,
	const ublas::vector<t_real>& offsets, const ublas::matrix<t_real>& rot,
	int iObjIdx)
{
	if(iObjIdx < 0)
	{
		clear();
		iObjIdx = 0;
	}

	std::lock_guard<QMutex> _lck(m_mutex);

	if(iObjIdx >= int(m_vecObjs.size()))
		m_vecObjs.resize(iObjIdx+1);
	PlotObjGl& obj = m_vecObjs[iObjIdx];

	obj.plttype = PLOT_ELLIPSOID;
	obj.vecScale = tl::make_vec<ublas::vector<t_real>>({ widths[0], widths[1], widths[2] });
	obj.vecPos = offsets;

	if(obj.vecRotMat.size() != 9)
		obj.vecRotMat.resize(9);

	std::size_t iNum = 0;
	for(std::size_t i=0; i<3; ++i)
		for(std::size_t j=0; j<3; ++j)
			obj.vecRotMat[iNum++] = rot(j,i);
}


void PlotGl::PlotPoly(const std::vector<ublas::vector<t_real>>& vecVertices,
	const ublas::vector<t_real>& vecNorm, int iObjIdx)
{
	if(iObjIdx < 0)
	{
		clear();
		iObjIdx = 0;
	}

	std::lock_guard<QMutex> _lck(m_mutex);

	if(iObjIdx >= int(m_vecObjs.size()))
		m_vecObjs.resize(iObjIdx+1);
	PlotObjGl& obj = m_vecObjs[iObjIdx];

	obj.plttype = PLOT_POLY;
	obj.vecVertices = vecVertices;
	obj.vecNorm = vecNorm;
}


void PlotGl::PlotLines(const std::vector<ublas::vector<t_real>>& vecVertices,
	t_real_glob dLW, int iObjIdx)
{
	if(iObjIdx < 0)
	{
		clear();
		iObjIdx = 0;
	}

	std::lock_guard<QMutex> _lck(m_mutex);

	if(iObjIdx >= int(m_vecObjs.size()))
		m_vecObjs.resize(iObjIdx+1);
	PlotObjGl& obj = m_vecObjs[iObjIdx];

	obj.plttype = PLOT_LINES;
	obj.vecVertices = vecVertices;
	obj.dLineWidth = dLW;
}


// ----------------------------------------------------------------------------


void PlotGl::mousePressEvent(QMouseEvent *event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		m_bMouseRotateActive = 1;
		m_dMouseBegin[0] = event->POS_F().x();
		m_dMouseBegin[1] = event->POS_F().y();
	}

	if(event->buttons() & Qt::RightButton)
	{
		m_bMouseScaleActive = 1;
		m_dMouseScaleBegin = event->POS_F().y();
	}
}

void PlotGl::mouseReleaseEvent(QMouseEvent *event)
{
	if((event->buttons() & Qt::LeftButton) == 0)
		m_bMouseRotateActive = 0;

	if((event->buttons() & Qt::RightButton) == 0)
		m_bMouseScaleActive = 0;
}

void PlotGl::mouseMoveEvent(QMouseEvent *pEvt)
{
	bool bUpdateView = 0;
	if(m_bMouseRotateActive)
	{
		t_real dNewX = t_real(pEvt->POS_F().x());
		t_real dNewY = t_real(pEvt->POS_F().y());

		m_dMouseRot[0] += dNewX - m_dMouseBegin[0];
		m_dMouseRot[1] += dNewY - m_dMouseBegin[1];

		m_dMouseBegin[0] = dNewX;
		m_dMouseBegin[1] = dNewY;

		bUpdateView = 1;
	}

	if(m_bMouseScaleActive)
	{
		t_real dNewY = t_real(pEvt->POS_F().y());

		m_dMouseScale *= 1.-(dNewY - m_dMouseScaleBegin)/t_real(height()) * 2.;
		m_dMouseScaleBegin = dNewY;

		bUpdateView = 1;
	}

	if(bUpdateView)
		updateViewMatrix();


	t_real dMouseX = 2.*pEvt->POS_F().x()/t_real(m_size.iW) - 1.;
	t_real dMouseY = -(2.*pEvt->POS_F().y()/t_real(m_size.iH) - 1.);

	bool bHasSelected = 0;
	if(m_bEnabled.load())
	{
		mouseSelectObj(dMouseX, dMouseY);

		for(PlotObjGl& obj : m_vecObjs)
		{
			if(obj.bSelected)
			{
				m_sigHover(&obj);
				bHasSelected = 1;
				break;
			}
		}
	}
	if(!bHasSelected)
		m_sigHover(nullptr);
}


void PlotGl::wheelEvent(QWheelEvent* pEvt)
{
#if QT_VER>=5
	const t_real dDelta = pEvt->angleDelta().y()/8. / 150.;
#else
	const t_real dDelta = pEvt->delta()/8. / 150.;
#endif

	m_dMouseScale *= std::pow(2., dDelta);;
	updateViewMatrix();

	QWidget::wheelEvent(pEvt);
}


void PlotGl::TogglePerspective()
{
	std::lock_guard<QMutex> _lck(m_mutex_resize);

	m_bPerspective = !m_bPerspective;
	m_size.bDoResize = 1;
}


// ----------------------------------------------------------------------------


void PlotGl::updateViewMatrix()
{
	t_mat4 matScale = tl::make_mat<t_mat4>(
		{{m_dMouseScale,              0,             0, 0},
		{            0,   m_dMouseScale,             0, 0},
		{            0,               0, m_dMouseScale, 0},
		{            0,               0,             0, 1}});

	t_mat4 matR0 = tl::rotation_matrix_3d_z(tl::d2r<t_real>(m_dMouseRot[0]));
	t_mat4 matR1 = tl::rotation_matrix_3d_x(tl::d2r<t_real>(-90. + m_dMouseRot[1]));
	matR0.resize(4,4,1); matR1.resize(4,4,1);
	matR0(3,3) = matR1(3,3) = 1.;
	for(short i=0; i<3; ++i) matR0(i,3)=matR0(3,i)=matR1(i,3)=matR1(3,i)=0.;
	t_mat4 matRot0 = matR0, matRot1 = matR1;

	t_mat4 matTrans = tl::make_mat<t_mat4>(
		{{ 1, 0, 0,  0},
		{  0, 1, 0,  0},
		{  0, 0, 1, -2},
		{  0, 0, 0,  1}});

	{
		std::lock_guard<QMutex> _lck(m_mutex);
		m_matView = ublas::prod(matTrans, matRot1);
		m_matView = ublas::prod(m_matView, matRot0);
		m_matView = ublas::prod(m_matView, matScale);
	}
}


void PlotGl::AddHoverSlot(const typename t_sigHover::slot_type& conn)
{
	m_sigHover.connect(conn);
}


void PlotGl::mouseSelectObj(t_real dX, t_real dY)
{
	std::lock_guard<QMutex> _lck(m_mutex);
	tl::Line<t_real> ray = tl::screen_ray(dX, dY, m_matProj, m_matView);

	for(PlotObjGl& obj : m_vecObjs)
	{
		obj.bSelected = 0;

		std::unique_ptr<tl::Quadric<t_real>> pQuad;
		t_vec3 vecOffs = ublas::zero_vector<t_real>(3);

		if(obj.plttype == PLOT_SPHERE)
		{
			pQuad.reset(new tl::QuadSphere<t_real>(obj.vecScale[0]));
			vecOffs = obj.vecPos;
		}
		else if(obj.plttype == PLOT_ELLIPSOID)
		{
			pQuad.reset(new tl::QuadEllipsoid<t_real>(obj.vecScale[0], obj.vecScale[1], obj.vecScale[2]));

			vecOffs = obj.vecPos;
			t_mat3 matRot = tl::make_mat<t_mat3>(
				{{obj.vecRotMat[0], obj.vecRotMat[1], obj.vecRotMat[2]},
				 {obj.vecRotMat[3], obj.vecRotMat[4], obj.vecRotMat[5]},
				 {obj.vecRotMat[6], obj.vecRotMat[7], obj.vecRotMat[8]}});
			pQuad->transform(matRot);
		}

		std::vector<t_real> vecT;
		if(pQuad)
		{
			pQuad->SetOffset(vecOffs);
			vecT = pQuad->intersect(ray);
		}

		if(vecT.size() > 0)
		{
			for(t_real t : vecT)
			{
				if(t < 0.) continue; // beyond "near" plane
				if(t > 1.) continue; // beyond "far" plane
				obj.bSelected = 1;
			}
		}
	}
}


void PlotGl::SetLabels(const char* pcLabX, const char* pcLabY, const char* pcLabZ)
{
	std::lock_guard<QMutex> _lck(m_mutex);

	m_strLabels[0] = pcLabX;
	m_strLabels[1] = pcLabY;
	m_strLabels[2] = pcLabZ;
}
