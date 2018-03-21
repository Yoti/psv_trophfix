#include <stdio.h>
#include <stdlib.h>
#include <debugScreen.h>
#include <psp2/ctrl.h>
#include <psp2/appmgr.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/registrymgr.h>
#include <psp2/kernel/processmgr.h>

#define printf psvDebugScreenPrintf
#define AUTORUN_PATH "savedata0:autorun.bin"
#define SAVEDATA_PATH "savedata0:sce_sys/param.sfo"
#define TROPHYFIX_PATH "app0:sce_sys/eboot.bin"

int is365(void)
{
	SceUID fd;
	fd = sceIoOpen("vs0:tai/henkaku.skprx", SCE_O_RDONLY, 0777);
	if (fd < 0)
	{
		return 0;
	}
	else
	{
		sceIoClose(fd);
		return 1;
	}
}

void ContinueByKey(int key)
{
	SceCtrlData ctrl;

	do
	{
		sceCtrlReadBufferPositive(0, &ctrl, 1);
		sceKernelDelayThread(100000);
	}
	while(!(ctrl.buttons & key)); // press

	do
	{
		sceCtrlReadBufferPositive(0, &ctrl, 1);
		sceKernelDelayThread(100000);
	}
	while((ctrl.buttons & key)); // release

	sceKernelDelayThread(100000);
}

int psvIoCopy(char*src, char*dst)
{
	int size;
	char*buf;
	SceUID fd;

	// open src
	fd = sceIoOpen(src, SCE_O_RDONLY, 0777);
	if (fd < 0)
		return fd;

	// size src
	size = sceIoLseek(fd, 0, SCE_SEEK_END);

	// fill buf
	buf = malloc(size);
	sceIoLseek(fd, 0, SCE_SEEK_SET);
	sceIoRead(fd, buf, size);

	// close src
	sceIoClose(fd);

	// open dst
	fd = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	if (fd < 0)
		return fd;

	// write dst
	sceIoWrite(fd, buf, size);

	// close dst
	sceIoClose(fd);

	return 0;
}

int CheckForEncryption(char title_buf[9])
{
	SceUID dfd;
	char path_buf[255];
	int ret = 0;

	memset(path_buf, 0, sizeof(path_buf));

	sprintf(path_buf, "ux0:app/%s/sce_pfs", title_buf);
	dfd = sceIoDopen(path_buf);
	if (dfd >= 0)
	{
		sceIoDclose(dfd);
		ret = 1;
	}

	sprintf(path_buf, "gro0:app/%s/sce_pfs", title_buf);
	dfd = sceIoDopen(path_buf);
	if (dfd >= 0)
	{
		sceIoDclose(dfd);
		ret = 1;
	}

	sprintf(path_buf, "ur0:app/%s/sce_pfs", title_buf);
	dfd = sceIoDopen(path_buf);
	if (dfd >= 0)
	{
		sceIoDclose(dfd);
		ret = 1;
	}

	return ret;
}

void ReplaceEboot(char title_buf[9])
{
	SceUID dfd;
	char path_buf[255];
	char dest_buf[255];

	memset(path_buf, 0, sizeof(path_buf));
	memset(dest_buf, 0, sizeof(dest_buf));

	sprintf(path_buf, "ux0:app/%s", title_buf);
	dfd = sceIoDopen(path_buf);
	if (dfd >= 0)
	{
		sceIoDclose(dfd);
		sprintf(path_buf, "ux0:app/%s/eboot.bin", title_buf);
		sprintf(dest_buf, "ux0:app/%s/eboot.bin_", title_buf);
		sceIoRename(path_buf, dest_buf);

		sprintf(dest_buf, "ux0:app/%s/eboot.bin", title_buf);
		psvIoCopy(TROPHYFIX_PATH, dest_buf);
	}
	else
	{
		sprintf(path_buf, "ur0:app/%s", title_buf);
		dfd = sceIoDopen(path_buf);
		if (dfd >= 0)
		{
			sceIoDclose(dfd);
			sprintf(path_buf, "ur0:app/%s/eboot.bin", title_buf);
			sprintf(dest_buf, "ur0:app/%s/eboot.bin_", title_buf);
			sceIoRename(path_buf, dest_buf);

			sprintf(dest_buf, "ux0:app/%s/eboot.bin", title_buf);
			psvIoCopy(TROPHYFIX_PATH, dest_buf);
		}
	}
}

void RestoreEboot(char title_buf[9])
{
	SceUID fd;
	char path_buf[255];
	char dest_buf[255];

	memset(path_buf, 0, sizeof(path_buf));
	memset(dest_buf, 0, sizeof(dest_buf));

	sprintf(path_buf, "ux0:app/%s/eboot.bin_", title_buf);
	fd = sceIoOpen(path_buf, SCE_O_RDONLY, 0777);
	if (fd >= 0)
	{
		sceIoClose(fd);
		sprintf(path_buf, "ux0:app/%s/eboot.bin", title_buf);
		sceIoRemove(path_buf);

		sprintf(path_buf, "ux0:app/%s/eboot.bin_", title_buf);
		sprintf(dest_buf, "ux0:app/%s/eboot.bin", title_buf);
		sceIoRename(path_buf, dest_buf);
	}
	else
	{
		sprintf(path_buf, "ur0:app/%s/eboot.bin_", title_buf);
		fd = sceIoOpen(path_buf, SCE_O_RDONLY, 0777);
		if (fd >= 0)
		{
			sceIoClose(fd);
			sprintf(path_buf, "ur0:app/%s/eboot.bin", title_buf);
			sceIoRemove(path_buf);

			sprintf(path_buf, "ur0:app/%s/eboot.bin_", title_buf);
			sprintf(dest_buf, "ur0:app/%s/eboot.bin", title_buf);
			sceIoRename(path_buf, dest_buf);
		}
	}
}

void BackupAndPatch(char title_buf[9])
{
	SceUID dfd;
	char path_buf[255];
	char dest_buf[255];

	memset(path_buf, 0, sizeof(path_buf));
	memset(dest_buf, 0, sizeof(dest_buf));

	if (is365)
		sprintf(path_buf, "ux0:repatch/%s", title_buf);
	else
		sprintf(path_buf, "ux0:patch/%s", title_buf);
	dfd = sceIoDopen(path_buf);
	if (dfd >= 0)
	{
		sceIoDclose(dfd);
		if (is365)
			sprintf(path_buf, "ux0:repatch/%s_", title_buf);
		else
			sprintf(path_buf, "ux0:patch/%s_", title_buf);
		sceIoRename(path_buf, dest_buf);
	}

	if (is365)
	{
		sceIoMkdir("ux0:repatch", 0777);
		sprintf(path_buf, "ux0:repatch/%s", title_buf);
		sceIoMkdir(path_buf, 0777);
		sprintf(path_buf, "ux0:repatch/%s/sce_sys", title_buf);
		sceIoMkdir(path_buf, 0777);
	}
	else
	{
		sceIoMkdir("ux0:patch", 0777);
		sprintf(path_buf, "ux0:patch/%s", title_buf);
		sceIoMkdir(path_buf, 0777);
		sprintf(path_buf, "ux0:patch/%s/sce_sys", title_buf);
		sceIoMkdir(path_buf, 0777);
	}

	if (is365)
		sprintf(dest_buf, "ux0:repatch/%s/eboot.bin", title_buf);
	else
		sprintf(dest_buf, "ux0:patch/%s/eboot.bin", title_buf);
	psvIoCopy(TROPHYFIX_PATH, dest_buf);

	sprintf(path_buf, "ux0:app/%s/sce_sys/param.sfo", title_buf);
	if (is365)
		sprintf(dest_buf, "ux0:repatch/%s/sce_sys/param.sfo", title_buf);
	else
		sprintf(dest_buf, "ux0:patch/%s/sce_sys/param.sfo", title_buf);
	if (psvIoCopy(path_buf, dest_buf) < 0)
	{
		sprintf(path_buf, "gro0:app/%s/sce_sys/param.sfo", title_buf);
		if (psvIoCopy(path_buf, dest_buf) < 0)
		{
			sprintf(path_buf, "ur0:app/%s/sce_sys/param.sfo", title_buf);
			psvIoCopy(path_buf, dest_buf);
		}
	}
}

void RemoveAndRestore(char title_buf[9])
{
	SceUID dfd;
	char path_buf[255];
	char dest_buf[255];

	memset(path_buf, 0, sizeof(path_buf));
	memset(dest_buf, 0, sizeof(dest_buf));

	if (is365)
	{
		sprintf(path_buf, "ux0:repatch/%s/sce_sys/param.sfo", title_buf);
		sceIoRemove(path_buf);
		sprintf(path_buf, "ux0:repatch/%s/eboot.bin", title_buf);
		sceIoRemove(path_buf);

		sprintf(path_buf, "ux0:repatch/%s/sce_sys", title_buf);
		sceIoRmdir(path_buf);
		sprintf(path_buf, "ux0:repatch/%s", title_buf);
		sceIoRmdir(path_buf);

		sprintf(path_buf, "ux0:repatch/%s_", title_buf);
	}
	else
	{
		sprintf(path_buf, "ux0:patch/%s/sce_sys/param.sfo", title_buf);
		sceIoRemove(path_buf);
		sprintf(path_buf, "ux0:patch/%s/eboot.bin", title_buf);
		sceIoRemove(path_buf);

		sprintf(path_buf, "ux0:patch/%s/sce_sys", title_buf);
		sceIoRmdir(path_buf);
		sprintf(path_buf, "ux0:patch/%s", title_buf);
		sceIoRmdir(path_buf);

		sprintf(path_buf, "ux0:patch/%s_", title_buf);
	}
	dfd = sceIoDopen(path_buf);
	if (dfd >= 0)
	{
		sceIoDclose(dfd);
		if (is365)
			sprintf(dest_buf, "ux0:repatch/%s", title_buf);
		else
			sprintf(dest_buf, "ux0:patch/%s", title_buf);
		sceIoRename(path_buf, dest_buf);
	}
}

void run()
{
	SceUID fd;
	SceUID dfd;
	SceIoDirent dir;
	uint64_t aid_key;
	uint64_t aid_sfo;
	char title_buf[15];
	char path_buf[255];
	char key_buf[2048];

	memset(title_buf, 0, sizeof(title_buf));
	memset(path_buf, 0, sizeof(path_buf));
	memset(key_buf, 0, sizeof(key_buf));

	sceRegMgrGetKeyBin("/CONFIG/NP/", "account_id", key_buf, sizeof(key_buf));
	memcpy(&aid_key, key_buf, 0x08); // key_buf больше не нужен

	dfd = sceIoDopen("ux0:user/00/savedata/");
	if (dfd >= 0)
	{
		while(sceIoDread(dfd, &dir) != 0)
		{
			if (SCE_S_ISDIR(dir.d_stat.st_mode)) // там не должно быть чего-то кроме папок
												 // может добавить проверку на длину имени
			{
				sprintf(path_buf, "ux0:user/00/savedata/%s/sce_sys/param.sfo", dir.d_name);
				fd = sceIoOpen(path_buf, SCE_O_RDONLY, 0777);
				if (fd >= 0)
				{
					printf("%s -> ", dir.d_name);

					sceIoLseek(fd, 0xE4, SCE_SEEK_SET); // ACCOUNT_ID
					sceIoRead(fd, title_buf, 0x08);
					memcpy(&aid_sfo, title_buf, 0x08);

					sceIoLseek(fd, 0x51C, SCE_SEEK_SET); // APPLICATION_ID
					sceIoRead(fd, title_buf, 0x09);
					printf("%s\n", title_buf);
					sceIoClose(fd);

					if (aid_sfo != 0)
					{
						if (aid_key != aid_sfo)
						{
							if (CheckForEncryption(title_buf) == 1)
							{
								BackupAndPatch(title_buf);

								fd = sceIoOpen(AUTORUN_PATH, SCE_O_WRONLY | SCE_O_CREAT, 0777);
								sceIoWrite(fd, title_buf, 0x09);
								sceIoClose(fd);

								sprintf(path_buf, "psgm:play?titleid=%s", title_buf);
								sceKernelDelayThread(250000);
								sceAppMgrLaunchAppByUri(0xFFFFF, path_buf);
								sceKernelDelayThread(250000);
								sceKernelExitProcess(0);
							}
							else
							{
								ReplaceEboot(title_buf);

								fd = sceIoOpen(AUTORUN_PATH, SCE_O_WRONLY | SCE_O_CREAT, 0777);
								sceIoWrite(fd, title_buf, 0x09);
								sceIoClose(fd);

								sprintf(path_buf, "psgm:play?titleid=%s", title_buf);
								sceKernelDelayThread(250000);
								sceAppMgrLaunchAppByUri(0xFFFFF, path_buf);
								sceKernelDelayThread(250000);
								sceKernelExitProcess(0);
							}
						}
					}
				}
			}
		}
		sceIoDclose(dfd);
		sceIoRemove(AUTORUN_PATH);

		printf("\nNothing to fix!\n");
		printf("Press O to exit...\n");
		ContinueByKey(SCE_CTRL_CIRCLE);
		sceKernelExitProcess(0);
	}
}

int main(int argc, char *argv[])
{
	int ret;
	SceUID fd;
	SceCtrlData ctrl;
	uint64_t aid_key;
	uint64_t aid_sfo;
	char title_buf[15];
	char key_buf[2048];

	psvDebugScreenInit();
	psvDebugScreenClear(0);

	printf("Trophies Fixer v1.1 by Yoti\n\n");

	memset(key_buf, 0, sizeof(key_buf));
	ret = sceRegMgrGetKeyBin("/CONFIG/NP/", "account_id", key_buf, sizeof(key_buf));
	if (ret < 0)
	{
		printf("You must enable unsafe homebrew first!\n");
		printf("Press O to exit...\n");
		ContinueByKey(SCE_CTRL_CIRCLE);
		sceKernelExitProcess(0);
	}

	fd = sceIoOpen(AUTORUN_PATH, SCE_O_RDONLY, 0777);
	if (fd >= 0)
	{
		printf("Autorun mode started!\n");
		memset(title_buf, 0, sizeof(title_buf));
		sceIoRead(fd, title_buf, 9);
		sceIoClose(fd);
		if (CheckForEncryption(title_buf) == 1)
			RemoveAndRestore(title_buf);
		else
			RestoreEboot(title_buf);
		run();
	}
	// else
	printf(" Now you can earn trophies in all applications.\n");
	printf(" Why you still not start this trophies fixer???\n\n");
	printf("Press X to fix or O to exit\n\n");

	fd = sceIoOpen(SAVEDATA_PATH, SCE_O_RDONLY, 0777);
	if (fd >= 0)
	{
		memcpy(&aid_key, key_buf, 0x08);
		sceIoLseek(fd, 0xE4, SCE_SEEK_SET);
		sceIoRead(fd, title_buf, 0x08);
		memcpy(&aid_sfo, title_buf, 0x08);
		sceIoClose(fd);
		if (aid_key != aid_sfo)
		{
			fd = sceIoOpen(SAVEDATA_PATH, SCE_O_WRONLY, 0777);
			if (fd >= 0)
			{
				sceIoLseek(fd, 0xE4, SCE_SEEK_SET);
				sceIoWrite(fd, &aid_key, 0x08);
				sceIoClose(fd);
			}
		}
	}

	for(;;)
	{
		sceCtrlPeekBufferPositive(0, &ctrl, 1);
		if (ctrl.buttons == SCE_CTRL_CROSS)
		{
			run();
			break;
		}
		else if (ctrl.buttons == SCE_CTRL_CIRCLE)
		{
			sceKernelExitProcess(0);
			break; // ???
		}
		else if (ctrl.buttons == SCE_CTRL_TRIANGLE)
		{
			sceKernelDelayThread(250000);
			sceAppMgrLaunchAppByUri(0xFFFFF, "pstc:");
			sceKernelDelayThread(250000);
			sceKernelExitProcess(0);
			break; // ???
		}
		sceKernelDelayThread(100000);
	}

	return 0;
}
