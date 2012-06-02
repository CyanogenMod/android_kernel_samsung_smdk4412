   zreladdr-y	:= 0x20008000
params_phys-y	:= 0x20000100

   zreladdr-$(CONFIG_CPU_S5PC110)       := 0x30008000
params_phys-$(CONFIG_CPU_S5PC110)       := 0x30000100
