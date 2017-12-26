#include <debugScreen.h> // memset()
#include <psp2/appmgr.h> // sceAppMgrLaunchAppByUri()
#include <psp2/io/fcntl.h> // sceIo*()
#include <psp2/registrymgr.h> // sceRegMgrGetKeyBin()
#include <psp2/kernel/processmgr.h> // sceKernelExitProcess()

int main(int argc, char *argv[])
{
	SceUID fd;
	char key_buf[2048];

	memset(key_buf, 0, sizeof(key_buf));

	sceRegMgrGetKeyBin("/CONFIG/NP/", "account_id", key_buf, sizeof(key_buf));

	fd = sceIoOpen("savedata0:sce_sys/param.sfo", SCE_O_RDWR, 0777);
	if(fd >= 0)
	{
		sceIoLseek(fd, 0xE4, SCE_SEEK_SET); // ACCOUNT_ID
		sceIoWrite(fd, key_buf, 8);
	}
	sceIoClose(fd);

	sceKernelDelayThread(250000);
	sceAppMgrLaunchAppByUri(0xFFFFF, "psgm:play?titleid=TROPHYFIX");
	sceKernelDelayThread(250000);
	sceKernelExitProcess(0);

	return 0;
}
