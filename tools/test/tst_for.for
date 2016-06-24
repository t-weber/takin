! gfortran -c -ffree-form -o tst_for.o tst_for.for

subroutine add(a,b, result)
	real*8 a, b, result

	result = a+b

	return
	end


subroutine pr(str)
	character str(5)

	print *, "Got string: ", str

	return
	end


function sub(a,b)
	real*8 a,b, result
	
	result = a-b
	return
	end
