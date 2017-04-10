/**
 * quaternion test
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date jan-2015
 * @license GPLv2
 */

// moc-qt5 tst_quat2.cpp > tst_quat2.moc && gcc -fPIC -I. -I../.. -I/usr/include/GL -I/usr/include/qt5 -I/usr/include/freetype2 -o tst_quat2 tst_quat2.cpp -lstdc++ -lm -lQt5Core -lQt5Gui -lQt5OpenGL -lQt5Widgets -lGL -std=c++14

#include <iostream>

#include "tlibs/math/linalg.h"
#include "tlibs/math/quat.h"
#include "tlibs/gfx/gl.h"

#include <QtOpenGL/QGLWidget>
#include <QtGui/QOpenGLFunctions_2_1>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtCore/QTimer>

using namespace tl;


class GlWidget : public QGLWidget /*QOpenGLWidget*/
{ Q_OBJECT
	protected:
		QOpenGLFunctions_2_1 *m_pGL = 0;
		double m_dTick = 0.;
		QTimer m_timer;

	protected slots:
		void tick()
		{
			m_dTick += 0.02;
			update();
		}

	public:
		GlWidget(QWidget* pParent) : QGLWidget/*QOpenGLWidget*/(pParent), m_timer(this)
		{
			QObject::connect(&m_timer, &QTimer::timeout, this, &GlWidget::tick);
			m_timer.start(5);
		}
		virtual ~GlWidget()
		{
			m_timer.stop();
		}

	protected:
		virtual void initializeGL() override
		{
			m_pGL = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_1>();
			m_pGL->initializeOpenGLFunctions();

			m_pGL->glClearColor(1.,1.,1.,0.);
			m_pGL->glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
			m_pGL->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
			m_pGL->glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
			m_pGL->glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		}

		virtual void resizeGL(int w, int h) override
		{
			if(w<=0) w=1; if(h<=0) h=1;
			m_pGL->glViewport(0,0,w,h);

			m_pGL->glMatrixMode(GL_PROJECTION);
			//t_mat4 mat = perspective_matrix(45./180.*M_PI, double(w)/double(h), 0.1, 100.);
			//GLdouble glmat[16]; to_gl_array(mat, glmat);
			//m_pGL->glLoadMatrixd(glmat);
			m_pGL->glLoadIdentity();

			m_pGL->glMatrixMode(GL_MODELVIEW);
			m_pGL->glLoadIdentity();
		}

		virtual void paintGL() override
		{
			m_pGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			m_pGL->glPushMatrix();

			m_pGL->glTranslated(0., 0., -0.5);

			t_mat3 mat0 = rotation_matrix_3d_z(0.);
			t_mat3 mat1 = rotation_matrix_3d_z(M_PI/2.);

			math::quaternion<double> quat0 = rot3_to_quat(mat0);
			math::quaternion<double> quat1 = rot3_to_quat(mat1);
			math::quaternion<double> quat = slerp(quat0, quat1, m_dTick);
			//math::quaternion<double> quat = lerp(quat0, quat1, m_dTick);

			t_mat3 mat = quat_to_rot3(quat);
			GLdouble glmat[16]; to_gl_array(mat, glmat);
			m_pGL->glMultMatrixd(glmat);

			m_pGL->glColor3f(0.,0.,0.);
			m_pGL->glBegin(GL_LINE_STRIP);
				m_pGL->glVertex3f(-0.5, 0., 0.);
				m_pGL->glVertex3f(0.5, 0., 0.);
				m_pGL->glVertex3f(0., 0.5, 0.);
				m_pGL->glVertex3f(-0.5, 0., 0.);
			m_pGL->glEnd();

			m_pGL->glPopMatrix();
		}
};


int main(int argc, char **argv)
{
	QApplication a(argc, argv);

	QDialog dlg;
	GlWidget *pGl = new GlWidget(&dlg);
	dlg.resize(800, 600);

	QGridLayout *pGrid = new QGridLayout(&dlg);
	pGrid->addWidget(pGl, 0,0,1,1);
	dlg.exec();

	return 0;
}

#include "tst_quat2.moc"
