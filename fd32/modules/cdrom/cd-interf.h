

typedef struct cd_dev_parm
{
    DWORD Size;
    void *DeviceId;
}
cd_dev_parm_t;

typedef struct cd_dev_info
{
    DWORD Size;
    void *DeviceId;
    char name[4];
    DWORD multiboot_id;
    int type;
}
cd_dev_info_t;

typedef struct cd_set_tout
{
    DWORD Size;
    void *DeviceId;
    DWORD tout_read_us;
}
cd_set_tout_t;

#define CD_PREMOUNT 333330
#define CD_DEV_INFO 333331
#define CD_SET_TO   333332

