g_tst = 123.


function TakinInit()
	println("Calling TakinInit")
end


function TakinSqw(h, k, l, E)
	println("Calling TakinSqw(", h, ", ", k, ", ", l, ", ", E, ")")
	return g_tst
end
