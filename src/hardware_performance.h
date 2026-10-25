#pragma once
#include "memory_budget.h"
#include <QThread>
#include <QThreadPool>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QtConcurrentRun>
#include <QSysInfo>
#include <vector>
#include <cmath>
#include <cstring>
#include <vulkan/vulkan.h>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace HardwarePerformance {
inline QJsonObject detect(){
    QJsonObject report{{"logicalProcessors",qMax(1,QThread::idealThreadCount())},{"architecture",QSysInfo::currentCpuArchitecture()}};
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);if(GlobalMemoryStatusEx(&memory)){report["ramBytes"]=QString::number(memory.ullTotalPhys);report["availableRamBytes"]=QString::number(memory.ullAvailPhys);}
#endif
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="9XD hardware detection";app.apiVersion=VK_API_VERSION_1_0;VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;VkInstance instance{};
    if(vkCreateInstance(&ci,nullptr,&instance)!=VK_SUCCESS)return report;
    uint32_t count=0;if(vkEnumeratePhysicalDevices(instance,&count,nullptr)==VK_SUCCESS&&count){std::vector<VkPhysicalDevice> devices(count);if(vkEnumeratePhysicalDevices(instance,&count,devices.data())==VK_SUCCESS){qint64 best=-1;for(auto gpu:devices){VkPhysicalDeviceProperties props{};VkPhysicalDeviceMemoryProperties memory{};vkGetPhysicalDeviceProperties(gpu,&props);vkGetPhysicalDeviceMemoryProperties(gpu,&memory);quint64 local=0;for(uint32_t i=0;i<memory.memoryHeapCount;++i)if(memory.memoryHeaps[i].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)local+=memory.memoryHeaps[i].size;auto score=(props.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU?1000000LL:props.deviceType==VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU?100000LL:0)+qint64(local/(1024*1024));if(score>best){best=score;report["gpu"]=QString::fromUtf8(props.deviceName);report["deviceLocalBytes"]=QString::number(local);report["dedicatedGpu"]=props.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;}}}}
    vkDestroyInstance(instance,nullptr);return report;
}
inline QJsonObject benchmark(){
    auto report=detect();const int cores=report["logicalProcessors"].toInt(1);QJsonArray trials;int bestWorkers=1;double bestRate=0;
    // Fixed, modest working set. This is a synthetic CPU/RAM test, not a GPU FPS test.
    std::vector<float> data(4*1024*1024,1.25f);QList<int> choices{1};for(int n:{qMax(1,cores/2),qMax(1,cores-1)}){n=qMin(n,16);if(!choices.contains(n))choices.append(n);}
    for(int workers:choices){QThreadPool pool;pool.setMaxThreadCount(workers);auto run=[&]{QList<QFuture<void>> futures;for(int t=0;t<workers;++t)futures.append(QtConcurrent::run(&pool,[&,t]{auto begin=data.size()*t/workers,end=data.size()*(t+1)/workers;for(int pass=0;pass<8;++pass)for(size_t i=begin;i<end;++i)data[i]=std::sqrt(data[i]*1.00001f+float(i%97)*.0001f); }));for(auto &future:futures)future.waitForFinished();};run();QElapsedTimer timer;timer.start();run();auto ns=qMax(qint64(1),timer.nsecsElapsed());double rate=double(data.size())*8*1e9/ns;trials.append(QJsonObject{{"workers",workers},{"milliseconds",double(ns)/1e6},{"millionElementsPerSecond",rate/1e6}});if(rate>bestRate*1.05){bestRate=rate;bestWorkers=workers;}}
    std::vector<char> src(32*1024*1024,1),dst(src.size());QElapsedTimer timer;timer.start();quint64 checksum=0;for(int i=0;i<8;++i){std::memcpy(dst.data(),src.data(),src.size());checksum+=quint8(dst[size_t(i)*4096]);}auto ns=qMax(qint64(1),timer.nsecsElapsed());
    report["cpuTrials"]=trials;report["recommendedWorkers"]=bestWorkers;report["ramCopyGiBPerSecond"]=double(src.size())*8*1e9/ns/AssetMemory::GiB;report["checksum"]=QString::number(checksum);bool ok=false;auto ram=report["ramBytes"].toString().toULongLong(&ok);report["recommendedMemoryGiB"]=ok?qBound(1,qRound(double(ram)/AssetMemory::GiB/2),64):8;return report;
}
}
