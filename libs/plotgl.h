/**
 * gl plotter
 * @author tweber
 * @date 19-may-2013
 * @license GPLv2
 */

#ifndef __MIEZE_PLOT_GL__
#define __MIEZE_PLOT_GL__

#if QT_VER>=5
       	#include <QtWidgets>
#endif
#include <QGLWidget>
//#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QThread>
#include <QMutex>
#include <QSettings>

#include <vector>
#include <atomic>

#include "tlibs/gfx/gl.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
namespace ublas = boost::numeric::ublas;

#include <boost/signals2.hpp>
namespace sig = boost::signals2;


using tl::t_real_gl;
using t_qglwidget = QGLWidget;
//using t_qglwidget = QOpenGLWidget;

enum PlotTypeGl
{
	PLOT_INVALID,

	PLOT_SPHERE,
	PLOT_ELLIPSOID,
};

struct PlotObjGl
{
	PlotTypeGl plttype = PLOT_INVALID;
	std::vector<t_real_gl> vecParams;
	std::vector<t_real_glob> vecColor;

	bool bSelected = 0;
	bool bUseLOD = 1;
	std::string strLabel;
};

class PlotGl : public t_qglwidget, QThread
{
protected:
	QSettings *m_pSettings = 0;
	std::atomic<bool> m_bEnabled;
	QMutex m_mutex;

	static constexpr t_real_gl m_dFOV = 45./180.*M_PI;
	tl::t_mat4 m_matProj, m_matView;

	tl::GlFontMap *m_pFont = nullptr;

	std::vector<PlotObjGl> m_vecObjs;
	GLuint m_iLstSphere[8];
	QString m_strLabels[3];

	std::size_t m_iPrec = 6;
	t_real_gl m_dXMin=-10., m_dXMax=10.;
	t_real_gl m_dYMin=-10., m_dYMax=10.;
	t_real_gl m_dZMin=-10., m_dZMax=10.;
	t_real_gl m_dXMinMaxOffs, m_dYMinMaxOffs, m_dZMinMaxOffs;

	//void initializeGL();
	void resizeEvent(QResizeEvent*);
	void paintEvent(QPaintEvent*);

	void SetColor(t_real_glob r, t_real_glob g, t_real_glob b, t_real_glob a=1.);
	void SetColor(std::size_t iIdx);


	// ------------------------------------------------------------------------
	// mouse stuff
	bool m_bMouseRotateActive = 0;
	t_real_gl m_dMouseRot[2];
	t_real_gl m_dMouseBegin[2];

	bool m_bMouseScaleActive = 0;
	t_real_gl m_dMouseScale;
	t_real_gl m_dMouseScaleBegin;

	t_real_gl m_dMouseX = 0., m_dMouseY = 0.;

	void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);

	void updateViewMatrix();
	void mouseSelectObj(t_real_gl dX, t_real_gl dY);

public:
	using t_sigHover = sig::signal<void(const PlotObjGl*)>;
	void AddHoverSlot(const typename t_sigHover::slot_type& conn);
protected:
	t_sigHover m_sigHover;


	// ------------------------------------------------------------------------
	// render thread
	bool m_bDoResize = 1;
	bool m_bRenderThreadActive = 1;

	void initializeGLThread();
	void freeGLThread();
	void resizeGLThread(int w, int h);
	void paintGLThread();
	void tickThread(t_real_gl dTime);
	void run();

	int m_iW=640, m_iH=480;
	// ------------------------------------------------------------------------

public:
	PlotGl(QWidget* pParent, QSettings *pSettings=nullptr, t_real_gl dMouseScale=25.);
	virtual ~PlotGl();

	void PlotSphere(const ublas::vector<t_real_gl>& vecPos, t_real_gl dRadius, int iObjIdx=-1);
	void PlotEllipsoid(const ublas::vector<t_real_gl>& widths,
		const ublas::vector<t_real_gl>& offsets,
		const ublas::matrix<t_real_gl>& rot,
		int iObjsIdx=-1);
	void SetObjectCount(std::size_t iSize) { m_vecObjs.resize(iSize); }
	void SetObjectColor(std::size_t iObjIdx, const std::vector<t_real_glob>& vecCol);
	void SetObjectLabel(std::size_t iObjIdx, const std::string& strLab);
	void SetObjectUseLOD(std::size_t iObjIdx, bool bLOD);
	void clear();

	void SetLabels(const char* pcLabX, const char* pcLabY, const char* pcLabZ);

	template<class t_vec>
	void SetMinMax(const t_vec& vecMin, const t_vec& vecMax, const t_vec* pOffs=0)
	{
		m_dXMin = vecMin[0]; m_dXMax = vecMax[0];
		m_dYMin = vecMin[1]; m_dYMax = vecMax[1];
		m_dZMin = vecMin[2]; m_dZMax = vecMax[2];

		m_dXMinMaxOffs =  pOffs ? (*pOffs)[0] : 0.;
		m_dYMinMaxOffs =  pOffs ? (*pOffs)[1] : 0.;
		m_dZMinMaxOffs =  pOffs ? (*pOffs)[2] : 0.;
	}

	template<class t_vec=ublas::vector<t_real_gl>>
	void SetMinMax(const t_vec& vec, const t_vec* pOffs=0)
	{
		m_dXMin = -vec[0]; m_dXMax = vec[0];
		m_dYMin = -vec[1]; m_dYMax = vec[1];
		m_dZMin = -vec[2]; m_dZMax = vec[2];

		m_dXMinMaxOffs =  pOffs ? (*pOffs)[0] : 0.;
		m_dYMinMaxOffs =  pOffs ? (*pOffs)[1] : 0.;
		m_dZMinMaxOffs =  pOffs ? (*pOffs)[2] : 0.;
	}

	void SetEnabled(bool b);
	void SetPrec(std::size_t iPrec) { m_iPrec = iPrec; }
};

#endif
