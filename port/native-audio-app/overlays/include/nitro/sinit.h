static void NitroStaticInit(void);
#define PSP_CAT_(a,b) a##b
#define PSP_CAT(a,b) PSP_CAT_(a,b)
__attribute__((used,section(".psp_sinit"))) void (*PSP_CAT(PSPNativeCtor_,PSP_NATIVE_OV_ID))(void)=NitroStaticInit;
