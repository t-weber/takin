/**
 * gl plotter without threading
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 19-may-2013 -- jan-2019
 * @license GPLv2
 */

#ifndef __TAKIN_PLOT_GL_2__
#define __TAKIN_PLOT_GL_2__

#include "plotgl_iface.h"
#include "plotgl.h"
#include <QTimer>


class PlotGl2 : public PlotGl_iface, public QTimer
{
protected:
	QSettings *m_pSettings = nullptr;
	bool m_bEnabled;

	static constexpr t_real_glob m_dFOV = 45./180.*M_PI;
	tl::t_mat4_gen<t_real_glob> m_matProj, m_matView;
	bool m_bPerspective = 1;	// perspective or orthogonal projection?
	bool m_bResetPrespective = 0;
	ublas::vector<t_real_glob> m_vecCam;
	bool m_bDoZTest = 0;
	bool m_bDrawPolys = 1;
	bool m_bDrawLines = 1;
	bool m_bDrawSpheres = 1;

	tl::GlFontMap<t_real_glob> *m_pFont = nullptr;

	std::vector<PlotObjGl> m_vecObjs;
	GLuint m_iLstSphere[4];
	QString m_strLabels[3];

	std::size_t m_iPrec = 6;
	bool m_bDrawMinMax = 1;

	t_real_glob m_dTime = 0.;

	PlotGl_iface::t_sigHover m_sigHover;

protected:
	virtual void timerEvent(QTimerEvent *pEvt) override;

	void SetColor(t_real_glob r, t_real_glob g, t_real_glob b, t_real_glob a=1.);
	void SetColor(std::size_t iIdx);

	// ------------------------------------------------------------------------
	// mouse stuff
	bool m_bMouseRotateActive = 0;
	t_real_glob m_dMouseRot[2];
	t_real_glob m_dMouseBegin[2];

	bool m_bMouseScaleActive = 0;
	t_real_glob m_dMouseScale;
	t_real_glob m_dMouseScaleBegin;

	virtual void mousePressEvent(QMouseEvent*) override;
	virtual void mouseReleaseEvent(QMouseEvent*) override;
	virtual void mouseMoveEvent(QMouseEvent*) override;
	virtual void wheelEvent(QWheelEvent*) override;

	void updateViewMatrix();
	void mouseSelectObj(t_real_glob dX, t_real_glob dY);

protected:
	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;
	virtual void paintGL() override;

	void SetPerspective(int w, int h);
	void freeGL();
	void tick(t_real_glob dTime);

	t_real_glob GetCamObjDist(const PlotObjGl& obj) const;
	std::vector<std::size_t> GetObjSortOrder() const;
	PlotGlSize m_size;

public:
	PlotGl2(QWidget* pParent, QSettings *pSettings=nullptr, t_real_glob dMouseScale=25.);
	virtual ~PlotGl2();

	virtual void AddHoverSlot(const typename PlotGl_iface::t_sigHover::slot_type& conn) override;

	virtual void clear() override;
	virtual void TogglePerspective() override;
	virtual void ToggleZTest() override { m_bDoZTest = !m_bDoZTest; }
	virtual void ToggleDrawPolys() override { m_bDrawPolys = !m_bDrawPolys; }
	virtual void ToggleDrawLines() override { m_bDrawLines = !m_bDrawLines; }
	virtual void ToggleDrawSpheres() override { m_bDrawSpheres = !m_bDrawSpheres; }

	virtual void PlotSphere(const ublas::vector<t_real_glob>& vecPos, t_real_glob dRadius, int iObjIdx=-1) override;
	virtual void PlotEllipsoid(const ublas::vector<t_real_glob>& widths,
		const ublas::vector<t_real_glob>& offsets,
		const ublas::matrix<t_real_glob>& rot,
		int iObjsIdx=-1) override;
	virtual void PlotPoly(const std::vector<ublas::vector<t_real_glob>>& vecVertices,
		const ublas::vector<t_real_glob>& vecNorm, int iObjIdx=-1) override;
	virtual void PlotLines(const std::vector<ublas::vector<t_real_glob>>& vecVertices,
		t_real_glob dLW=2., int iObjIdx=-1) override;

	virtual void SetObjectCount(std::size_t iSize) override { m_vecObjs.resize(iSize); }
	virtual void SetObjectColor(std::size_t iObjIdx, const std::vector<t_real_glob>& vecCol) override;
	virtual void SetObjectLabel(std::size_t iObjIdx, const std::string& strLab) override;
	virtual void SetObjectUseLOD(std::size_t iObjIdx, bool bLOD) override;
	virtual void SetObjectCull(std::size_t iObjIdx, bool bCull) override;
	virtual void SetObjectAnimation(std::size_t iObjIdx, bool bAnimate) override;

	virtual void SetLabels(const char* pcLabX, const char* pcLabY, const char* pcLabZ) override;
	virtual void SetDrawMinMax(bool b) override { m_bDrawMinMax = b; }

	virtual void SetEnabled(bool b) override;
	virtual void SetPrec(std::size_t iPrec) override { m_iPrec = iPrec; }

	virtual void keyPressEvent(QKeyEvent*) override;
};


// choose between threaded and non-threaded plotters
extern PlotGl_iface*
make_gl_plotter(bool bThreaded, QWidget* pParent, QSettings *pSettings=nullptr, t_real_glob dMouseScale=25.);

#endif
