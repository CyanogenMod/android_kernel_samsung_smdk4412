#undef MPUACC_USES_CACHED_DATA
#define MPUACC_USES_MOUNTING_MATRIX

int mpu_accel_init(struct mldl_cfg *mldl_cfg, void *accel_handle);
int mpu_accel_exit(struct mldl_cfg *mldl_cfg);
int mpu_accel_suspend(struct mldl_cfg *mldl_cfg);
int mpu_accel_resume(struct mldl_cfg *mldl_cfg);
int mpu_accel_read(struct mldl_cfg *mldl_cfg, unsigned char *buffer);
