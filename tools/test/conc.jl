#
# testing the calculation of the concentration matrix
# @author Tobias Weber <tobias.weber@tum.de>
# @date 6-apr-2017
# @license GPLv2
# @desc see e.g.: (Arens 2015) p. 795
#


# measurement points
A_meas = [0 1; 1 1; 2 0.5; 3 4; 4 4.5; 5 5; 6 5.5]
#A_meas = [0 10; 1 11; 2 9; 3 10; 4 10; 5 9; 6 11]


function vec2_angle(vec)
	return atan2(vec[2], vec[1])
end


function ellipse(angle, xrad, yrad, xoffs, yoffs)
	pts = []

	c = cos(angle)
	s = sin(angle)

	for t in 0.:0.01:1.
		pt = [ xrad * cos(2.*pi*t),
				yrad * sin(2.*pi*t) ]

		pt = [c -s; s c] * pt + [xoffs, yoffs]
		push!(pts, pt)
	end

	return pts
end


function correl(C)
	return C[1,2] / sqrt(C[1,1] * C[2,2])
end


# regression line: y = mx + b
function fit_line(C, offs)
	m = C[1,2] / C[1,1]
	b = offs[2] - m*offs[1]

	return [m, b]
end



# ellipsoid offsets
offs = Float64[]
for iPt = 1:size(A_meas)[2]
	push!(offs, mean(A_meas[:,iPt]))
end
println("Ellipsoid offsets: ", offs)


# centre points on mean vector
A = copy(A_meas)
for iPt = 1:size(A)[1]
	A[iPt,:] -= offs
end


C = A'*A
R = inv(C)

println("\nCovariance matrix: ", C)
println("Concentration matrix: ", R)


# test alternate calculation
C2 = zeros(size(A)[2], size(A)[2])
for iPt = 1:size(A)[1]
	pt = A[iPt,:]
	C2 += pt*pt'
end
println("Covariance matrix (alternate calc): ", C2)


# test pseudo-inverse, R*A' is only valid for full row-rank of A (here: 2)
U,S,V = svd(A)
D = diagm(S)
println("\nPseudo-inverse:", V*pinv(D)*U')
println("Pseudo-inverse (alternate calc): ", pinv(A), "\n", R*A')


evals, evecs = eig(R)
println("Eigenvectors: ", evecs)
println("Eigenvalues: ", evals)
println()


# ellipsoid axes and lengths
lens = sqrt(1./evals[:])
for iAx = 1 : size(evecs)[2]
	println("Ellipsoid axis ", iAx, ": ", evecs[:, iAx],
		" with length ", lens[iAx])
end



# assuming 2d ellipses from here on

coeff = correl(C)
println("Correlation coefficient: ", coeff, ", angle: ", acos(coeff)/pi*180.)

angle = vec2_angle(evecs[:, 1])
println("Ellipse angle: ", angle/pi*180., " deg")

m, b = fit_line(C, offs)
println("Regression line: slope = ", m, ", y0 = ", b)


# plot
open(`gnuplot -p`, "w", DevNull) do gpl
	ell = ellipse(angle, lens[1], lens[2], offs[1], offs[2])

	println(gpl, "m = ", m, "\nb = ", b)
	println(gpl, "line(x) = m*x + b")

	print(gpl, "plot \"-\" u 1:2 w points pt 7 ps 1.5 lc rgb '#ff0000' title 'measurements', ")
	print(gpl, "\"-\" u 1:2 w lines lw 1.5 lc rgb '#009900' title 'concentration ellipse', ")
	println(gpl, "line(x) lw 1.5 lc rgb '#0000ff' title 'regression line' \\\n")

	# points
	for iPt = 1:size(A_meas)[1]
		pt = A_meas[iPt,:]
		println(gpl, pt[1], " ", pt[2])
	end
	println(gpl, "e")

	# ellipse
	for pt in ell
		println(gpl, pt[1], " ", pt[2])
	end
	println(gpl, "e")	
end
