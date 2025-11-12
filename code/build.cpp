#include "libs/build.h"

int main(int ArgsCount, char *Args[], char *Env[])
{
    LinuxChangeToExecutableDirectory(Args);
    LinuxRebuildSelf(ArgsCount, Args, Env);
    
    printf("[linux handmade compile]\n");
    str8_list BuildCommandList = CommonBuildCommand(false, true ,true);
    Str8ListAppend(&BuildCommandList, 
                   S8Lit("-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_SMALL_RESOLUTION=1 "
                         "-lX11 -lXfixes -lasound -lcurl "
                         "-o linux_handmade "
                         "../code/libs/hm_linux/linux_handmade.cpp"));
    str8 BuildCommand = Str8ListJoin(BuildCommandList, sizeof(OutputBuffer), OutputBuffer, ' ');
    
    linux_command_result CommandResult = LinuxRunCommandString(BuildCommand, Env, true);
    smm BytesToRead = LinuxErrorWrapperRead(CommandResult.Stderr, OutputBuffer, CommandResult.StderrBytesToRead);
    if(BytesToRead)
    {
        printf("%*s", (int)BytesToRead, OutputBuffer);
    }
    
    return 0;
}
