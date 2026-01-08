#include "libs/build.h"

void Build(char *Env[], char *Name, str8 Flags)
{
    
    printf("%s\n", Name);
    str8_list BuildCommandList = CommonBuildCommand(false, true ,true);
    Str8ListAppend(&BuildCommandList, Flags);
    str8 BuildCommand = Str8ListJoin(BuildCommandList, sizeof(OutputBuffer), OutputBuffer, ' ');
    
    linux_command_result CommandResult = LinuxRunCommandString(BuildCommand, Env, true);
    smm BytesToRead = LinuxErrorWrapperRead(CommandResult.Stderr, OutputBuffer, CommandResult.StderrBytesToRead);
    if(BytesToRead)
    {
        printf("%*s", (int)BytesToRead, OutputBuffer);
    }
    
}


int main(int ArgsCount, char *Args[], char *Env[])
{
    LinuxChangeToExecutableDirectory(Args);
    LinuxRebuildSelf(ArgsCount, Args, Env);
    
    Build(Env, "[linux handmade compile]",
						    S8Lit("-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_SMALL_RESOLUTION=1 "
                "-lX11 -lXfixes -lasound "
                "-o linux_handmade "
                "../code/libs/hm_linux/linux_handmade.cpp"));
    
    Build(Env, "[handmade compile]", 
          S8Lit("-shared -fPIC "
                "-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1"
                "-Wno-implicit-int-float-conversion -Wno-float-conversion -Wno-implicit-float-conversion -Wno-shorten-64-to-32 "
                "-o handmade.so "
                "../code/handmade.cpp"));
    
    return 0;
}
