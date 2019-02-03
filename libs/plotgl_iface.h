/**
 * gl plotter interface
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date jan-2019
 * @license GPLv2
 */

#ifndef __TAKIN_PLOT_GL_IFACE__
#define __TAKIN_PLOT_GL_IFACE__

#if QT_VER>=5
	#include <QtWidgets>
#endif
#include <QGLWidget>

#include <vector>

#include "tlibs/gfx/gl.h"
#include "libs/globals.h"
#include "libs/globals_qt.h"

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
namespace ublas = boost::numeric::ublas;

#include <boost/signals2.hpp>
namespace sig = boost::signals2;


using t_qglwidget = QGLWidget;


/**
 * types of plottable objects
 */
enum PlotTypeGl
{
	PLOT_INVALID,

	PLOT_SPHERE,
	PLOT_ELLIPSOID,

	PLOT_POLY,
	PLOT_LINES,
};


/**
 * plottable object
 */
struct PlotObjGl
{
	PlotTypeGl plttype = PLOT_INVALID;

	ublas::vector<t_real_glob> vecPos;
	ublas::vector<t_real_glob> vecScale;
	std::vector<t_real_glob> vecRotMat;

	std::vector<t_real_glob> vecColor;
	t_real_glob dLineWidth = 2.;

	std::vector<ublas::vector<t_real_glob>> vecVertices;
	ublas::vector<t_real_glob> vecNorm;

	bool bSelected = 0;
	bool bUseLOD = 1;
	bool bCull = 1;

	bool bAnimated = 0;
	t_real_glob dScaleMult = 1.;

	std::string strLabel;
};


struct PlotGlSize
{
	int iW = 800, iH = 600;
	t_real_glob dDPIScale = 1.;
	bool bDoResize = true;
};


class PlotGl_iface : public t_qglwidget
{
protected:
	t_real_glob m_dXMin=-10., m_dXMax=10.;
	t_real_glob m_dYMin=-10., m_dYMax=10.;
	t_real_glob m_dZMin=-10., m_dZMax=10.;
	t_real_glob m_dXMinMaxOffs, m_dYMinMaxOffs, m_dZMinMaxOffs;

public:
	PlotGl_iface(const QGLFormat &form, QWidget *pParent) : t_qglwidget(form, pParent) {}
	PlotGl_iface(QWidget *pParent) : t_qglwidget(pParent) {}
	virtual ~PlotGl_iface() = default;

	using t_sigHover = sig::signal<void(const PlotObjGl*)>;
	virtual void AddHoverSlot(const typename t_sigHover::slot_type& conn) = 0;

	virtual void clear() = 0;
	virtual void TogglePerspective() = 0;
	virtual void ToggleZTest() = 0;
	virtual void ToggleDrawPolys() = 0;
	virtual void ToggleDrawLines() = 0;
	virtual void ToggleDrawSpheres() = 0;

	virtual void PlotSphere(const ublas::vector<t_real_glob>& vecPos, t_real_glob dRadius, int iObjIdx=-1) = 0;
	virtual void PlotEllipsoid(const ublas::vector<t_real_glob>& widths,
		const ublas::vector<t_real_glob>& offsets,
		const ublas::matrix<t_real_glob>& rot,
		int iObjsIdx=-1) = 0;
	virtual void PlotPoly(const std::vector<ublas::vector<t_real_glob>>& vecVertices,
		const ublas::vector<t_real_glob>& vecNorm, int iObjIdx=-1) = 0;
	virtual void PlotLines(const std::vector<ublas::vector<t_real_glob>>& vecVertices,
		t_real_glob dLW=2., int iObjIdx=-1) = 0;

	virtual void SetObjectCount(std::size_t iSize) = 0;
	virtual void SetObjectColor(std::size_t iObjIdx, const std::vector<t_real_glob>& vecCol) = 0;
	virtual void SetObjectLabel(std::size_t iObjIdx, const std::string& strLab) = 0;
	virtual void SetObjectUseLOD(std::size_t iObjIdx, bool bLOD) = 0;
	virtual void SetObjectCull(std::size_t iObjIdx, bool bCull) = 0;
	virtual void SetObjectAnimation(std::size_t iObjIdx, bool bAnimate) = 0;

	virtual void SetEnabled(bool b) = 0;
	virtual void SetPrec(std::size_t iPrec) = 0;

	virtual void SetLabels(const char* pcLabX, const char* pcLabY, const char* pcLabZ) = 0;
	virtual void SetDrawMinMax(bool b) = 0;

	virtual void keyPressEvent(QKeyEvent *pEvt) override { t_qglwidget::keyPressEvent(pEvt); }

	template<class t_vec>
	/*virtual*/ void SetMinMax(const t_vec& vecMin, const t_vec& vecMax, const t_vec* pOffs=0)
	{
		m_dXMin = vecMin[0]; m_dXMax = vecMax[0];
		m_dYMin = vecMin[1]; m_dYMax = vecMax[1];
		m_dZMin = vecMin[2]; m_dZMax = vecMax[2];

		m_dXMinMaxOffs =  pOffs ? (*pOffs)[0] : 0.;
		m_dYMinMaxOffs =  pOffs ? (*pOffs)[1] : 0.;
		m_dZMinMaxOffs =  pOffs ? (*pOffs)[2] : 0.;
	}

	template<class t_vec=ublas::vector<t_real_glob>>
	/*virtual*/ void SetMinMax(const t_vec& vec, const t_vec* pOffs=0)
	{
		m_dXMin = -vec[0]; m_dXMax = vec[0];
		m_dYMin = -vec[1]; m_dYMax = vec[1];
		m_dZMin = -vec[2]; m_dZMax = vec[2];

		m_dXMinMaxOffs =  pOffs ? (*pOffs)[0] : 0.;
		m_dYMinMaxOffs =  pOffs ? (*pOffs)[1] : 0.;
		m_dZMinMaxOffs =  pOffs ? (*pOffs)[2] : 0.;
	}
};

#endif
