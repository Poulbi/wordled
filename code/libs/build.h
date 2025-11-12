//~ Libraries
// Standard library (TODO(luca): get rid of it) 
#include <stdio.h>
#include <string.h>
// POSIX
#include <unistd.h>
// Linux
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
// External
#include <lr/lr_types.h>

//~ Macros
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Assert(Expression) if(!(Expression)) { __asm__ volatile("int3"); } 
#define MemoryCopy memcpy

//~ Types
struct str8
{
    umm Size;
    u8 *Data;
};
#define S8Lit(String) (str8){.Size = (sizeof((String)) - 1), .Data = (u8 *)(String)}

struct str8_list
{
    str8 *Strings;
    umm Count;
    umm Capacity;
};

//~ Global variables
global_variable u8 OutputBuffer[Kilobytes(64)] = {};

//~ Strings
umm CountCString(char *String)
{
    umm Result = 0;
    
    while(*String++) Result++;
    
    return Result;
}

b32 IsWhiteSpace(u8 Char)
{
    b32 Result = (Char == ' ' || Char == '\t' || Char == '\n');
    return Result;
}


void Str8ListAppend(str8_list *List, str8 String)
{
    Assert(List->Count < List->Capacity);
    List->Strings[List->Count++] = String;
}

#define Str8ListAppendMultiple(List, ...) \
do \
{ \
str8 Strings[] = {__VA_ARGS__}; \
_Str8ListAppendMultiple(List, ArrayCount(Strings), Strings); \
} while(0);
void _Str8ListAppendMultiple(str8_list *List, umm Count, str8 *Strings)
{
    for(u32 At = 0;
        At < Count;
        At++)
    {
        Str8ListAppend(List, Strings[At]);
    }
}

str8 Str8ListJoin(str8_list List, umm BufferSize, u8 *Buffer, u8 Char)
{
    str8 Result = {};
    
    umm BufferIndex = 0;
    for(umm At = 0;
        At < List.Count;
        At++)
    {
        str8 *StringAt = List.Strings + At;
        
        Assert(BufferIndex + StringAt->Size < BufferSize);
        MemoryCopy(Buffer + BufferIndex, StringAt->Data, StringAt->Size);
        BufferIndex += StringAt->Size;
        if(Char)
        {
            Buffer[BufferIndex++] = Char;
        }
    }
    
    Result.Data = Buffer;
    Result.Size = BufferIndex;
    
    return Result;
}

//~ Linux
struct linux_command_result
{
    b32 Error;
    
    umm StdoutBytesToRead;
    b32 StdinBytesToWrite;
    umm StderrBytesToRead;
    int Stdout;
    int Stdin;
    int Stderr;
};

//- Error wrappers 
void LinuxErrorWrapperPipe(int Pipe[2])
{
    int Ret = pipe(Pipe);
    if(Ret == -1)
    {
        perror("pipe");
        Assert(0);
    }
}

void LinuxErrorWrapperDup2(int OldFile, int NewFile)
{
    int Ret = dup2(OldFile, NewFile);
    if(Ret == -1)
    {
        perror("dup2");
        Assert(0);
    }
}

int LinuxErrorWrapperGetAvailableBytesToRead(int File)
{
    int BytesToRead = 0;
    
    int Ret = ioctl(File, FIONREAD, &BytesToRead);
    if(Ret == -1)
    {
        perror("read(ioctl)");
        Assert(0);
    }
    
    return BytesToRead;
}

smm LinuxErrorWrapperRead(int File, void *Buffer, umm BytesToRead)
{
    smm BytesRead = read(File, Buffer, BytesToRead);
    if(BytesRead == -1)
    {
        perror("read");
        Assert(0);
    }
    Assert(BytesRead == BytesRead);
    
    return BytesRead;
}

//- 
str8 LinuxFindCommandInPATH(umm BufferSize, u8 *Buffer, char *Command, char *Env[])
{
    char **VarAt = Env;
    char Search[] = "PATH=";
    int MatchedSearch = false;
    
    str8 Result = {};
    Result.Data = Buffer;
    
    while(*VarAt && !MatchedSearch)
    {
        MatchedSearch = true;
        
        for(unsigned int At = 0;
            (At < sizeof(Search) - 1) && (VarAt[At]);
            At++)
        {
            if(Search[At] != VarAt[0][At])
            {
                MatchedSearch = false;
                break;
            }
        }
        
        VarAt++;
    }
    
    if(MatchedSearch)
    {
        VarAt--;
        char *Scan = VarAt[0];
        while(*Scan && *Scan != '=') Scan++;
        Scan++;
        
        while((*Scan) && (Scan != VarAt[1]))
        {
            int Len = 0;
            while(Scan[Len] && Scan[Len] != ':' && 
                  (Scan+Len != VarAt[1])) Len++;
            
            // Add the PATH entry
            int At;
            for(At = 0; At < Len; At++)
            {
                Result.Data[At] = Scan[At];
            }
            Result.Data[At++] = '/';
            
            // Add the executable name
            for(char *CharAt = Command;
                *CharAt;
                CharAt++)
            {
                Result.Data[At++] = *CharAt;
            }
            Result.Data[At] = 0;
            
            // Check if it exists
            int AccessMode = F_OK | X_OK;
            int Ret = access((char *)Result.Data, AccessMode);
            if(Ret == 0)
            {
                Result.Size = At;
                break;
            }
            
            Scan += Len + 1;
        }
    }
    
    return Result;
}

linux_command_result LinuxRunCommand(char *Args[], b32 Pipe)
{
    linux_command_result Result = {};
    
    int StdoutPipe[2] = {};
    int StdinPipe[2] = {};
    int StderrPipe[2] = {};
    
    int Ret = 0;
    int WaitStatus = 0;
    
    if(Pipe)
    {    
        LinuxErrorWrapperPipe(StdoutPipe);
        LinuxErrorWrapperPipe(StdinPipe);
        LinuxErrorWrapperPipe(StderrPipe);
        Result.Stdout = StdoutPipe[0];
        Result.Stdin = StdinPipe[1];
        Result.Stderr = StderrPipe[0];
    }
    
    pid_t ChildPID = fork();
    if(ChildPID != -1)
    {
        if(ChildPID == 0)
        {
            if(Pipe)
            {            
                LinuxErrorWrapperDup2(StdoutPipe[1], STDOUT_FILENO);
                LinuxErrorWrapperDup2(StdinPipe[0], STDIN_FILENO);
                LinuxErrorWrapperDup2(StderrPipe[1], STDERR_FILENO);
            }
            
            if(execve(Args[0], Args, __environ) == -1)
            {
                perror("exec");
            }
        }
        else
        {
            wait(&WaitStatus);
        }
    }
    else
    {
        perror("fork");
    }
    
    b32 Exited = WIFEXITED(WaitStatus);
    if(Exited)
    {
        s8 ExitStatus = WEXITSTATUS(WaitStatus);
        if(ExitStatus)
        {
            Result.Error = true;
        }
    }
    
    if(Pipe)
    {    
        Result.StdoutBytesToRead = LinuxErrorWrapperGetAvailableBytesToRead(Result.Stdout);
        Result.StderrBytesToRead = LinuxErrorWrapperGetAvailableBytesToRead(Result.Stderr);
    }
    
    return Result;
}

linux_command_result LinuxRunCommandString(str8 Command, char *Env[], b32 Pipe)
{
    linux_command_result Result = {};
    
    char *Args[64] = {};
    
    u8 ArgsBuffer[1024] = {};
    umm ArgsBufferIndex = 0;
    
    // 1. split on whitespace into null-terminated strings.
    //    TODO: skip quotes
    u32 ArgsCount = 0;
    umm Start = 0;
    for(umm At = 0;
        At <= Command.Size;
        At++)
    {
        if(IsWhiteSpace(Command.Data[At]) || At == Command.Size)
        {
            Args[ArgsCount++] = (char *)(ArgsBuffer + ArgsBufferIndex);
            Assert(ArgsCount < ArrayCount(Args));
            umm Size = At - Start;
            MemoryCopy(ArgsBuffer + ArgsBufferIndex, Command.Data + Start, Size);
            ArgsBufferIndex += Size;
            ArgsBuffer[ArgsBufferIndex++] = 0;
            Assert(ArgsBufferIndex < ArrayCount(ArgsBuffer));
            
            while(IsWhiteSpace(Command.Data[At])) At++;
            
            Start = At;
        }
    }
    
    if(Args[0] && Args[0][0] != '/')
    {
        u8 Buffer[PATH_MAX] = {};
        str8 ExePath = LinuxFindCommandInPATH(sizeof(Buffer), Buffer, Args[0], Env);
        if(ExePath.Size)
        {
            Args[0] = (char *)ExePath.Data;
        }
    }
    
    Result = LinuxRunCommand(Args, Pipe);
    
    return Result;
}

internal void
LinuxChangeDirectory(char *Path)
{
    if(chdir(Path) == -1)
    {
        perror("chdir");
    }
}

internal void
LinuxChangeToExecutableDirectory(char *Args[])
{
    char *ExePath = Args[0];
    s32 Length = (s32)CountCString(ExePath);
    char ExecutableDirPath[PATH_MAX] = {};
    s32 LastSlash = 0;
    for(s32 At = 0;
        At < Length;
        At++)
    {
        if(ExePath[At] == '/')
        {
            LastSlash = At;
        }
    }
    MemoryCopy(ExecutableDirPath, ExePath, LastSlash);
    ExecutableDirPath[LastSlash] = 0;
    
    LinuxChangeDirectory(ExecutableDirPath);
}

//~ Helpers
str8_list CommonBuildCommand(b32 GCC, b32 Clang, b32 Debug)
{
    str8_list BuildCommand = {};
    
    // Exclusive arguments
    if(GCC) Clang = false;
    b32 Release = !Debug;
    
    u8 StringsBuffer[Kilobytes(4)] = {};
    BuildCommand.Strings = (str8 *)StringsBuffer;
    BuildCommand.Capacity = ArrayCount(StringsBuffer)/sizeof(str8);
    
    str8 CommonCompilerFlags = S8Lit("-DOS_LINUX=1 -fsanitize-trap -nostdinc++");
    str8 CommonWarningFlags = S8Lit("-Wall -Wextra -Wconversion -Wdouble-promotion -Wno-sign-conversion -Wno-sign-compare -Wno-double-promotion -Wno-unused-but-set-variable -Wno-unused-variable -Wno-write-strings -Wno-pointer-arith -Wno-unused-parameter -Wno-unused-function");
    
    str8 LinuxLinkerFlags = S8Lit("-lpthread -lm");
    
    str8 Compiler = {};
    str8 Mode = {};
    
    if(0) {}
    else if(Release)
    {
        Mode = S8Lit("release");
    }
    else if(Debug)
    {
        Mode = S8Lit("debug");
    }
    
    if(0) {}
    else if(Clang)
    {
        Compiler = S8Lit("clang");
        Str8ListAppend(&BuildCommand, Compiler);
        if(Debug)
        {
            Str8ListAppend(&BuildCommand, S8Lit("-g -ggdb -g3"));
        }
        else if(Release)
        {
            Str8ListAppend(&BuildCommand, S8Lit("-O3"));
        }
        Str8ListAppend(&BuildCommand, CommonCompilerFlags);
        Str8ListAppend(&BuildCommand, S8Lit("-fdiagnostics-absolute-paths -ftime-trace"));
        Str8ListAppend(&BuildCommand, CommonWarningFlags);
        Str8ListAppend(&BuildCommand, S8Lit("-Wno-null-dereference -Wno-missing-braces -Wno-vla-cxx-extension -Wno-writable-strings -Wno-missing-designated-field-initializers -Wno-address-of-temporary -Wno-int-to-void-pointer-cast"));
    }
    else if(GCC)
    {
        Compiler = S8Lit("g++");
        Str8ListAppend(&BuildCommand, Compiler);
        
        if(Debug)
        {
            Str8ListAppend(&BuildCommand, S8Lit("-g -ggdb -g3"));
        }
        else if(Release)
        {
            Str8ListAppend(&BuildCommand, S8Lit("-O3"));
        }
        Str8ListAppend(&BuildCommand, CommonCompilerFlags);
        Str8ListAppend(&BuildCommand, CommonWarningFlags);
        Str8ListAppend(&BuildCommand, S8Lit("-Wno-cast-function-type -Wno-missing-field-initializers -Wno-int-to-pointer-cast"));
    }
    
    Str8ListAppend(&BuildCommand, LinuxLinkerFlags);
    
    printf("%*s mode\n", (int)Mode.Size, Mode.Data);
    printf("%*s compile\n", (int)Compiler.Size, Compiler.Data);
    
    return BuildCommand;
}

internal void
LinuxRebuildSelf(int ArgsCount, char *Args[], char *Env[])
{
    b32 Rebuild = true;
    b32 ForceRebuild = false;
    for(int ArgsIndex = 1;
        ArgsIndex < ArgsCount;
        ArgsIndex++)
    {
        if(!strcmp(Args[ArgsIndex], "norebuild"))
        {
            Rebuild = false;
        }
        if(!strcmp(Args[ArgsIndex], "rebuild"))
        {
            ForceRebuild = true;
        }
    }
    
    if(ForceRebuild || Rebuild)
    {
        printf("[self compile]\n");
        str8_list BuildCommandList = CommonBuildCommand(false, true, true);
        Str8ListAppend(&BuildCommandList, S8Lit("-o build ../code/build.cpp"));
        str8 BuildCommand = Str8ListJoin(BuildCommandList, sizeof(OutputBuffer), OutputBuffer, ' ');
        
        //printf("%*s\n", (int)BuildCommand.Size, BuildCommand.Data);
        
        linux_command_result CommandResult = LinuxRunCommandString(BuildCommand, Env, true);
        umm BytesRead = LinuxErrorWrapperRead(CommandResult.Stderr, OutputBuffer, CommandResult.StderrBytesToRead);
        if(BytesRead)
        {
            printf("%*s\n", (int)BytesRead, OutputBuffer);
        }
        
        // Run without rebuilding
        char *Arguments[64] = {};
        u32 At;
        for(At = 1;
            At < ArgsCount;
            At++)
        {
            if(strcmp(Args[At], "rebuild"))
            {
                Arguments[At] = Args[At];
            }
        }
        Arguments[0] = "./build";
        // NOTE(luca): We changed to the Executable's build path
        Arguments[At++] = "norebuild";
        Assert(At < ArrayCount(Arguments));
        
        LinuxRunCommand(Arguments, false);
        
        _exit(0);
    }
}
