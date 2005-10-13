/*
    Returned error codes: block error category with
        code = (ASC << 8) | ASC_QUALIFIER,
    or -EIO. The latter is a bad sign...
    To be fixed later: -EINVAL when trying to read more than 0xFFFF sectors
    at once.
*/

#define REQ_CD_SET_TO   333332

static inline int req_cd_set_timeout(int (*r)(int function, ...), uint32_t tout_read_us)
{
    return r(REQ_CD_SET_TO, tout_read_us);
}

