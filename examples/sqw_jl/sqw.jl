#
# Sample Julia S(q,w) module for ferromagnetic dispersions
# @author tw
# @license GPLv2
# @date dec-2016
#

# example dispersion
function disp_ferro(q, D, offs)
	return D*q^2. + offs
end

# Gaussian peak
function gauss(x, x0, sig, amp)
	norm = (sqrt(2.*pi) * sig)
	return amp * exp(-0.5*((x-x0)/sig)^2.) / norm
end



# global variables which can be accessed / changed by Takin
g_G = vec([1., 1., 0.])	# Bragg peak
g_D = 50.				# magnon stiffness
g_offs = 0.				# energy gap
g_sig = 0.02			# linewidth
g_S0 = 10.				# intensity

g_inc_sig = 0.02	# incoherent width
g_inc_amp = 10.		# incoherent intensity



# the init function is called after Takin has changed a global variable
function TakinInit()
	println("Calling TakinInit")
end


# called for every Monte-Carlo point
function TakinSqw(h::Float64, k::Float64, l::Float64, E::Float64)
	#println("Calling TakinSqw(", h, ", ", k, ", ", l, ", ", E, ") -> ", S)
	Q = vec([h,k,l])
	q = vecnorm(Q - g_G)

	E_peak = disp_ferro(q, g_D, g_offs)

	S_p = gauss(E, E_peak, g_sig, g_S0)
	S_m = gauss(E, -E_peak, g_sig, g_S0)
	incoh = gauss(E, 0., g_inc_sig, g_inc_amp)

	S = S_p + S_m + incoh
	return Float64(S)
end
