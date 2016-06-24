/**
 * monte carlo convolution tool
 * @author tweber
 * @date 2015, 2016
 * @license GPLv2
 */

#ifndef __MCONV_SQW_H__
#define __MCONV_SQW_H__

//#define USE_RTREE

#include <string>
#include <unordered_map>
#include <list>
#include <tuple>
#include <memory>
#include <boost/numeric/ublas/vector.hpp>

#include "tlibs/math/math.h"
#include "tlibs/math/kd.h"
#include "../res/defs.h"
#include "sqwbase.h"

#ifdef USE_RTREE
	#include "tlibs/math/rt.h"
	#define RT_ELEMS 64
#endif

namespace ublas = boost::numeric::ublas;


// -----------------------------------------------------------------------------

struct ElastPeak
{
	t_real_reso h, k, l;
	t_real_reso dSigQ, dSigE;
	t_real_reso dS;
};


/**
 * Bragg peaks
 */
class SqwElast : public SqwBase
{
protected:
	bool m_bLoadedFromFile = false;
	std::list<ElastPeak> m_lstPeaks;

public:
	SqwElast() { SqwBase::m_bOk = true; }
	SqwElast(const char* pcFile);
	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	void AddPeak(t_real_reso h, t_real_reso k, t_real_reso l, t_real_reso dSigQ, t_real_reso dSigE, t_real_reso dS);

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};


// -----------------------------------------------------------------------------


/**
 * tabulated model
 */
class SqwKdTree : public SqwBase
{
protected:
	std::unordered_map<std::string, std::string> m_mapParams;
	std::shared_ptr<tl::Kd<t_real_reso>> m_kd;

public:
	SqwKdTree(const char* pcFile = nullptr);
	virtual ~SqwKdTree() = default;

	bool open(const char* pcFile);
	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};


// -----------------------------------------------------------------------------


/**
 * simple phonon model
 */
class SqwPhonon : public SqwBase
{
private:
	SqwPhonon() {};

protected:
	static t_real_reso disp(t_real_reso dq, t_real_reso da, t_real_reso df);

	void create();
	void destroy();

protected:
#ifdef USE_RTREE
	std::shared_ptr<tl::Rt<t_real_reso, 3, RT_ELEMS>> m_rt;
#else
	std::shared_ptr<tl::Kd<t_real_reso>> m_kd;
#endif
	unsigned int m_iNumqs = 250;
	unsigned int m_iNumArc = 50;
	t_real_reso m_dArcMax = 10.;

	ublas::vector<t_real_reso> m_vecBragg;

	ublas::vector<t_real_reso> m_vecLA;
	ublas::vector<t_real_reso> m_vecTA1;
	ublas::vector<t_real_reso> m_vecTA2;

	t_real_reso m_dLA_amp=20., m_dLA_freq=M_PI/2., m_dLA_E_HWHM=0.1, m_dLA_q_HWHM=0.1, m_dLA_S0=1.;
	t_real_reso m_dTA1_amp=15., m_dTA1_freq=M_PI/2., m_dTA1_E_HWHM=0.1, m_dTA1_q_HWHM=0.1, m_dTA1_S0=1.;
	t_real_reso m_dTA2_amp=10., m_dTA2_freq=M_PI/2., m_dTA2_E_HWHM=0.1, m_dTA2_q_HWHM=0.1, m_dTA2_S0=1.;

	t_real_reso m_dIncAmp=0., m_dIncSig=0.1;
	t_real_reso m_dT = 100.;

public:
	SqwPhonon(const ublas::vector<t_real_reso>& vecBragg,
		const ublas::vector<t_real_reso>& vecTA1,
		const ublas::vector<t_real_reso>& vecTA2,
		t_real_reso dLA_amp, t_real_reso dLA_freq, t_real_reso dLA_E_HWHM, t_real_reso dLA_q_HWHM, t_real_reso dLA_S0,
		t_real_reso dTA1_amp, t_real_reso dTA1_freq, t_real_reso dTA1_E_HWHM, t_real_reso dTA1_q_HWHM, t_real_reso dTA1_S0,
		t_real_reso dTA2_amp, t_real_reso dTA2_freq, t_real_reso dTA2_E_HWHM, t_real_reso dTA2_q_HWHM, t_real_reso dTA2_S0,
		t_real_reso dT);
	SqwPhonon(const char* pcFile);

	virtual ~SqwPhonon() = default;

	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;


	const ublas::vector<t_real_reso>& GetBragg() const { return m_vecBragg; }
	const ublas::vector<t_real_reso>& GetLA() const { return m_vecLA; }
	const ublas::vector<t_real_reso>& GetTA1() const { return m_vecTA1; }
	const ublas::vector<t_real_reso>& GetTA2() const { return m_vecTA2; }

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};


// -----------------------------------------------------------------------------


/**
 * simple magnon model
 */
class SqwMagnon : public SqwBase
{
private:
	SqwMagnon() {};

protected:
	static t_real_reso ferro_disp(t_real_reso dq, t_real_reso dD, t_real_reso doffs);
	static t_real_reso antiferro_disp(t_real_reso dq, t_real_reso dD, t_real_reso doffs);

	void create();
	void destroy();

protected:
#ifdef USE_RTREE
	std::shared_ptr<tl::Rt<t_real_reso, 3, RT_ELEMS>> m_rt;
#else
	std::shared_ptr<tl::Kd<t_real_reso>> m_kd;
#endif

	unsigned short m_iWhichDisp = 0;		// 0: ferro, 1: antiferro
	unsigned int m_iNumPoints = 100;

	ublas::vector<t_real_reso> m_vecBragg;

	t_real_reso m_dD = 1., m_dOffs = 0.;
	t_real_reso m_dE_HWHM = 0.1, m_dq_HWHM = 0.1;
	t_real_reso m_dS0 = 1.;

	t_real_reso m_dIncAmp = 0., m_dIncSig = 0.1;
	t_real_reso m_dT = 300.;

public:
	SqwMagnon(const char* pcFile);
	virtual ~SqwMagnon() = default;

	virtual t_real_reso operator()(t_real_reso dh, t_real_reso dk, t_real_reso dl, t_real_reso dE) const override;

	const ublas::vector<t_real_reso>& GetBragg() const { return m_vecBragg; }

	virtual std::vector<SqwBase::t_var> GetVars() const override;
	virtual void SetVars(const std::vector<SqwBase::t_var>&) override;

	virtual SqwBase* shallow_copy() const override;
};

#endif
