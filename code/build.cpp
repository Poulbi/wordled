#include "libs/build.h"

void Build(char *Name, str8 Flags)
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
    
    Build("[linux handmade compile]", S8Lit("-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_SMALL_RESOLUTION=1 "
                                            "-lX11 -lXfixes -lasound "
                                            "-o linux_handmade "
                                            "../code/libs/hm_linux/linux_handmade.cpp"));
    
    Build("[handmade compile]", 
          S8Lit("-shared -fPIC "
                "-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1"
                "-Wno-conversion "
                "-o handmade.so "
                "../code/handmade.cpp"));
    
    return 0;
}
