#include <chrono>
#include <math.h>
#include <sys/stat.h>

#include "H5Cpp.h"

using namespace H5;

const H5std_string DATASET_NAME( "ExtendibleArray" );
hsize_t dims[2]; 	// dataset dimensions

long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
};

void loop_read(const char* filename, int buffer_size){
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    //  const H5std_string FILE_NAME( filename );
    const H5std_string FILE_NAME("testhfdata.h5");
    FileCreatPropList fcparm;
    FileAccPropList fprop;

    hid_t  fcpl = -1;
    hid_t  fapl = -1;
    hid_t  driver;         /* VFD used for file */
    hid_t  file, dataspace, dataset, filespace, memspace;
    hsize_t chunk_dims[2];
    int rank_chunk;

    fapl = H5Pcreate(H5P_FILE_ACCESS);
    // core = 1; // 21474836480; //10737418240; // 512*1048576;
    //H5Pset_fapl_core(fapl, core, true);
    // H5Pset_fapl_log(fapl, "core.log", H5FD_LOG_ALL, 32*1048576 );
    //H5Pset_fapl_log(fapl, "core.log", H5FD_LOG_FILE_WRITE, 8*1048576 );
    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_WEAK);

    H5Pset_cache(fapl,0, 521, buffer_size*1048576, 1);

    //Set a 64 Mb cache
    //fprop.setCache((int) 0, (size_t) buffer_size, buffer_size*1048576, 1);

    file = H5Fopen("testhfdata.h5", H5F_ACC_RDONLY, fapl);
    // H5File file(FILE_NAME, H5F_ACC_RDONLY,fcparm, fprop);

    H5Pclose(fapl);
    
    dataset = H5Dopen(file, "ExtendibleArray", H5P_DEFAULT);

    // DataSet dataset = file.openDataSet(DATASET_NAME);

    // H5T_class_t type_class = dataset.getTypeClass();

    dataspace = H5Dget_space(dataset);
    // DataSpace dataspace = dataset.getSpace();
    
    int rank = H5Sget_simple_extent_dims(dataspace, dims, NULL);

    // int rank = dataspace.getSimpleExtentDims( dims );

    hsize_t row_sel[1] = {dims[1]}, offset[2] = {0, 0}, count[2] = {1, dims[1]};
    int row_out[dims[1]];

//     DSetCreatPropList cparms = dataset.getCreatePlist();
//     rank_chunk = cparms.getChunk( 2, chunk_dims);

    hid_t dcpl = H5Dget_create_plist(dataset);
    H5Pget_chunk(dcpl, 2, chunk_dims);
    H5Pclose(dcpl);


    for(hsize_t i =0; i<dims[0]; i++){

      memspace = H5Screate_simple (1, row_sel, NULL);

      H5Sselect_hyperslab (dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);

      //  dataspace.selectHyperslab( H5S_SELECT_SET, count, offset );
      
      H5Dread (dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, row_out);

      //   dataset.read(row_out, PredType::NATIVE_INT, memspace, dataspace);
      offset[0]+=1;
      H5Sclose(memspace);

//       for (hsize_t j=0; j<dims[1]; j++) {
// 	printf (" %3d", row_out[j]);
//       }


    }
    H5Sclose(dataspace);
    H5Dclose(dataset);
    H5Fclose(file);

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    int duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
    auto fileSize = (float) GetFileSize(FILE_NAME)/1000000; //MB
    float av_speed = 1000*fileSize/(duration); // MB/s
    float av_freq = float(dims[0])/duration;
    float hdf_ratio = 1000000*fileSize/(sizeof(row_out)*dims[0]);

    printf("\e[1;32m%-20i%-20i%-20i%-20i%-20.2f%-20.2f%-20.2f%-20.2f\e[m\n", chunk_dims[0], dims[0], dims[1]*4, duration, fileSize, hdf_ratio, av_speed, av_freq); 

}


int main(int argc,  char *argv[]){

  //printf("%-20s%-20s%-20s%-20s%-20s%-20s%-20s%-20s\n", "Chunk size", "Loop limit", "Bytes/extension", "Duration (ms)", "Filesize (MB)", "HDF Ratio", "Read  speed (MB/s)", "Frequency (kHz)"); 
    if(atoi(argv[2])==1){
        printf("%-20s%-20s%-20s%-20s%-20s%-20s%-20s%-20s\n", "#Chunk size", "Loop limit", "Bytes/extension", "Duration (ms)", "Filesize (MB)", "HDF Ratio", "Read  speed (MB/s)", "Frequency (kHz)");
    };
    loop_read(argv[1], atoi(argv[3]));
    return 0;
}
