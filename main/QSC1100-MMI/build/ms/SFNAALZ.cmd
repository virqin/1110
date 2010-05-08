@echo off

del /f /a buildSFNAALZ.log.old

if exist buildSFNAALZ.log move buildSFNAALZ.log buildSFNAALZ.log.old > nul

make -r -f dmss1100.mak USES_SECBOOT_2_0=yes USES_SPANSION_S29WS256N0SB=yes USES_HS_USB_ON_MODEM_PROC=yes USES_BREW_3.1=yes USES_LINKER_XREFS=yes USES_HS_USB_PMIC_PHY=yes USES_FDI=yes USES_XO=yes USES_NOR_PSRAM=yes USES_OSBL_SBI_PMIC_LIB=yes USES_AUDFMT_AMR=yes USES_IALARM=yes USES_NO_ATCOP=yes USES_CDMA=yes USES_NO_STRIP_NO_ODM=yes USES_RFMSM=yes USES_OEMSBL_SPLASH_SCREEN=yes USES_ROMBOOT=yes USES_CHARGER=yes USES_BREW_SMS=yes USES_GENERATE_MDSP=yes USES_LOWTIER_LOWMEM=yes USES_FSBL_SBI_PMIC_LIB=yes USES_PROCESS_ERROR_CAL=yes USES_MEM_USAGE_HTML=yes USES_OSBL_CHG_SCREEN=yes USES_CLEAN_REMOVE=yes USES_AUTH=yes USES_BREW_BUIW=yes USES_QHSUSB_DEAD_BATTERY_CHARGING=yes USES_SURF=yes USES_ERR_EXCLUDE_RESET_DETECT=yes USES_SPLIT_CODE_DATA=yes USES_REMOVE_WMS_RESOURCES=yes USES_HSU=yes USES_SNDCTL=yes USES_SBI=yes USES_AUDIOEQ=yes USES_UIM=yes USES_BREW_APPMGR=yes USES_NORPRG_HDR=yes USES_EFS2=yes USES_ZBLD=yes USES_HS_USB_CHG_REMOTE_API=yes USES_BOOT_SPLASH_SCREEN=yes USES_BUILD_FLASHTOOLS=yes USES_STRIP=yes USES_QSC1100_TARGET=yes USES_RF_QSC1100=yes USES_ARM_SBC_CODEC=yes USES_LOW=yes USES_QHSUSB=yes USES_SERVICES_ERR=yes  ASIC=q1100 BUILD=SFNAALZ VERSION=3350 BUILDDIR=SXNAALZ 2>&1 | ..\..\tools\build\qtee buildSFNAALZ.log