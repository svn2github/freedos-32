/*
    Returned error codes: block error category with
        code = (ASC << 8) | ASC_QUALIFIER,
    or -EIO. The latter is a bad sign...
    To be fixed later: -EINVAL when trying to read more than 0xFFFF sectors
    at once.
*/

#define REQ_CD_SET_TO   333332
#define REQ_CD_EXTENDED_ERROR_REPORT    333334
#define REQ_CD_GET_LAST_SESSION 333336

#define CD_ERROR_FLAG_DEFERRED 1
#define CD_ERROR_FLAG_SENSE_INVALID 2

struct cd_error_info
{
    int error_code; /* This is the same as the normal block device error return code */
    int flags;
    int error_type;
    uint32_t extra_inf[2];
};

struct cd_device;

static inline int req_cd_set_timeout(int (*r)(int function, ...), uint32_t tout_read_us)
{
    return r(REQ_CD_SET_TO, tout_read_us);
}

static inline int req_cd_extended_error_report(int (*r)(int function, ...), struct cd_error_info** ei)
{
    return r(REQ_CD_EXTENDED_ERROR_REPORT, ei);
}

static inline int req_cd_get_last_session(int (*r)(int function, ...), struct cd_device *d, uint32_t *last_session_address)
{
    return r(REQ_CD_GET_LAST_SESSION, d, last_session_address);
}
