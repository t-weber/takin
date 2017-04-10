/**
 * hdf test
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 9-apr-17
 * @license GPLv2
 * 
 * gcc -lstdc++ -o tst_hdf tst_hdf.cpp -lstdc++ -lhdf5
 */

#include <hdf5.h>
#include <iostream>
#include <vector>

int main()
{
	if(::H5open() < 0)
	{
		std::cerr << "Cannot init hdf5." << std::endl;
		return -1;
	}

	hbool_t bIsSafe = 0;
	if(::H5is_library_threadsafe(&bIsSafe) >= 0)
		std::cout << "Hdf5 is thread-safe: " << bIsSafe << std::endl;


	hid_t file1 = ::H5Fcreate("tst.hdf", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if(file1 < 0)
	{
		std::cerr << "Cannot create file." << std::endl;
		return -1;
	}

	hsize_t iDims[] = { 5 };
	hid_t set1 = ::H5Screate_simple(1, iDims, 0);
	if(set1 < 0)
		std::cerr << "Cannot create data set." << std::endl;

	hid_t data1 = ::H5Dcreate(file1, "/sqw", H5T_NATIVE_DOUBLE, set1, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if(data1 < 0)
		std::cerr << "Cannot open data." << std::endl;

	double pdArr[] = { 1., 2., 3., 4., 5. };
	if(::H5Dwrite(data1, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, pdArr) < 0)
		std::cerr << "Cannot write data." << std::endl;


	::H5Fflush(file1, H5F_SCOPE_GLOBAL);
	::H5Sclose(set1);
	::H5Dclose(data1);
	::H5Fclose(file1);


	// ------------------------------------------------------------------------


	hid_t file2 = ::H5Fopen("tst.hdf", H5F_ACC_RDONLY, H5P_DEFAULT);
	if(file1 < 0)
	{
		std::cerr << "Cannot open file." << std::endl;
		return -1;
	}

	hid_t data2 = ::H5Dopen(file2, "/sqw", H5P_DEFAULT);
	if(data2 < 0)
		std::cerr << "Cannot open data." << std::endl;
	hid_t set2 = ::H5Dget_space(data2);
	if(set2 < 0)
		std::cerr << "Cannot create data set." << std::endl;

	int iRank = ::H5Sget_simple_extent_ndims(set2);
	std::vector<hsize_t> vecDim(iRank);
	::H5Sget_simple_extent_dims(set2, vecDim.data(), 0);
	::H5Sclose(set2);
	std::cout << "Rank: " << iRank << ", dims: ";
	for(hsize_t iDim : vecDim)
		std::cout << iDim << ", ";
	std::cout << std::endl;

	std::vector<double> vecData(vecDim[0]);
	if(::H5Dread(data2, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, vecData.data()) < 0)
		std::cerr << "Cannot read data." << std::endl;

	std::cout << "Data: ";
	for(double d : vecData)
		std::cout << d << " ";
	std::cout << std::endl;

	::H5Dclose(data2);
	::H5Fclose(file2);


	::H5close();
	return 0;
}
