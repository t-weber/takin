/*
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


enum PlotTypeGl
{
	PLOT_INVALID,

	PLOT_SPHERE,
	PLOT_ELLIPSOID,
};

struct PlotObjGl
{
	PlotTypeGl plttype = PLOT_INVALID;
	std::vector<double> vecParams;
	std::vector<t_real_glob> vecColor;

	bool bSelected = 0;
	bool bUseLOD = 1;
	std::string strLabel;
};

class PlotGl : public QGLWidget, QThread
{
protected:
	QSettings *m_pSettings = 0;
	std::atomic<bool> m_bEnabled;
	QMutex m_mutex;

	static constexpr double m_dFOV = 45./180.*M_PI;
	tl::t_mat4 m_matProj, m_matView;

	tl::GlFontMap *m_pFont = nullptr;

	std::vector<PlotObjGl> m_vecObjs;
	GLuint m_iLstSphere[8];
	QString m_strLabels[3];

	unsigned int m_iPrec = 6;
	double m_dXMin=-10., m_dXMax=10.;
	double m_dYMin=-10., m_dYMax=10.;
	double m_dZMin=-10., m_dZMax=10.;
	double m_dXMinMaxOffs, m_dYMinMaxOffs, m_dZMinMaxOffs;

	//void initializeGL();
	void resizeEvent(QResizeEvent*);
	void paintEvent(QPaintEvent*);

	void SetColor(t_real_glob r, t_real_glob g, t_real_glob b, t_real_glob a=1.);
	void SetColor(unsigned int iIdx);

	// ------------------------------------------------------------------------
	// mouse stuff
	bool m_bMouseRotateActive = 0;
	double m_dMouseRot[2];
	double m_dMouseBegin[2];

	bool m_bMouseScaleActive = 0;
	double m_dMouseScale;
	double m_dMouseScaleBegin;

	double m_dMouseX = 0., m_dMouseY = 0.;

	void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);

	void updateViewMatrix();
	void mouseSelectObj(double dX, double dY);

	// ------------------------------------------------------------------------
	// render thread
	bool m_bGLInited = 0;
	bool m_bDoResize = 1;
	bool m_bRenderThreadActive = 1;

	void initializeGLThread();
	void freeGLThread();
	void resizeGLThread(int w, int h);
	void paintGLThread();
	void tickThread(double dTime);
	void run();

	int m_iW=640, m_iH=480;
	// ------------------------------------------------------------------------

public:
	PlotGl(QWidget* pParent, QSettings *pSettings=0);
	virtual ~PlotGl();

	void PlotSphere(const ublas::vector<double>& vecPos, double dRadius, int iObjIdx=-1);
	void PlotEllipsoid(const ublas::vector<double>& widths,
		const ublas::vector<double>& offsets,
		const ublas::matrix<double>& rot,
		int iObjsIdx=-1);
	void SetObjectCount(unsigned int iSize) { m_vecObjs.resize(iSize); }
	void SetObjectColor(int iObjIdx, const std::vector<t_real_glob>& vecCol);
	void SetObjectLabel(int iObjIdx, const std::string& strLab);
	void SetObjectUseLOD(int iObjIdx, bool bLOD);
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

	template<class t_vec=ublas::vector<double> >
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
	void SetPrec(unsigned int iPrec) { m_iPrec = iPrec; }
};

#endif
