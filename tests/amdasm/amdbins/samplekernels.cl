kernel void add(uint n, const global uint* adat, const global uint* bdat,
            global uint* cdat)
{
    size_t gid = get_global_id(0);
    if (gid >= n) return;
    cdat[gid] = adat[gid]+bdat[gid];
}

kernel void multiply(uint n, const global uint* adat, const global uint* bdat,
            global uint* cdat)
{
    size_t gid = get_global_id(0);
    if (gid >= n) return;
    cdat[gid] = adat[gid]*bdat[gid];
}
