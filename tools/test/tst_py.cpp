//gcc -std=c++11 -I /usr/include/python2.7 -o tst_py tst_py.cpp -lstdc++ -lboost_python -lpython2.7

#include <iostream>

#include <boost/python.hpp>
namespace py = boost::python;


void list_keys(py::dict dict)
{
	std::cout << py::len(dict.items()) << " keys in dict:" << std::endl;
	for(int i=0; i<py::len(dict.items()); ++i)
	{
		std::cout << std::string(py::extract<std::string>(dict.items()[i][0]))
			<< std::endl;
	}
}

void call_py_fkt()
{
	try
	{
		py::object sys = py::import("sys");
		py::dict sysdict = py::extract<py::dict>(sys.attr("__dict__"));
		py::list path = py::extract<py::list>(sysdict["path"]);
		path.append(".");

		py::object mod = py::import("sqw");
		py::dict moddict = py::extract<py::dict>(mod.attr("__dict__"));
		py::object Sqw = moddict["TakinSqw"];

		for(double dE=0.; dE<1.; dE+=0.1)
		{
			double dS = py::extract<double>(Sqw(1., 1., 1., dE));
			std::cout << dS << std::endl;
		}



		py::object mn = py::import("__main__");
		py::dict mndict = py::extract<py::dict>(mn.attr("__dict__"));

		std::cout << "\nmain dict:\n";
		list_keys(mndict);

		//std::cout << "\nsys dict:\n";
		//list_keys(sysdict);

		std::cout << "\nmod dict:\n";
		list_keys(moddict);
	}
	catch(const py::error_already_set& ex)
	{
		PyErr_Print();
		PyErr_Clear();
	}
}


int main(int argc, char** argv)
{
	Py_Initialize();
	call_py_fkt();

	//Py_Finalize();
	return 0;
}
