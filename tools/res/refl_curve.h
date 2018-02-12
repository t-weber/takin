/**
 * reflectivity curve
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date oct-2017
 * @license GPLv2
 */

#ifndef __TAKIN_REFL_H__
#define __TAKIN_REFL_H__

#include "tlibs/fit/interpolation.h"
#include "tlibs/phys/neutrons.h"
#include "tlibs/file/loaddat.h"
#include "tlibs/file/file.h"
#include "tlibs/log/log.h"


/**
 * reflectivity curve
 */
template<class t_real = double>
class ReflCurve
{
	private:
		bool m_bOk = 0;

	protected:
		// expects a reflectivity curve data file with [k, R] columns
		tl::DatFile<t_real, char> m_file;
		std::unique_ptr<tl::LinInterp<t_real>> m_pinterp;

	public:
		ReflCurve(const std::string& strFile, const std::vector<std::string>* pPaths = nullptr)
		{
			m_bOk = 0;

			if(!pPaths)
			{ // use given file
				if(!m_file.Load(strFile))
					return;
			}
			else
			{ // try to find file in given paths
				std::string _strFile;
				bool bOk = 0;
				std::tie(bOk, _strFile) = 
					tl::find_file<std::string, std::vector>(*pPaths, strFile);

				if(!bOk)
					return;
				if(!m_file.Load(_strFile))
					return;
			}

			if(m_file.GetColumnCount() < 2)
				return;

			const auto& col0 = m_file.GetColumn(0);
			const auto& col1 = m_file.GetColumn(1);

			m_pinterp.reset(new tl::LinInterp<t_real>(col0.size(), col0.data(), col1.data()));
			m_bOk = 1;
		}

		~ReflCurve() = default;

		t_real operator()(const tl::t_wavenumber_si<t_real>& k) const
		{
			if(!*this)
			{
				tl::log_err("Trying to use invalid reflectivity curve!");
				return t_real(1);
			}

			t_real dK = static_cast<t_real>(k*tl::get_one_angstrom<t_real>());
			dK = std::abs(dK);

			t_real dRefl = (*m_pinterp)(dK);
			return tl::clamp<t_real>(dRefl, 0, 1);
		}

		t_real operator()(const tl::t_length_si<t_real>& lam) const
		{
			return operator()(tl::lam2k(lam));
		}

		t_real operator()(t_real k) const
		{
			return operator()(k/tl::get_one_angstrom<t_real>());
		}

		operator bool() const { return m_bOk; }
};

#endif
