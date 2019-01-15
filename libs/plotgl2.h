/**
 * gl plotter without threading
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2013 -- 2019
 * @license GPLv2
 */

#ifndef __TAKIN_PLOT_GL_2__
#define __TAKIN_PLOT_GL_2__

#include "plotgl.h"
#include <QTimer>


class PlotGl2 : public t_qglwidget, public QTimer
{
protected:
	QSettings *m_pSettings = nullptr;
	std::atomic<bool> m_bEnabled;
	mutable QMutex m_mutex, m_mutex_resize;

	static constexpr t_real_glob m_dFOV = 45./180.*M_PI;
	tl::t_mat4_gen<t_real_glob> m_matProj, m_matView;
	bool m_bPerspective = 1;	// perspective or orthogonal projection?
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
	t_real_glob m_dXMin=-10., m_dXMax=10.;
	t_real_glob m_dYMin=-10., m_dYMax=10.;
	t_real_glob m_dZMin=-10., m_dZMax=10.;
	t_real_glob m_dXMinMaxOffs, m_dYMinMaxOffs, m_dZMinMaxOffs;
	bool m_bDrawMinMax = 1;

	t_real_glob m_dTime = 0.;

protected:
	virtual bool event(QEvent*) override;
	virtual void resizeEvent(QResizeEvent*) override;
	virtual void paintEvent(QPaintEvent*) override;

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

public:
	using t_sigHover = sig::signal<void(const PlotObjGl*)>;
	void AddHoverSlot(const typename t_sigHover::slot_type& conn);
protected:
	t_sigHover m_sigHover;


	// ------------------------------------------------------------------------
	// rendering
	virtual void initializeGL() override;
	void freeGL();
	virtual void resizeGL(int w, int h) override;
	virtual void paintGL() override;
	void tick(t_real_glob dTime);

	t_real_glob GetCamObjDist(const PlotObjGl& obj) const;
	std::vector<std::size_t> GetObjSortOrder() const;
	PlotGlSize m_size;
	// ------------------------------------------------------------------------

public:
	PlotGl2(QWidget* pParent, QSettings *pSettings=nullptr, t_real_glob dMouseScale=25.);
	virtual ~PlotGl2();

	void clear();
	void TogglePerspective();
	void ToggleZTest() { m_bDoZTest = !m_bDoZTest; }
	void ToggleDrawPolys() { m_bDrawPolys = !m_bDrawPolys; }
	void ToggleDrawLines() { m_bDrawLines = !m_bDrawLines; }
	void ToggleDrawSpheres() { m_bDrawSpheres = !m_bDrawSpheres; }

	void PlotSphere(const ublas::vector<t_real_glob>& vecPos, t_real_glob dRadius, int iObjIdx=-1);
	void PlotEllipsoid(const ublas::vector<t_real_glob>& widths,
		const ublas::vector<t_real_glob>& offsets,
		const ublas::matrix<t_real_glob>& rot,
		int iObjsIdx=-1);
	void PlotPoly(const std::vector<ublas::vector<t_real_glob>>& vecVertices,
		const ublas::vector<t_real_glob>& vecNorm, int iObjIdx=-1);
	void PlotLines(const std::vector<ublas::vector<t_real_glob>>& vecVertices,
		t_real_glob dLW=2., int iObjIdx=-1);

	void SetObjectCount(std::size_t iSize) { m_vecObjs.resize(iSize); }
	void SetObjectColor(std::size_t iObjIdx, const std::vector<t_real_glob>& vecCol);
	void SetObjectLabel(std::size_t iObjIdx, const std::string& strLab);
	void SetObjectUseLOD(std::size_t iObjIdx, bool bLOD);
	void SetObjectCull(std::size_t iObjIdx, bool bCull);
	void SetObjectAnimation(std::size_t iObjIdx, bool bAnimate);

	void SetLabels(const char* pcLabX, const char* pcLabY, const char* pcLabZ);
	void SetDrawMinMax(bool b) { m_bDrawMinMax = b; }

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

	template<class t_vec=ublas::vector<t_real_glob>>
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

	virtual void keyPressEvent(QKeyEvent*) override;
};

#endif
