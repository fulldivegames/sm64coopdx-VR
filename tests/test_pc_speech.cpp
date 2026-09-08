#define SDL_MAIN_HANDLED
#include "../src/pc/pc_speech_input.cpp"
#include <assert.h>
#undef assert
#define assert(test) do { if(!(test)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#test); fflush(stderr); ExitProcess(1); } } while(0)
static DWORD WINAPI cancelSoon(void*) { Sleep(200); SetEvent(cancelEvent); return 0; }
int main(int argc,char **argv) {
    assert(argc==2 || argc==3);
    assert(speechThreadBudget(0)==1 && speechThreadBudget(1)==1);
    assert(speechThreadBudget(4)==2 && speechThreadBudget(8)==4);
    assert(speechThreadBudget(12)==6 && speechThreadBudget(64)==6);
    SDL_SetMainReady();
    char text[256];
    assert(!pc_speech_recognition_poll(text,sizeof(text)));
    strcpy(result,"hello"); ready=listening=true;
    pc_speech_recognition_stop();
    assert(pc_speech_recognition_is_listening());
    assert(pc_speech_recognition_poll(text,sizeof(text)) && !strcmp(text,"hello"));
    strcpy(result,"a\xc3\xa9z"); ready=true;
    assert(pc_speech_recognition_poll(text,3) && !strcmp(text,"a"));
    listening=false;
    // Test executable lives next to the game's speech folder.
    WCHAR path[32768]; GetModuleFileNameW(NULL,path,32768);
    speechDirectory=path; speechDirectory.resize(speechDirectory.find_last_of(L"\\/")+1); speechDirectory+=L"speech\\";
    SDL_AudioSpec spec; Uint8 *bytes=NULL; Uint32 size=0;
    assert(SDL_LoadWAV(argv[1],&spec,&bytes,&size));
    assert(spec.freq==16000 && spec.format==AUDIO_S16LSB && spec.channels==1);
    std::vector<short> pcm((short*)bytes,(short*)(bytes+size)); SDL_FreeWAV(bytes);
    cancelEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
    std::string transcript,failure;
    const ULONGLONG started=GetTickCount64();
    assert(transcribe(pcm,transcript,failure));
    fprintf(stdout,"Recognition (%llu ms): %s\n",GetTickCount64()-started,transcript.c_str());
    assert(transcript.find("country")!=std::string::npos && transcript.find("ask not")!=std::string::npos);
    if(argc==2) {
        transcript.clear(); failure.clear();
        assert(transcribe(pcm,transcript,failure,true));
        assert(transcript.find("country")!=std::string::npos && transcript.find("ask not")!=std::string::npos);
        puts("PASS: generic CPU fallback transcription");
    }
    // Cancelling during model loading/inference must discard output and exit.
    HANDLE stopper=CreateThread(NULL,0,cancelSoon,NULL,0,NULL);
    transcript.clear(); failure.clear();
    assert(!transcribe(pcm,transcript,failure));
    WaitForSingleObject(stopper,2000); closeHandle(stopper); closeHandle(cancelEvent);
    // Exercise actual capture/start-stop state, stopping before any utterance.
    assert(pc_speech_recognition_start());
    pc_speech_recognition_stop();
    assert(WaitForSingleObject(cancelEvent,0)==WAIT_TIMEOUT);
    assert(WaitForSingleObject(worker,15000)==WAIT_OBJECT_0);
    assert(!pc_speech_recognition_is_listening());
    assert(!pc_speech_recognition_poll(text,sizeof(text)));
    closeHandle(worker); closeHandle(cancelEvent);
    puts("PASS: offline transcription, UTF-8, pending-result preservation, cancel, mic start/finish");
    return 0;
}
