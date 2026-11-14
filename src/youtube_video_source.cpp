#include "youtube_video_source.h"
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
namespace Reconstruction {
void YoutubeSource::attachJob(){
#ifdef Q_OS_WIN
    closeJob();job_=CreateJobObjectW(nullptr,nullptr);if(!job_)return;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof(limits));
    auto handle=OpenProcess(PROCESS_SET_QUOTA|PROCESS_TERMINATE,FALSE,DWORD(process_->processId()));
    if(!handle||!AssignProcessToJobObject(job_,handle)){if(handle)CloseHandle(handle);closeJob();return;}CloseHandle(handle);
#endif
}
void YoutubeSource::closeJob(){
#ifdef Q_OS_WIN
    if(job_){CloseHandle(job_);job_=nullptr;}
#endif
}
void YoutubeSource::terminateJob(){
#ifdef Q_OS_WIN
    if(job_)TerminateJobObject(job_,1);
#endif
}
}
