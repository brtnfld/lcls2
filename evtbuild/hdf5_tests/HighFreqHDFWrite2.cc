#include <chrono>
#include <math.h>
//#include <stdio.h>
//#include <string.h>
#include <sys/stat.h>

#include "H5Cpp.h"
using namespace H5;

long GetFileSize(std::string filename)
{
  struct stat stat_buf;
  int rc = stat(filename.c_str(), &stat_buf);
  return rc == 0 ? stat_buf.st_size : -1;
}

void loop_write(const char* filename, int loop_limit, hsize_t chunk_size, hsize_t num_bytes){
    char* Cfilename;
    //    strcpy (Cfilename,filename);
    const H5std_string FILE_NAME("testhfdata.h5");
    const H5std_string DATASETNAME("ExtendibleArray");
    int32_t  data[num_bytes];//= { {1, 1}};    // data to writ
    for(hsize_t j=0; j<num_bytes; j++){
      data[j] = j;
    }

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    hsize_t dims[2] = {1,num_bytes};        // dataset dimensions at creation
    hsize_t maxdims[2] = {H5S_UNLIMITED, num_bytes}; 
    //   hsize_t dims2[2] = {10000,num_bytes};        // dataset dimensions at creation
    //hsize_t maxdims2[2] = {10000, num_bytes}; 

    hsize_t chunk_dims[2] ={chunk_size, num_bytes};
    int num_ints = (int)num_bytes/4;
    // Variables used in extending and writing to the extended portion of dataset 
    hsize_t size[2];
    hsize_t offset[2];
    hsize_t dimsext[2] = {1, num_bytes};         // extend dimensions
    size_t core;
    hid_t  fcpl = -1;
    hid_t  fapl = -1;
    hid_t  driver;         /* VFD used for file */
    hid_t  file, dataspace, dataset, filespace, memspace;
    

    fapl = H5Pcreate(H5P_FILE_ACCESS);
    core = 1; // 21474836480; //10737418240; // 512*1048576;
    //H5Pset_fapl_core(fapl, core, true);
    // H5Pset_fapl_log(fapl, "core.log", H5FD_LOG_ALL, 32*1048576 );
    //H5Pset_fapl_log(fapl, "core.log", H5FD_LOG_FILE_WRITE, 8*1048576 );
    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_WEAK);

    fcpl = H5Pcreate(H5P_FILE_CREATE);
#if 0
    H5Pset_file_space_strategy(fcpl,H5F_FSPACE_STRATEGY_PAGE,0,(hsize_t)1);
    H5Pset_file_space_page_size(fcpl, (hsize_t)(1024*1048576));
      
    H5Pset_page_buffer_size(fapl, (size_t)(1024*1048576), 0, 0);
#endif

      //FileAccPropList fapl(fapl_id);

    // fapl.setCore(core,true);//

    // FileCreatPropList fcparm;
        // Increase cache size to match chunks. Guessing on the parameters here. 
    // size_t rd_chunk_bytes = chunk_dims[0]*chunk_dims[1]*4;
    // FileAccPropList fprop;
    // fprop.setCache((int) chunk_dims[0]*chunk_dims[1], (size_t) chunk_dims[0]*chunk_dims[1], rd_chunk_bytes, 1);

//     plist_id = H5Pcreate(H5P_FILE_ACCESS);
//     core = 1073741824;
//     H5Pset_fapl_core(plist_id, core, 1);

    // Create a new file using the default property lists. 
    //H5File file(FILE_NAME, H5F_ACC_TRUNC, FileCreatPropList::DEFAULT, fapl);

    //H5Pset_cache(fapl,0, 521, 1024*1048576, 1);
    H5Pset_cache(fapl,0, 521, 1*1048576, 1);

    file = H5Fcreate("testhfdata.h5", H5F_ACC_TRUNC, fcpl, fapl);
    H5Pclose(fcpl);
    
    driver = H5Pget_driver(fapl);
    if(driver == H5FD_CORE) 
      printf("core driver \n");
    
    H5Pclose(fapl);

    dataspace = H5Screate_simple (2, dims, maxdims);
    //dataspace = H5Screate_simple (2, dims2, maxdims2);

    //  DataSpace *dataspace = new DataSpace (2, dims, maxdims);
    hid_t prop = H5Pcreate(H5P_DATASET_CREATE);
    // Modify dataset creation property to enable chunking

    H5Pset_layout(prop, H5D_CHUNKED);

    if( H5Pset_fill_time(prop, H5D_FILL_TIME_NEVER) < 0 ) {
      printf("writehdf5 error: Could not set fill time\n");
    }

    H5Pset_chunk(prop, 2, chunk_dims);
    

    //DSetCreatPropList prop;
    // prop.setChunk(2, chunk_dims);

    // fprop.setCache((int) chunk_dims[0]*chunk_dims[1], (size_t) chunk_dims[0]*chunk_dims[1]);


    // Create the chunked dataset.  Note the use of pointer.
    //  DataSet *dataset = new DataSet(file.createDataSet( DATASETNAME, 
    //                                                 PredType::STD_I32LE, *dataspace, prop) );

    //dataset = H5Dcreate(file, "ExtendibleArray", H5T_STD_I32LE, dataspace, H5P_DEFAULT, prop, H5P_DEFAULT);
    dataset = H5Dcreate(file, "ExtendibleArray", H5T_NATIVE_INT, dataspace, H5P_DEFAULT, prop, H5P_DEFAULT);

    for(int i=0; i<loop_limit; i++){
        // Extend the dataset. Dataset becomes n+1 x 3.
        size[0] = dims[0] + i*dimsext[0];
        size[1] = dims[1];

	
	H5Dset_extent (dataset, size);
	//  dataset->extend(size); 

        // Select a hyperslab in extended portion of the dataset.
	//  DataSpace *filespace = new DataSpace(dataset->getSpace ());
	
	filespace = H5Dget_space (dataset);
	

        offset[0] = size[0]-1;
        offset[1] = 0;
        // filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);
	H5Sselect_hyperslab (filespace, H5S_SELECT_SET, offset, NULL, dimsext, NULL);

        // Define memory space.
        //DataSpace *memspace = new DataSpace(2, dimsext, NULL);
	memspace = H5Screate_simple (2, dimsext, NULL);
        // Write data to the extended portion of the dataset.
	//   dataset->write(data, PredType::NATIVE_INT, *memspace, *filespace);
	H5Dwrite(dataset, H5T_NATIVE_INT, memspace, filespace, H5P_DEFAULT,data);
        // delete filespace;
	H5Sclose(filespace);
	H5Sclose(memspace);
	// H5Fflush(file,H5F_SCOPE_LOCAL);
        //delete filespace;
        //delete memspace;

    };

    // Close all objects and file.
    //prop.close();
    H5Pclose(prop);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    //  delete dataspace;
    //delete dataset;
    // printf("******* H5Fclose -- start ******** \n");
    std::chrono::high_resolution_clock::time_point ta = std::chrono::high_resolution_clock::now();
    H5Fclose(file);
    std::chrono::high_resolution_clock::time_point tb = std::chrono::high_resolution_clock::now();
    int durationA = std::chrono::duration_cast<std::chrono::milliseconds>( tb - ta ).count();
    // printf("H5Fclose %-20i\n", durationA);

    //  file.close();

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    int duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    auto fileSize = (float) GetFileSize(FILE_NAME)/1000000; //MB
    float av_speed = 1000*fileSize/(duration); // MB/s
    float av_freq = float(loop_limit)/duration;
    float hdf_ratio = 1000000*fileSize/(num_bytes*loop_limit*4.0);

    //  printf("%-20i%-20i%-20i%-20i%-20.2f%-20.2f%-20.2f%-20.2f\n", chunk_size , loop_limit, 4*num_bytes, duration, fileSize, hdf_ratio, av_speed, av_freq); 
        };



int main (int argc, char *argv[])
{

    const H5std_string FILE_NAME(argv[1]);   
    const H5std_string DATASETNAME("ExtendibleArray");


    // void loop_write(const char* filename, size_t loop_limit, hsize_t chunk_size, size_t num_bytes){
    auto chunk_size = (hsize_t) atoi(argv[3]);
       if(atoi(argv[3])==1){
	 printf("%-20s%-20s%-20s%-20s%-20s%-20s%-20s%-20s\n", "#Chunk size", "Loop limit", "Bytes/extension", "Duration (ms)", "Filesize (MB)", "HDF Ratio", "Write speed (MB/s)", "Frequency (kHz)");
    };
       loop_write(argv[1], atoi(argv[2]), chunk_size,(size_t)atoi(argv[4])/4);
    return 0;  // successfully terminated
}
