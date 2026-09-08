#include "pc_speech_input.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <stdio.h>
#include <string.h>

// No SAPI/COM service: SDL captures audio, the bundled offline CPU recognizer
// receives a WAV through a private pipe. No recordings or transcripts on disk.
static SRWLOCK lock = SRWLOCK_INIT;
static HANDLE worker, cancelEvent;
static volatile LONG finishRequested;
static bool listening, ready;
static char result[4096], error[256];
static std::wstring speechDirectory;
static const size_t maxSamples = 16000 * 30;

// Leave scheduler capacity for the game, compositor and streaming software.
static unsigned speechThreadBudget(unsigned processors) {
    return std::max(1u, std::min(6u, processors / 2));
}

static bool cancelled() { return WaitForSingleObject(cancelEvent, 0) == WAIT_OBJECT_0; }
static void closeHandle(HANDLE &h) { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); h = NULL; }
static std::vector<unsigned char> makeWav(const std::vector<short>& pcm) {
    std::vector<unsigned char> wav(44 + pcm.size()*2);
    memcpy(wav.data(), "RIFF", 4); memcpy(wav.data()+8, "WAVEfmt ", 8);
    auto u32 = [&](size_t at, unsigned v) { for(int i=0;i<4;i++) wav[at+i]=(v>>(8*i))&255; };
    auto u16 = [&](size_t at, unsigned v) { wav[at]=v&255; wav[at+1]=(v>>8)&255; };
    u32(4, (unsigned)wav.size()-8); u32(16,16); u16(20,1); u16(22,1);
    u32(24,16000); u32(28,32000); u16(32,2); u16(34,16);
    memcpy(wav.data()+36,"data",4); u32(40,(unsigned)pcm.size()*2);
    memcpy(wav.data()+44,pcm.data(),pcm.size()*2);
    return wav;
}

static bool transcribe(const std::vector<short>& pcm, std::string &text, std::string &failure, bool generic = false) {
    HANDLE inRead=NULL, inWrite=NULL, outRead=NULL, outWrite=NULL, nullError=NULL;
    HANDLE job=NULL;
    PROCESS_INFORMATION process={};
    SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};
    bool success=false;
    do {
        if (!CreatePipe(&inRead,&inWrite,&sa,1024*1024) || !CreatePipe(&outRead,&outWrite,&sa,16384)) break;
        SetHandleInformation(inWrite,HANDLE_FLAG_INHERIT,0);
        SetHandleInformation(outRead,HANDLE_FLAG_INHERIT,0);
        nullError=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
        if (nullError==INVALID_HANDLE_VALUE) break;
        STARTUPINFOW startup={}; startup.cb=sizeof(startup);
        startup.dwFlags=STARTF_USESTDHANDLES;
        startup.hStdInput=inRead; startup.hStdOutput=outWrite; startup.hStdError=nullError;
        std::wstring exe=speechDirectory+L"whisper-cli.exe";
        bool accelerated=false;
        __builtin_cpu_init();
        if(!generic && __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma") && __builtin_cpu_supports("f16c") &&
                GetFileAttributesW((speechDirectory+L"whisper-cli-avx2.exe").c_str())!=INVALID_FILE_ATTRIBUTES) {
            exe=speechDirectory+L"whisper-cli-avx2.exe";
            accelerated=true;
        }
        // With stdin '-', CLI defaults output to '-' and disables its live
        // callback. A named output stem restores stdout; without -otxt/etc it
        // creates no file. Relative model path also avoids ANSI path conversion.
        SYSTEM_INFO systemInfo={}; GetSystemInfo(&systemInfo);
        const unsigned threads=speechThreadBudget(systemInfo.dwNumberOfProcessors);
        std::wstring args=L"\""+exe+L"\" -m ggml-small-q5_1.bin -f - -of transcript -l auto -t "
            +std::to_wstring(threads)+L" -ng -np -nt -sns";
        // CPU only, bounded low-priority threads: no GPU context or driver changes.
        if (!CreateProcessW(exe.c_str(),&args[0],NULL,NULL,TRUE,
                CREATE_NO_WINDOW|CREATE_SUSPENDED|BELOW_NORMAL_PRIORITY_CLASS,NULL,speechDirectory.c_str(),&startup,&process)) break;
        job=CreateJobObjectW(NULL,NULL);
        if (job) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={};
            limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if (!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)) ||
                !AssignProcessToJobObject(job,process.hProcess)) closeHandle(job);
        }
        ResumeThread(process.hThread);
        closeHandle(inRead); closeHandle(outWrite); closeHandle(nullError);
        std::vector<unsigned char> wav=makeWav(pcm);
        DWORD written=0;
        bool sent=WriteFile(inWrite,wav.data(),(DWORD)wav.size(),&written,NULL) && written==wav.size();
        closeHandle(inWrite);
        if (!sent) break;
        const ULONGLONG deadline=GetTickCount64()+(accelerated ? 120000 : 600000);
        bool ended=false;
        while (!cancelled() && GetTickCount64()<deadline) {
            DWORD available=0;
            if (PeekNamedPipe(outRead,NULL,0,NULL,&available,NULL) && available) {
                char buffer[1024]; DWORD count=0;
                if (ReadFile(outRead,buffer,std::min<DWORD>(available,sizeof(buffer)),&count,NULL)) {
                    if(text.size()+count>16384) { failure="Dictation returned too much text. Try a shorter phrase."; break; }
                    text.append(buffer,count);
                }
                continue;
            }
            // Once the child exits, check the pipe again before finishing:
            // its last write may have landed between PeekNamedPipe and wait.
            if (ended) break;
            if (WaitForSingleObject(process.hProcess,25)==WAIT_OBJECT_0) ended=true;
        }
        DWORD code=1;
        if (ended && GetExitCodeProcess(process.hProcess,&code) && code==0) success=true;
        else if (!cancelled() && failure.empty()) failure="Offline dictation failed or timed out. Check the speech folder and try a shorter phrase.";
    } while(false);
    if(process.hProcess && WaitForSingleObject(process.hProcess,0)!=WAIT_OBJECT_0) {
        // Only our own per-dictation helper, never another application's process.
        TerminateProcess(process.hProcess,1); WaitForSingleObject(process.hProcess,2000);
    }
    closeHandle(process.hThread); closeHandle(process.hProcess); closeHandle(job);
    closeHandle(inRead); closeHandle(inWrite); closeHandle(outRead); closeHandle(outWrite); closeHandle(nullError);
    if(!success && !cancelled() && failure.empty()) failure="Could not run the bundled offline dictation engine. Re-extract the full build.";
    // CLI emits newlines around segments. Preserve UTF-8, normalize whitespace.
    std::string cleaned;
    for(unsigned char c:text) {
        if(c<=32) { if(!cleaned.empty() && cleaned.back()!=' ') cleaned+=' '; }
        else cleaned+=(char)c;
    }
    while(!cleaned.empty() && cleaned.back()==' ') cleaned.pop_back();
    text.swap(cleaned);
    return success;
}

static DWORD WINAPI recognize(void*) {
    std::string text, failure;
    std::vector<short> pcm; pcm.reserve(maxSamples);
    bool initAudio=(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)==0;
    SDL_AudioDeviceID device=0;
    if(initAudio && SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) failure="Could not initialize microphone capture.";
    if(failure.empty() && !cancelled()) {
        SDL_AudioSpec wanted={}; wanted.freq=16000; wanted.format=AUDIO_S16SYS; wanted.channels=1; wanted.samples=512;
        device=SDL_OpenAudioDevice(NULL,1,&wanted,NULL,0);
        if(!device) failure="Could not open the default microphone. Check recording-device selection and microphone permissions.";
    }
    bool heardVoice=false;
    if(device) {
        SDL_PauseAudioDevice(device,0);
        const ULONGLONG started=GetTickCount64();
        ULONGLONG lastVoice=started;
        while(!cancelled() && pcm.size()<maxSamples && GetTickCount64()-started<30000) {
            short chunk[1600];
            unsigned count=SDL_DequeueAudio(device,chunk,sizeof(chunk))/sizeof(short);
            double energy=0;
            for(unsigned i=0;i<count;i++) energy+=(double)chunk[i]*chunk[i];
            if(count && sqrt(energy/count)>180) { heardVoice=true; lastVoice=GetTickCount64(); }
            size_t keep=std::min<size_t>(count,maxSamples-pcm.size());
            pcm.insert(pcm.end(),chunk,chunk+keep);
            if(InterlockedCompareExchange(&finishRequested,0,0)) break;
            if(heardVoice && GetTickCount64()-lastVoice>1800) break;
            if(!heardVoice && GetTickCount64()-started>10000) break;
            SDL_Delay(20);
        }
        SDL_CloseAudioDevice(device);
    }
    if(initAudio) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    if(!cancelled() && failure.empty()) {
        if(!heardVoice || pcm.size()<3200) failure="No speech detected. Check the default microphone and try again.";
        else transcribe(pcm,text,failure);
    }
    AcquireSRWLockExclusive(&lock);
    if(!cancelled()) {
        if(!failure.empty()) snprintf(error,sizeof(error),"%s",failure.c_str());
        else if(text.empty()) snprintf(error,sizeof(error),"No speech recognized. Try again closer to the microphone.");
        else { size_t n=std::min(text.size(),sizeof(result)-1); while(n && n<text.size() && ((unsigned char)text[n]&0xc0)==0x80) --n;
            memcpy(result,text.data(),n); result[n]=0; ready=true; }
    }
    listening=false;
    ReleaseSRWLockExclusive(&lock);
    return 0;
}

extern "C" bool pc_speech_recognition_start(void) {
    if(worker) {
        if(WaitForSingleObject(worker,0)!=WAIT_OBJECT_0) return false;
        closeHandle(worker); closeHandle(cancelEvent);
    }
    AcquireSRWLockExclusive(&lock);
    ready=false; result[0]=error[0]=0; InterlockedExchange(&finishRequested,0);
    WCHAR path[32768]; DWORD length=GetModuleFileNameW(NULL,path,32768);
    if(!length || length>=32768) { snprintf(error,sizeof(error),"Could not locate the speech folder."); ReleaseSRWLockExclusive(&lock); return false; }
    speechDirectory=path; speechDirectory.resize(speechDirectory.find_last_of(L"\\/")+1); speechDirectory+=L"speech\\";
    if(GetFileAttributesW((speechDirectory+L"whisper-cli.exe").c_str())==INVALID_FILE_ATTRIBUTES ||
       GetFileAttributesW((speechDirectory+L"ggml-small-q5_1.bin").c_str())==INVALID_FILE_ATTRIBUTES) {
        snprintf(error,sizeof(error),"Offline dictation files are missing. Extract the complete build, including the speech folder.");
        ReleaseSRWLockExclusive(&lock); return false;
    }
    cancelEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
    listening=cancelEvent!=NULL;
    if(listening) worker=CreateThread(NULL,0,recognize,NULL,0,NULL);
    if(!worker) { listening=false; closeHandle(cancelEvent); snprintf(error,sizeof(error),"Could not start offline dictation."); }
    bool started=listening; ReleaseSRWLockExclusive(&lock); return started;
}
extern "C" void pc_speech_recognition_cancel(void) {
    AcquireSRWLockExclusive(&lock); if(cancelEvent) SetEvent(cancelEvent);
    listening=ready=false; result[0]=error[0]=0; ReleaseSRWLockExclusive(&lock);
}
extern "C" void pc_speech_recognition_stop(void) { InterlockedExchange(&finishRequested,1); }
extern "C" bool pc_speech_recognition_poll(char* text,size_t size) {
    if(!text || !size) return false;
    AcquireSRWLockExclusive(&lock); bool available=ready;
    if(available) { size_t n=strlen(result); if(n>=size) { n=size-1; while(n && ((unsigned char)result[n]&0xc0)==0x80) --n; }
        memcpy(text,result,n); text[n]=0; ready=false; }
    ReleaseSRWLockExclusive(&lock); return available;
}
extern "C" bool pc_speech_recognition_is_listening(void) {
    AcquireSRWLockShared(&lock); bool active=listening; ReleaseSRWLockShared(&lock); return active;
}
extern "C" bool pc_speech_recognition_error(char* text,size_t size) {
    if(!text || !size) return false;
    AcquireSRWLockExclusive(&lock); bool available=error[0]!=0;
    if(available) { snprintf(text,size,"%s",error); error[0]=0; } ReleaseSRWLockExclusive(&lock); return available;
}
#else
extern "C" bool pc_speech_recognition_start(void) { return false; }
extern "C" void pc_speech_recognition_cancel(void) {}
extern "C" void pc_speech_recognition_stop(void) {}
extern "C" bool pc_speech_recognition_poll(char*,size_t) { return false; }
extern "C" bool pc_speech_recognition_is_listening(void) { return false; }
extern "C" bool pc_speech_recognition_error(char*,size_t) { return false; }
#endif
