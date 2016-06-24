#
# Sample Python S(q,w) module for (anti-)ferromagnetic dispersions
# @author tw
# @license GPLv2
# @date jun-2016
#

import math as m

import numpy as np
import numpy.linalg as la
from numpy import array	# in global namespace so that Takin can access it

import scipy as sp
import scipy.constants as const



# -----------------------------------------------------------------------------
# dispersion
# -----------------------------------------------------------------------------

# kB in meV/K
kB = const.k / const.e * 1e3


# dispersion relations
def disp_ferro(q, D, offs):
	return D*q**2. + offs

def disp_antiferro(q, D, offs):
	return D*q + offs


# Gaussian peak
def gauss(x, x0, sig, amp):
	norm = (np.sqrt(2.*m.pi) * sig)
	return amp * np.exp(-0.5*((x-x0)/sig)**2.) / norm

# Bose factor
def bose(E, T):
	if E >= 0.:
		return 1./(m.exp(abs(E)/(kB*T)) - 1.) + 1.;
	else:
		return 1./(m.exp(abs(E)/(kB*T)) - 1.);

# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Takin interface
# -----------------------------------------------------------------------------

# global variables which can be accessed / changed by Takin
g_G = np.array([1., 1., 0.])	# Bragg peak

g_D = 20.			# magnon stiffness
g_offs = 0.			# energy gap
g_sig = 0.02		# linewidth
g_S0 = 1.			# intensity

g_inc_sig = 0.02	# incoherent width
g_inc_amp = 1.		# incoherent intensity

g_T = 300.			# temperature

g_disp = 0			# which dispersion?



# the init function is called after Takin has changed a global variable
def TakinInit():
	print("Init: G=" + repr(g_G) + ", T=" + repr(g_T))



# called for every Monte-Carlo point
def TakinSqw(h, k, l, E):
	try:
		Q = np.array([h,k,l])
		q = la.norm(Q - g_G)

		E_peak = 0.
		if g_disp == 0:
			E_peak = disp_ferro(q, g_D, g_offs)
		elif g_disp == 1:
			E_peak = disp_antiferro(q, g_D, g_offs)
		else:
			return 0.

		S_p = gauss(E, E_peak, g_sig, g_S0)
		S_m = gauss(E, -E_peak, g_sig, g_S0)
		incoh = gauss(E, 0., g_inc_sig, g_inc_amp)

		return (S_p + S_m)*bose(E, g_T) + incoh
	except ZeroDivisionError:
		return 0.

# -----------------------------------------------------------------------------
