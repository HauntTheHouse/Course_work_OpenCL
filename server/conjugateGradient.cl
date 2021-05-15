__kernel void conjugateGradient(int dim, int num_vals,
                                __local float *r, __global float *x, __local float * A_times_p, __local float *p,
                              __global int *rows, __global int *cols, __global float *A,
                              __global float *b, __global float *result)
{
//    local float
//  t)r[dim], x[dim], A_times_p[dim], p[dim];
    local float alpha, r_length, old_r_dot_r, new_r_dot_r;
    local int iteration;

    int id = get_local_id(0);
    int start_index = -1;
    int end_index = -1;
    float Ap_dot_p;

    for (int i = id; i < num_vals; i++)
    {
        if ((rows[i] == id) && (start_index == -1))
        {
            start_index = i;
        }
        else if ((rows[i] == id + 1) && (end_index == -1))
        {
            end_index = i - 1;
            break;
        }
        else if ((i == num_vals - 1) && (end_index == -1))
        {
            end_index = i;
        }
    }

    x[id] = 0.0;
    r[id] = b[id];
    p[id] = b[id];
    barrier(CLK_LOCAL_MEM_FENCE);

    if (id == 0)
    {
        old_r_dot_r = 0.0;
        for (int i = 0; i < dim; i++)
        {
            old_r_dot_r += r[i] * r[i];
        }
        r_length = sqrt(old_r_dot_r);
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    iteration = 0;
    while ((iteration < 1000) && (r_length >= 0.01f))
    {
        A_times_p[id] = 0.0;
        for (int i = start_index; i <= end_index; i++)
        {
            A_times_p[id] += A[i] * p[cols[i]];
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        if (id == 0)
        {
            Ap_dot_p = 0.0;
            for (int i = 0; i < dim; i++)
            {
                Ap_dot_p += A_times_p[i] * p[i];
            }
            alpha = old_r_dot_r/Ap_dot_p;
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        x[id] += alpha * p[id];
        r[id] -= alpha * A_times_p[id];
        barrier(CLK_LOCAL_MEM_FENCE);

        if (id == 0)
        {
            new_r_dot_r = 0.0;
            for (int i = 0; i < dim; i++)
            {
                new_r_dot_r += r[i] * r[i];
            }
            r_length = sqrt(new_r_dot_r);
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        p[id] = r[id] + (new_r_dot_r/old_r_dot_r) * p[id];
        barrier(CLK_LOCAL_MEM_FENCE);

        old_r_dot_r = new_r_dot_r;

        if (id == 0)
        {
            iteration++;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    result[0] = (float) iteration;
    result[1] = r_length;
}