#include <stdbool.h>
#include "mpi.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <Python.h>

int hpat_dist_get_rank();
int hpat_dist_get_size();
int64_t hpat_dist_get_start(int64_t total, int num_pes, int node_id);
int64_t hpat_dist_get_end(int64_t total, int num_pes, int node_id);
int64_t hpat_dist_get_node_portion(int64_t total, int num_pes, int node_id);
double hpat_dist_get_time();
double hpat_get_time();
int hpat_barrier();
MPI_Datatype get_MPI_typ(int typ_enum);
MPI_Datatype get_val_rank_MPI_typ(int typ_enum);
MPI_Op get_MPI_op(int op_enum);
int get_elem_size(int type_enum);
void hpat_dist_reduce(char *in_ptr, char *out_ptr, int op, int type_enum);

int hpat_dist_exscan_i4(int value);
int64_t hpat_dist_exscan_i8(int64_t value);
float hpat_dist_exscan_f4(float value);
double hpat_dist_exscan_f8(double value);

int hpat_dist_arr_reduce(void* out, int64_t* shapes, int ndims, int op_enum, int type_enum);
int hpat_dist_irecv(void* out, int size, int type_enum, int pe, int tag, bool cond);
int hpat_dist_isend(void* out, int size, int type_enum, int pe, int tag, bool cond);
int hpat_dist_wait(int req, bool cond);

void c_alltoallv(void* send_data, void* recv_data, int* send_counts,
                int* recv_counts, int* send_disp, int* recv_disp, int typ_enum);
int64_t hpat_dist_get_item_pointer(int64_t ind, int64_t start, int64_t count);
int hpat_dummy_ptr[64];
void* hpat_get_dummy_ptr() {
    return hpat_dummy_ptr;
}

PyMODINIT_FUNC PyInit_hdist(void) {
    PyObject *m;
    static struct PyModuleDef moduledef = {
            PyModuleDef_HEAD_INIT, "hdist", "No docs", -1, NULL, };
    m = PyModule_Create(&moduledef);
    if (m == NULL)
        return NULL;

    PyObject_SetAttrString(m, "hpat_dist_get_rank",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_rank)));
    PyObject_SetAttrString(m, "hpat_dist_get_size",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_size)));
    PyObject_SetAttrString(m, "hpat_dist_get_start",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_start)));
    PyObject_SetAttrString(m, "hpat_dist_get_end",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_end)));
    PyObject_SetAttrString(m, "hpat_dist_get_node_portion",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_node_portion)));
    PyObject_SetAttrString(m, "hpat_dist_get_time",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_time)));
    PyObject_SetAttrString(m, "hpat_get_time",
                            PyLong_FromVoidPtr((void*)(&hpat_get_time)));
    PyObject_SetAttrString(m, "hpat_barrier",
                            PyLong_FromVoidPtr((void*)(&hpat_barrier)));

    PyObject_SetAttrString(m, "hpat_dist_reduce",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_reduce)));

    PyObject_SetAttrString(m, "hpat_dist_exscan_i4",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_exscan_i4)));
    PyObject_SetAttrString(m, "hpat_dist_exscan_i8",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_exscan_i8)));
    PyObject_SetAttrString(m, "hpat_dist_exscan_f4",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_exscan_f4)));
    PyObject_SetAttrString(m, "hpat_dist_exscan_f8",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_exscan_f8)));

    PyObject_SetAttrString(m, "hpat_dist_arr_reduce",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_arr_reduce)));
    PyObject_SetAttrString(m, "hpat_dist_irecv",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_irecv)));
    PyObject_SetAttrString(m, "hpat_dist_isend",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_isend)));
    PyObject_SetAttrString(m, "hpat_dist_wait",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_wait)));
    PyObject_SetAttrString(m, "hpat_dist_get_item_pointer",
                            PyLong_FromVoidPtr((void*)(&hpat_dist_get_item_pointer)));
    PyObject_SetAttrString(m, "hpat_get_dummy_ptr",
                            PyLong_FromVoidPtr((void*)(&hpat_get_dummy_ptr)));
    PyObject_SetAttrString(m, "c_alltoallv",
                            PyLong_FromVoidPtr((void*)(&c_alltoallv)));
    return m;
}

int hpat_dist_get_rank()
{
    int is_initialized;
    MPI_Initialized(&is_initialized);
    if (!is_initialized)
        MPI_Init(NULL, NULL);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // printf("my_rank:%d\n", rank);
    return rank;
}

int hpat_dist_get_size()
{
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // printf("mpi_size:%d\n", size);
    return size;
}

int64_t hpat_dist_get_start(int64_t total, int num_pes, int node_id)
{
    int64_t div_chunk = (int64_t)ceil(total/((double)num_pes));
    int64_t start = std::min(total, node_id*div_chunk);
    // printf("rank %d start:%lld\n", node_id, start);
    return start;
}

int64_t hpat_dist_get_end(int64_t total, int num_pes, int node_id)
{
    int64_t div_chunk = (int64_t)ceil(total/((double)num_pes));
    int64_t end = std::min(total, (node_id+1)*div_chunk);
    // printf("rank %d end:%lld\n", node_id, end);
    return end;
}

int64_t hpat_dist_get_node_portion(int64_t total, int num_pes, int node_id)
{
    int64_t div_chunk = (int64_t)ceil(total/((double)num_pes));
    int64_t start = std::min(total, node_id*div_chunk);
    int64_t end = std::min(total, (node_id+1)*div_chunk);
    int64_t portion = end-start;
    // printf("rank %d portion:%lld\n", node_id, portion);
    return portion;
}

double hpat_dist_get_time()
{
    double wtime;
    MPI_Barrier(MPI_COMM_WORLD);
    wtime = MPI_Wtime();
    return wtime;
}

double hpat_get_time()
{
    double wtime;
    wtime = MPI_Wtime();
    return wtime;
}

int hpat_barrier()
{
    MPI_Barrier(MPI_COMM_WORLD);
    return 0;
}

void hpat_dist_reduce(char *in_ptr, char *out_ptr, int op_enum, int type_enum)
{
    // printf("reduce value: %d\n", value);
    MPI_Datatype mpi_typ = get_MPI_typ(type_enum);
    MPI_Datatype mpi_op = get_MPI_op(op_enum);

    // argmax and argmin need special handling
    if (mpi_op==MPI_MAXLOC || mpi_op==MPI_MINLOC)
    {
        // since MPI's indexed types use 32 bit integers, we workaround by
        // using rank as index, then broadcasting the actual values from the
        // target rank.
        // TODO: generate user-defined reduce operation to avoid this workaround
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        // allreduce struct is value + integer
        int value_size;
        MPI_Type_size(mpi_typ, &value_size);
        // copy input index_value to output
        memcpy(out_ptr, in_ptr, value_size+ sizeof(int64_t));
        // printf("rank:%d index:%lld value:%lf value_size:%d\n", rank,
        //     *(int64_t*)in_ptr, *(double*)(in_ptr+sizeof(int64_t)), value_size);

        // format: value + int (input format is int64+value)
        char *in_val_rank = (char*) malloc(value_size+sizeof(int));
        char *out_val_rank = (char*) malloc(value_size+sizeof(int));

        char *in_val_ptr = in_ptr + sizeof(int64_t);
        memcpy(in_val_rank, in_val_ptr, value_size);
        memcpy(in_val_rank+value_size, &rank, sizeof(int));
        // TODO: support int64_int value on Windows
        MPI_Datatype val_rank_mpi_typ = get_val_rank_MPI_typ(type_enum);
        MPI_Allreduce(in_val_rank, out_val_rank, 1, val_rank_mpi_typ, mpi_op, MPI_COMM_WORLD);

        int target_rank = *((int*)(out_val_rank+value_size));
        // printf("rank:%d allreduce rank:%d val:%lf\n", rank, target_rank, *(double*)out_val_rank);
        MPI_Bcast(out_ptr, value_size+sizeof(int64_t), MPI_BYTE, target_rank, MPI_COMM_WORLD);
        free(in_val_rank);
        free(out_val_rank);
        return;
    }

    MPI_Allreduce(in_ptr, out_ptr, 1, mpi_typ, mpi_op, MPI_COMM_WORLD);
    return;
}

int hpat_dist_arr_reduce(void* out, int64_t* shapes, int ndims, int op_enum, int type_enum)
{
    int i;
    // printf("ndims:%d shape: ", ndims);
    // for(i=0; i<ndims; i++)
    //     printf("%d ", shapes[i]);
    // printf("\n");
    // fflush(stdout);
    int total_size = (int)shapes[0];
    for(i=1; i<ndims; i++)
        total_size *= (int)shapes[i];
    MPI_Datatype mpi_typ = get_MPI_typ(type_enum);
    MPI_Datatype mpi_op = get_MPI_op(op_enum);
    int elem_size = get_elem_size(type_enum);
    void* res_buf = malloc(total_size*elem_size);
    MPI_Allreduce(out, res_buf, total_size, mpi_typ, mpi_op, MPI_COMM_WORLD);
    memcpy(out, res_buf, total_size*elem_size);
    free(res_buf);
    return 0;
}


int hpat_dist_exscan_i4(int value)
{
    // printf("sum value: %d\n", value);
    int out=0;
    MPI_Exscan(&value, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    return out;
}

int64_t hpat_dist_exscan_i8(int64_t value)
{
    // printf("sum value: %lld\n", value);
    int64_t out=0;
    MPI_Exscan(&value, &out, 1, MPI_LONG_LONG_INT, MPI_SUM, MPI_COMM_WORLD);
    return out;
}

float hpat_dist_exscan_f4(float value)
{
    // printf("sum value: %f\n", value);
    float out=0;
    MPI_Exscan(&value, &out, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    return out;
}

double hpat_dist_exscan_f8(double value)
{
    // printf("sum value: %lf\n", value);
    double out=0;
    MPI_Exscan(&value, &out, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    return out;
}

int hpat_dist_irecv(void* out, int size, int type_enum, int pe, int tag, bool cond)
{
    MPI_Request mpi_req_recv = -1;
    // printf("irecv size:%d pe:%d tag:%d, cond:%d\n", size, pe, tag, cond);
    // fflush(stdout);
    if(cond)
    {
        MPI_Datatype mpi_typ = get_MPI_typ(type_enum);
        MPI_Irecv(out, size, mpi_typ, pe, tag, MPI_COMM_WORLD, &mpi_req_recv);
    }
    // printf("after irecv size:%d pe:%d tag:%d, cond:%d\n", size, pe, tag, cond);
    // fflush(stdout);
    return mpi_req_recv;
}

int hpat_dist_isend(void* out, int size, int type_enum, int pe, int tag, bool cond)
{
    MPI_Request mpi_req_recv = -1;
    // printf("isend size:%d pe:%d tag:%d, cond:%d\n", size, pe, tag, cond);
    // fflush(stdout);
    if(cond)
    {
        MPI_Datatype mpi_typ = get_MPI_typ(type_enum);
        MPI_Isend(out, size, mpi_typ, pe, tag, MPI_COMM_WORLD, &mpi_req_recv);
    }
    // printf("after isend size:%d pe:%d tag:%d, cond:%d\n", size, pe, tag, cond);
    // fflush(stdout);
    return mpi_req_recv;
}

int hpat_dist_wait(int req, bool cond)
{
    if (cond)
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    return 0;
}

// _h5_typ_table = {
//     int8:0,
//     uint8:1,
//     int32:2,
//     int64:3,
//     float32:4,
//     float64:5
//     }

MPI_Datatype get_MPI_typ(int typ_enum)
{
    // printf("h5 type enum:%d\n", typ_enum);
    if (typ_enum < 0 || typ_enum > 6)
    {
        std::cerr << "Invalid MPI_Type" << "\n";
        return MPI_LONG_LONG_INT;
    }
    MPI_Datatype types_list[] = {MPI_CHAR, MPI_UNSIGNED_CHAR,
            MPI_INT, MPI_LONG_LONG_INT, MPI_FLOAT, MPI_DOUBLE,
            MPI_UNSIGNED_LONG_LONG};
    return types_list[typ_enum];
}

MPI_Datatype get_val_rank_MPI_typ(int typ_enum)
{
    // printf("h5 type enum:%d\n", typ_enum);
    // XXX: LONG is used for int64, which doesn't work on Windows
    // XXX: LONG is used for uint64
    if (typ_enum < 0 || typ_enum > 6)
    {
        std::cerr << "Invalid MPI_Type" << "\n";
        return 8;
    }
    MPI_Datatype types_list[] = {MPI_UNDEFINED, MPI_UNDEFINED,
            MPI_2INT, MPI_LONG_INT, MPI_FLOAT_INT, MPI_DOUBLE_INT, MPI_LONG_INT};
    return types_list[typ_enum];
}

// from distributed_api Reduce_Type
MPI_Op get_MPI_op(int op_enum)
{
    // printf("op type enum:%d\n", op_enum);
    if (op_enum < 0 || op_enum > 5)
    {
        std::cerr << "Invalid MPI_Op" << "\n";
        return MPI_SUM;
    }
    MPI_Op ops_list[] = {MPI_SUM, MPI_PROD, MPI_MIN, MPI_MAX, MPI_MINLOC,
            MPI_MAXLOC};

    return ops_list[op_enum];
}

int get_elem_size(int type_enum)
{
    if (type_enum < 0 || type_enum > 6)
    {
        std::cerr << "Invalid MPI_Type" << "\n";
        return 8;
    }
    int types_sizes[] = {1,1,4,8,4,8,8};
    return types_sizes[type_enum];
}

int64_t hpat_dist_get_item_pointer(int64_t ind, int64_t start, int64_t count)
{
    // printf("ind:%lld start:%lld count:%lld\n", ind, start, count);
    if (ind >= start && ind < start+count)
        return ind-start;
    return -1;
}


void c_alltoallv(void* send_data, void* recv_data, int* send_counts,
                int* recv_counts, int* send_disp, int* recv_disp, int typ_enum)
{
    MPI_Datatype mpi_typ = get_MPI_typ(typ_enum);
    MPI_Alltoallv(send_data, send_counts, send_disp, mpi_typ,
        recv_data, recv_counts, recv_disp, mpi_typ, MPI_COMM_WORLD);
}
