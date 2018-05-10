#
# Sample Python S(q,w) module for simulating Bragg peaks
#
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
# @date may-2018
#

import math as m

import numpy as np
import numpy.linalg as la
from numpy import array	# in global namespace so that Takin can access it

import scipy as sp
import scipy.constants as const



# -----------------------------------------------------------------------------
# Helper functions
# -----------------------------------------------------------------------------

# Gaussian peak
def gauss(x, x0, sig, amp):
	norm = (np.sqrt(2.*m.pi) * sig)
	return amp * np.exp(-0.5*((x-x0)/sig)**2.) / norm

# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Takin interface
# -----------------------------------------------------------------------------

# global variables which can be accessed / changed by Takin

# Bragg peaks
g_Gs =	\
[ 	\
	np.array([+1., 0., 0.]),	\
	np.array([-1., 0., 0.]),	\
\
	np.array([0., +1., 0.]),	\
	np.array([0., -1., 0.]),	\
\
	np.array([+1., +1., 0.]),	\
	np.array([-1., -1., 0.]),	\
	np.array([+1., -1., 0.]),	\
	np.array([-1., +1., 0.])	\
]

g_sig_q = 0.01		# transversal width: mosaic, longitudinal width: Delta d/d
g_sig_E = 0.01		# energy width
g_amp = 1.		# intensity



#
# the init function is called after Takin has changed a global variable (optional)
#
def TakinInit():
	print("Init: Gs = " + repr(g_Gs))


#
# S(Q,E) function, called for every Monte-Carlo point
#
def TakinSqw(h, k, l, E):
	try:
#		print("h={0}, k={1}, l={2}, E={3}".format(h,k,l,E))
		S = 0.

		for G in g_Gs:
			Q = np.array([h, k, l])
			q = Q - G
			q_len = np.sqrt(np.dot(q, q))

			S_q = gauss(q_len, 0, g_sig_q, g_amp)
			S_E = gauss(E, 0, g_sig_E, g_amp)

			S += S_q * S_E

#		print("S={0}".format(S))
		return S

	except ZeroDivisionError:
		return 0.

# -----------------------------------------------------------------------------


import os
print("Script working directory: " + os.getcwd())


# test
#TakinInit()
#print(TakinSqw(1,1,0,0))
