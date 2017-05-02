#
# julia test
# @author tw
#

ccall((:tst_simple_call, :tst_jl), Void, ())

ccall((:tst_string, :tst_jl), Void, (Cstring,), "Test")

ccall((:tst_array, :tst_jl), Void, (Array{Int64, 1},), [1,2,3,4,-5,-6,-7])

a = ccall((:tst_array2, :tst_jl), Array{Int64, 1}, ())
println("Array: ", a)
