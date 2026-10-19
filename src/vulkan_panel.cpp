#include "vulkan_panel.h"
#include "theme.h"
#include "local_config.h"
#include "mesh_debug.h"
#include <QComboBox>
#include <QSpinBox>
#include <QSignalBlocker>
#include <functional>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QGuiApplication>
#include <QShowEvent>
#include <QHideEvent>
#include <QPlatformSurfaceEvent>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <QFile>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QtMath>
#include <cstring>
#include <cmath>
#ifdef Q_OS_MACOS
#define VK_USE_PLATFORM_METAL_EXT
#include "metal_surface.h"
#endif
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#endif
#include <vulkan/vulkan.h>
namespace {
void check(VkResult r,const char *operation){if(r!=VK_SUCCESS)throw std::runtime_error(QString("%1: Vulkan %2").arg(operation).arg(int(r)).toStdString());}
struct DrawVertex { MeshVertex vertex;float color[3];uint32_t textureRecord=0; };
class Surface final:public QWidget {
public:
    QLabel *status;QTimer timer;
    VkInstance instance=VK_NULL_HANDLE;VkSurfaceKHR surface=VK_NULL_HANDLE;VkPhysicalDevice gpu=VK_NULL_HANDLE;VkDevice device=VK_NULL_HANDLE;VkQueue queue=VK_NULL_HANDLE;
    uint32_t family=0;VkSwapchainKHR swap=VK_NULL_HANDLE;VkExtent2D extent{};VkRenderPass pass=VK_NULL_HANDLE;VkCommandPool pool=VK_NULL_HANDLE;VkCommandBuffer command=VK_NULL_HANDLE;VkFence fence=VK_NULL_HANDLE;VkSemaphore acquired=VK_NULL_HANDLE;
    std::vector<VkImageView> views;std::vector<VkFramebuffer> frames;std::vector<VkSemaphore> presented;
    void *layer=nullptr;bool failed=false,dirty=true;QString gpuName;
    std::shared_ptr<MeshData> mesh;
    MeshScene *scene=nullptr;QVector<MeshSpan> spans;
    std::unique_ptr<MeshDebug> debug;
    MeshElement element=MeshElement::Vertex;
    int selected=-1,firstLabel=0,labelLimit=200;
    bool showLabels=false,overlayDirty=true;
    float textScale=1.5f;
    bool gradientVertices=false,gradientFaces=false,customColors=false;
    QVector<QColor> gradientStops{QColor("#2070FF"),QColor("#30E0A0"),QColor("#FF4030")};
    QColor textColor{qApp->palette().color(QPalette::Text)},vertexColor{"#FFD060"};
    VkBuffer overlayBuffer=VK_NULL_HANDLE;VkDeviceMemory overlayMemory=VK_NULL_HANDLE;
    uint32_t overlayCount=0;VkPipeline overlayPipeline=VK_NULL_HANDLE;
    QPoint press;std::function<void()> selectionChanged;
    QVector3D eye()const{float y=qDegreesToRadians(yaw),p=qDegreesToRadians(pitch);return target+QVector3D(distance*std::cos(p)*std::sin(y),distance*std::sin(p),distance*std::cos(p)*std::cos(y));}
    QMatrix4x4 matrix()const{QMatrix4x4 projection;projection.perspective(45,float(width())/qMax(1,height()),.01f,100.f);QMatrix4x4 correction;correction(1,1)=-1;correction(2,2)=.5f;correction(2,3)=.5f;QMatrix4x4 camera;camera.lookAt(eye(),target,{0,1,0});return correction*projection*camera;}
    void releaseOverlay(){if(device){if(overlayBuffer)vkDestroyBuffer(device,overlayBuffer,nullptr);if(overlayMemory)vkFreeMemory(device,overlayMemory,nullptr);}overlayBuffer=VK_NULL_HANDLE;overlayMemory=VK_NULL_HANDLE;overlayCount=0;overlayDirty=true;}
    void uploadOverlay(){
        releaseOverlay();overlayDirty=false;if(!debug||(!showLabels&&selected<0))return;
        QVector<MeshLabel> labels;int localSelected=scene->selection().index;
        for(const auto &span:spans){
            if(element==MeshElement::Mesh){
                if(!showLabels&&span.id!=scene->active())continue;
                for(int start=0;start<span.vertexCount;start+=1000){auto candidates=debug->labels(matrix(),eye(),size(),MeshElement::Vertex,span.vertexStart+start,qMin(1000,span.vertexCount-start));if(!candidates.isEmpty()){auto label=candidates.first();label.id=span.id;labels.append(label);break;}}
            }else if(span.id==scene->active()){
                int base=element==MeshElement::Vertex?span.vertexStart:span.faceStart,count=element==MeshElement::Vertex?span.vertexCount:span.faceCount;
                int first=showLabels?firstLabel:localSelected,limit=showLabels?labelLimit:1;
                if(first>=0&&first<count)labels=debug->labels(matrix(),eye(),size(),element,base+first,qMin(limit,count-first));
                if(showLabels&&localSelected>=0&&(localSelected<first||localSelected>=first+limit))labels+=debug->labels(matrix(),eye(),size(),element,base+localSelected,1);
                for(auto &label:labels)label.id-=base;
            }
        }
        if(element==MeshElement::Mesh)localSelected=localSelected>=0?scene->active():-1;
        auto vertices=labelGeometry(labels,size(),element,textScale,localSelected,false,showLabels);overlayCount=vertices.size();if(vertices.isEmpty())return;
        VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};QVector<DrawVertex> gpuVertices;for(auto vertex:vertices)gpuVertices.append({vertex,{1,1,1}});b.size=gpuVertices.size()*sizeof(DrawVertex);b.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;b.sharingMode=VK_SHARING_MODE_EXCLUSIVE;check(vkCreateBuffer(device,&b,nullptr,&overlayBuffer),"Create labels");VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,overlayBuffer,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);check(vkAllocateMemory(device,&a,nullptr,&overlayMemory),"Label memory");check(vkBindBufferMemory(device,overlayBuffer,overlayMemory,0),"Bind labels");void *mapped;check(vkMapMemory(device,overlayMemory,0,b.size,0,&mapped),"Upload labels");std::memcpy(mapped,gpuVertices.constData(),size_t(b.size));vkUnmapMemory(device,overlayMemory);
    }
    QColor background{graphicsBackground()},modelColor{"#78B4E8"},gridColor{graphicsGrid()};
    bool showFaces=true,showWire=false,showPoints=false,showNormals=false,showGrid=true,lighting=true,geometryDirty=true;
    float yaw=35,pitch=20,distance=3.5,lightAngle=45;QPoint mouse;QVector3D target;
    VkBuffer geometry=VK_NULL_HANDLE;VkDeviceMemory geometryMemory=VK_NULL_HANDLE;
    VkImage depthImage=VK_NULL_HANDLE;VkDeviceMemory depthMemory=VK_NULL_HANDLE;VkImageView depthView=VK_NULL_HANDLE;
    VkDescriptorSetLayout textureLayout=VK_NULL_HANDLE;VkDescriptorPool texturePool=VK_NULL_HANDLE;VkDescriptorSet textureSet=VK_NULL_HANDLE;
    VkBuffer textureBuffer=VK_NULL_HANDLE;VkDeviceMemory textureMemory=VK_NULL_HANDLE;
    void releaseTextureBuffer(){if(textureBuffer)vkDestroyBuffer(device,textureBuffer,nullptr);if(textureMemory)vkFreeMemory(device,textureMemory,nullptr);textureBuffer=VK_NULL_HANDLE;textureMemory=VK_NULL_HANDLE;}
    QMap<int,uint32_t> uploadTextures(){
        releaseTextureBuffer();QMap<int,uint32_t> records;QVector<uint32_t> pixels((spans.size()+1)*4,0);
        for(int i=0;i<spans.size();++i){auto *entry=scene->entry(spans[i].id);if(!entry||entry->texture.isNull()||!entry->data->hasUvs)continue;const auto &image=entry->texture;uint32_t record=(i+1)*4;records.insert(entry->id,record);pixels[record]=image.width();pixels[record+1]=image.height();pixels[record+2]=pixels.size();
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){auto rgba=image.pixel(x,y);pixels.append(uint32_t(qRed(rgba))|(uint32_t(qGreen(rgba))<<8)|(uint32_t(qBlue(rgba))<<16)|(uint32_t(qAlpha(rgba))<<24));}
        }
        VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=pixels.size()*sizeof(uint32_t);VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(gpu,&properties);if(b.size>properties.limits.maxStorageBufferRange)throw std::runtime_error("Texture data exceeds this GPU's storage-buffer limit.");b.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;b.sharingMode=VK_SHARING_MODE_EXCLUSIVE;check(vkCreateBuffer(device,&b,nullptr,&textureBuffer),"Create texture buffer");VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,textureBuffer,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);check(vkAllocateMemory(device,&a,nullptr,&textureMemory),"Texture memory");check(vkBindBufferMemory(device,textureBuffer,textureMemory,0),"Bind textures");void *mapped;check(vkMapMemory(device,textureMemory,0,b.size,0,&mapped),"Upload textures");std::memcpy(mapped,pixels.constData(),size_t(b.size));vkUnmapMemory(device,textureMemory);
        VkDescriptorBufferInfo info{textureBuffer,0,b.size};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=textureSet;write.dstBinding=0;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write.pBufferInfo=&info;vkUpdateDescriptorSets(device,1,&write,0,nullptr);return records;
    }
    VkPipelineLayout pipelineLayout=VK_NULL_HANDLE;VkPipeline solid=VK_NULL_HANDLE,lines=VK_NULL_HANDLE,points=VK_NULL_HANDLE;
    uint32_t faceCount=0,wireStart=0,wireCount=0,pointStart=0,pointCount=0,normalStart=0,normalCount=0,gridStart=0,gridCount=0;
    uint32_t memoryType(uint32_t mask,VkMemoryPropertyFlags required){VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(gpu,&p);for(uint32_t i=0;i<p.memoryTypeCount;++i)if((mask&(1u<<i))&&(p.memoryTypes[i].propertyFlags&required)==required)return i;throw std::runtime_error("No compatible GPU memory.");}
    void releaseGeometry(){if(!device)return;releaseTextureBuffer();if(geometry)vkDestroyBuffer(device,geometry,nullptr);if(geometryMemory)vkFreeMemory(device,geometryMemory,nullptr);geometry=VK_NULL_HANDLE;geometryMemory=VK_NULL_HANDLE;geometryDirty=true;}
    void releaseDrawing(){if(!device)return;releaseTextureBuffer();if(texturePool)vkDestroyDescriptorPool(device,texturePool,nullptr);texturePool=VK_NULL_HANDLE;textureSet=VK_NULL_HANDLE;for(auto p:{solid,lines,points,overlayPipeline})if(p)vkDestroyPipeline(device,p,nullptr);solid=lines=points=overlayPipeline=VK_NULL_HANDLE;if(pipelineLayout)vkDestroyPipelineLayout(device,pipelineLayout,nullptr);pipelineLayout=VK_NULL_HANDLE;if(textureLayout)vkDestroyDescriptorSetLayout(device,textureLayout,nullptr);textureLayout=VK_NULL_HANDLE;if(depthView)vkDestroyImageView(device,depthView,nullptr);depthView=VK_NULL_HANDLE;if(depthImage)vkDestroyImage(device,depthImage,nullptr);depthImage=VK_NULL_HANDLE;if(depthMemory)vkFreeMemory(device,depthMemory,nullptr);depthMemory=VK_NULL_HANDLE;}
    void createDepth(){VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=VK_FORMAT_D32_SFLOAT;i.extent={extent.width,extent.height,1};i.mipLevels=1;i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;i.sharingMode=VK_SHARING_MODE_EXCLUSIVE;check(vkCreateImage(device,&i,nullptr,&depthImage),"Create depth buffer");VkMemoryRequirements req;vkGetImageMemoryRequirements(device,depthImage,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);check(vkAllocateMemory(device,&a,nullptr,&depthMemory),"Depth buffer memory");check(vkBindImageMemory(device,depthImage,depthMemory,0),"Bind depth buffer");VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=depthImage;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=i.format;v.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};check(vkCreateImageView(device,&v,nullptr,&depthView),"Depth view");}
    VkShaderModule shader(const char *path){QFile f(path);if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Shader not found.");auto bytes=f.readAll();std::vector<uint32_t> words(size_t(bytes.size())/4);if(bytes.isEmpty()||bytes.size()%4)throw std::runtime_error("Invalid shader.");std::memcpy(words.data(),bytes.constData(),size_t(bytes.size()));VkShaderModuleCreateInfo c{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};c.codeSize=bytes.size();c.pCode=words.data();VkShaderModule module;check(vkCreateShaderModule(device,&c,nullptr,&module),"Create shader");return module;}
    void createPipelines(){geometryDirty=true;
        VkDescriptorSetLayoutBinding textureBinding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};layoutInfo.bindingCount=1;layoutInfo.pBindings=&textureBinding;check(vkCreateDescriptorSetLayout(device,&layoutInfo,nullptr,&textureLayout),"Texture layout");VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};poolInfo.maxSets=1;poolInfo.poolSizeCount=1;poolInfo.pPoolSizes=&poolSize;check(vkCreateDescriptorPool(device,&poolInfo,nullptr,&texturePool),"Texture descriptor pool");VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=texturePool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&textureLayout;check(vkAllocateDescriptorSets(device,&allocate,&textureSet),"Texture descriptor set");
        VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,96};VkPipelineLayoutCreateInfo l{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};l.setLayoutCount=1;l.pSetLayouts=&textureLayout;l.pushConstantRangeCount=1;l.pPushConstantRanges=&range;check(vkCreatePipelineLayout(device,&l,nullptr,&pipelineLayout),"Pipeline layout");VkShaderModule vert=VK_NULL_HANDLE,frag=VK_NULL_HANDLE;try{vert=shader(":/shaders/mesh.vert.spv");frag=shader(":/shaders/mesh.frag.spv");
        VkPipelineShaderStageCreateInfo stages[2]{};for(auto &s:stages){s.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;s.pName="main";}stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=vert;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=frag;
        VkVertexInputBindingDescription binding{0,sizeof(DrawVertex),VK_VERTEX_INPUT_RATE_VERTEX};VkVertexInputAttributeDescription attributes[]={{0,0,VK_FORMAT_R32G32B32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32_SFLOAT,12},{2,0,VK_FORMAT_R32G32_SFLOAT,24},{3,0,VK_FORMAT_R32G32B32_SFLOAT,32},{4,0,VK_FORMAT_R32_UINT,44}};VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};vi.vertexBindingDescriptionCount=1;vi.pVertexBindingDescriptions=&binding;vi.vertexAttributeDescriptionCount=5;vi.pVertexAttributeDescriptions=attributes;
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=1;vp.scissorCount=1;VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};ds.depthTestEnable=VK_TRUE;ds.depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL;VkPipelineColorBlendAttachmentState ba{};ba.colorWriteMask=15;ba.blendEnable=VK_TRUE;ba.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;ba.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;ba.colorBlendOp=VK_BLEND_OP_ADD;ba.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;ba.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;ba.alphaBlendOp=VK_BLEND_OP_ADD;VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&ba;VkDynamicState dynamic[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dy.dynamicStateCount=2;dy.pDynamicStates=dynamic;
        VkGraphicsPipelineCreateInfo p{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};p.stageCount=2;p.pStages=stages;p.pVertexInputState=&vi;p.pInputAssemblyState=&ia;p.pViewportState=&vp;p.pRasterizationState=&rs;p.pMultisampleState=&ms;p.pDepthStencilState=&ds;p.pColorBlendState=&bs;p.pDynamicState=&dy;p.layout=pipelineLayout;p.renderPass=pass;
        ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;ds.depthWriteEnable=VK_TRUE;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&solid),"Face pipeline");ia.topology=VK_PRIMITIVE_TOPOLOGY_LINE_LIST;ds.depthWriteEnable=VK_FALSE;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&lines),"Line pipeline");ia.topology=VK_PRIMITIVE_TOPOLOGY_POINT_LIST;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&points),"Point pipeline");ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;ds.depthTestEnable=VK_FALSE;check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&p,nullptr,&overlayPipeline),"Label pipeline");
        }catch(...){if(vert)vkDestroyShaderModule(device,vert,nullptr);if(frag)vkDestroyShaderModule(device,frag,nullptr);throw;}vkDestroyShaderModule(device,vert,nullptr);vkDestroyShaderModule(device,frag,nullptr);
    }
    void upload(){releaseGeometry();auto textureRecords=uploadTextures();QVector<MeshVertex> all,vertices;faceCount=wireCount=pointCount=normalCount=0;
        if(mesh&&!mesh->vertices.isEmpty()){vertices=mesh->vertices;for(int i=0;i<vertices.size();++i)for(int j=0;j<3;++j)vertices[i].position[j]=debug->positions[i][j];
            for(auto index:mesh->triangles)all.append(vertices[index]);faceCount=all.size();wireStart=all.size();for(int i=0;i<mesh->triangles.size();i+=3)for(int j=0;j<3;++j){all.append(vertices[mesh->triangles[i+j]]);all.append(vertices[mesh->triangles[i+(j+1)%3]]);}wireCount=all.size()-wireStart;pointStart=all.size();all+=vertices;pointCount=vertices.size();normalStart=all.size();for(auto v:vertices){all.append(v);for(int j=0;j<3;++j)v.position[j]+=v.normal[j]*.08f;all.append(v);}normalCount=all.size()-normalStart;
        }
        gridStart=all.size();for(int i=-10;i<=10;++i){float t=i*.2f;for(auto p:{QVector3D(t,-1,-2),QVector3D(t,-1,2),QVector3D(-2,-1,t),QVector3D(2,-1,t)}){MeshVertex v{};v.position[0]=p.x();v.position[1]=p.y();v.position[2]=p.z();all.append(v);}}gridCount=all.size()-gridStart;
        VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};QVector<DrawVertex> colored;colored.reserve(all.size());for(int i=0;i<all.size();++i){uint32_t textureRecord=0;QColor color=Qt::white;if(i<int(faceCount)){int face=i/3;for(const auto &span:spans)if(face>=span.faceStart&&face<span.faceStart+span.faceCount){color=indexColor(face-span.faceStart,span.faceCount,gradientStops);textureRecord=textureRecords.value(span.id,0);break;}}else if(i>=int(pointStart)&&i<int(pointStart+pointCount)){int vertex=i-pointStart;for(const auto &span:spans)if(vertex>=span.vertexStart&&vertex<span.vertexStart+span.vertexCount){color=indexColor(vertex-span.vertexStart,span.vertexCount,gradientStops);break;}}colored.append({all[i],{float(color.redF()),float(color.greenF()),float(color.blueF())},textureRecord});}b.size=colored.size()*sizeof(DrawVertex);b.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;b.sharingMode=VK_SHARING_MODE_EXCLUSIVE;check(vkCreateBuffer(device,&b,nullptr,&geometry),"Create GPU vertices");VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,geometry,&req);VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);check(vkAllocateMemory(device,&a,nullptr,&geometryMemory),"Vertex memory");check(vkBindBufferMemory(device,geometry,geometryMemory,0),"Bind vertices");void *mapped;check(vkMapMemory(device,geometryMemory,0,b.size,0,&mapped),"Upload vertices");std::memcpy(mapped,colored.constData(),size_t(b.size));vkUnmapMemory(device,geometryMemory);geometryDirty=false;
    }
    void drawMesh(){VkViewport viewport{0,0,float(extent.width),float(extent.height),0,1};VkRect2D scissor{{0,0},extent};vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);VkDeviceSize zero=0;vkCmdBindVertexBuffers(command,0,1,&geometry,&zero);vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&textureSet,0,nullptr);
        auto mvp=matrix();
        struct Push{float matrix[16],color[4],light[4];}push{};std::memcpy(push.matrix,mvp.constData(),64);float angle=qDegreesToRadians(lightAngle);push.light[0]=std::cos(angle);push.light[1]=.8f;push.light[2]=std::sin(angle);
        auto draw=[&](VkPipeline pipeline,uint32_t first,uint32_t count,QColor color,bool lit,bool mapped=false){if(!count)return;push.color[0]=color.redF();push.color[1]=color.greenF();push.color[2]=color.blueF();push.color[3]=1;push.light[3]=(lit?1:0)+(mapped?2:0);vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);vkCmdPushConstants(command,pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,96,&push);vkCmdDraw(command,count,1,first,0);};
        if(showFaces)draw(solid,0,faceCount,modelColor,lighting,gradientFaces);if(showGrid)draw(lines,gridStart,gridCount,gridColor,false);if(showWire)draw(lines,wireStart,wireCount,(workbenchDark()?QColor("#93A8C1"):QColor("#33465E")),false);if(showPoints)draw(points,pointStart,pointCount,vertexColor,false,gradientVertices);if(showNormals)draw(lines,normalStart,normalCount,(workbenchDark()?QColor("#70E090"):QColor("#267546")),false);
        if(selected>=0&&mesh){if(element==MeshElement::Face&&selected<faceCount/3)draw(lines,wireStart+selected*6,6,graphicsSelection(),false);else if(element==MeshElement::Mesh)for(const auto &span:spans)if(span.id==scene->active())draw(lines,wireStart+span.faceStart*6,span.faceCount*6,graphicsSelection(),false);}
        if(overlayCount){QMatrix4x4 identity;std::memcpy(push.matrix,identity.constData(),64);vkCmdBindVertexBuffers(command,0,1,&overlayBuffer,&zero);draw(overlayPipeline,0,overlayCount,textColor,false);}
    }

    explicit Surface(QLabel *label):status(label){setMinimumSize(280,200);setAttribute(Qt::WA_NativeWindow);setAttribute(Qt::WA_PaintOnScreen);setAttribute(Qt::WA_NoSystemBackground);timer.setInterval(33);connect(&timer,&QTimer::timeout,this,[this]{draw();});}
    ~Surface()override{timer.stop();cleanup();}
    QPaintEngine *paintEngine()const override{return nullptr;}
    void retry(){cleanup();failed=false;dirty=true;if(isVisible())timer.start();}
    void clearSwap(){if(!device)return;vkDeviceWaitIdle(device);for(auto f:frames)vkDestroyFramebuffer(device,f,nullptr);frames.clear();for(auto v:views)vkDestroyImageView(device,v,nullptr);views.clear();for(auto s:presented)vkDestroySemaphore(device,s,nullptr);presented.clear();releaseDrawing();if(pass)vkDestroyRenderPass(device,pass,nullptr);pass=VK_NULL_HANDLE;if(swap)vkDestroySwapchainKHR(device,swap,nullptr);swap=VK_NULL_HANDLE;}
    void cleanup(){clearSwap();releaseGeometry();releaseOverlay();if(device){if(acquired)vkDestroySemaphore(device,acquired,nullptr);if(fence)vkDestroyFence(device,fence,nullptr);if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);}device=VK_NULL_HANDLE;acquired=VK_NULL_HANDLE;fence=VK_NULL_HANDLE;pool=VK_NULL_HANDLE;command=VK_NULL_HANDLE;if(surface&&instance)vkDestroySurfaceKHR(instance,surface,nullptr);surface=VK_NULL_HANDLE;if(instance)vkDestroyInstance(instance,nullptr);instance=VK_NULL_HANDLE;
#ifdef Q_OS_MACOS
        if(layer)releaseMetalLayer(layer);
#endif
        layer=nullptr;
    }
    void initialize(){
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
        throw std::runtime_error("Native Linux surfaces are not yet implemented; the editor remains available.");
#else
#ifdef Q_OS_MACOS
        if(QGuiApplication::platformName()!="cocoa")throw std::runtime_error("Vulkan requires a macOS graphical session (Cocoa).");
        if(qEnvironmentVariableIsEmpty("VK_DRIVER_FILES")&&qEnvironmentVariableIsEmpty("VK_ICD_FILENAMES")&&QString(WORKBENCH_VULKAN_DRIVER).size())qputenv("VK_DRIVER_FILES",WORKBENCH_VULKAN_DRIVER);
        const auto nativeView=winId();
        const char *extensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_EXT_METAL_SURFACE_EXTENSION_NAME,VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Binary Workbench";app.apiVersion=VK_API_VERSION_1_0;
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledExtensionCount=3;ci.ppEnabledExtensionNames=extensions;ci.flags=VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;check(vkCreateInstance(&ci,nullptr,&instance),"Create instance");
        layer=createMetalLayer(nativeView);resizeMetalLayer(layer,qRound(width()*devicePixelRatioF()),qRound(height()*devicePixelRatioF()),devicePixelRatioF());
        auto create=reinterpret_cast<PFN_vkCreateMetalSurfaceEXT>(vkGetInstanceProcAddr(instance,"vkCreateMetalSurfaceEXT"));if(!create)throw std::runtime_error("VK_EXT_metal_surface unavailable.");VkMetalSurfaceCreateInfoEXT metal{VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};metal.pLayer=static_cast<const CAMetalLayer *>(layer);check(create(instance,&metal,nullptr,&surface),"Create Metal surface");
#else
        if(QGuiApplication::platformName()!="windows")
            throw std::runtime_error("Vulkan requires a Windows graphical session.");
        // Obtain the HWND before creating the instance: WinIdChange can clean up Vulkan.
        const auto hwnd=reinterpret_cast<HWND>(winId());
        const char *extensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName="Binary Workbench";app.apiVersion=VK_API_VERSION_1_0;
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ci.pApplicationInfo=&app;ci.enabledExtensionCount=2;ci.ppEnabledExtensionNames=extensions;
        check(vkCreateInstance(&ci,nullptr,&instance),"Create instance");
        auto create=reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance,"vkCreateWin32SurfaceKHR"));
        if(!create)throw std::runtime_error("VK_KHR_win32_surface unavailable.");
        VkWin32SurfaceCreateInfoKHR native{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        native.hwnd=hwnd;
        native.hinstance=reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd,GWLP_HINSTANCE));
        check(create(instance,&native,nullptr,&surface),"Create Win32 surface");
#endif
        uint32_t count=0;check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"Enumerate GPUs");std::vector<VkPhysicalDevice> devices(count);check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"Enumerate GPUs");gpu=VK_NULL_HANDLE;
        for(auto candidate:devices){uint32_t n=0;vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,nullptr);std::vector<VkQueueFamilyProperties> props(n);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,props.data());for(uint32_t i=0;i<n;++i){VkBool32 present=VK_FALSE;check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate,i,surface,&present),"Presentation support");if((props[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&present){gpu=candidate;family=i;break;}}if(gpu)break;}
        if(!gpu)throw std::runtime_error("No GPU supporting Vulkan presentation was found.");VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(gpu,&props);gpuName=QString::fromUtf8(props.deviceName);
        uint32_t n=0;check(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&n,nullptr),"GPU extensions");std::vector<VkExtensionProperties> available(n);check(vkEnumerateDeviceExtensionProperties(gpu,nullptr,&n,available.data()),"GPU extensions");std::vector<const char *> enabled{VK_KHR_SWAPCHAIN_EXTENSION_NAME};for(auto e:available)if(QString(e.extensionName)=="VK_KHR_portability_subset")enabled.push_back("VK_KHR_portability_subset");
        float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;VkDeviceCreateInfo d{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};d.queueCreateInfoCount=1;d.pQueueCreateInfos=&q;d.enabledExtensionCount=uint32_t(enabled.size());d.ppEnabledExtensionNames=enabled.data();check(vkCreateDevice(gpu,&d,nullptr,&device),"Create device");vkGetDeviceQueue(device,family,0,&queue);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;pc.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(vkCreateCommandPool(device,&pc,nullptr,&pool),"Create command pool");VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};alloc.commandPool=pool;alloc.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;alloc.commandBufferCount=1;check(vkAllocateCommandBuffers(device,&alloc,&command),"Create command buffer");VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};fc.flags=VK_FENCE_CREATE_SIGNALED_BIT;check(vkCreateFence(device,&fc,nullptr,&fence),"Create fence");VkSemaphoreCreateInfo sc{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};check(vkCreateSemaphore(device,&sc,nullptr,&acquired),"Create semaphore");
#endif
    }
    void recreate(){
        clearSwap();VkSurfaceCapabilitiesKHR caps;check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu,surface,&caps),"Surface capabilities");
        extent=caps.currentExtent;if(extent.width==UINT32_MAX){extent.width=std::clamp(uint32_t(qMax(1,qRound(width()*devicePixelRatioF()))),caps.minImageExtent.width,caps.maxImageExtent.width);extent.height=std::clamp(uint32_t(qMax(1,qRound(height()*devicePixelRatioF()))),caps.minImageExtent.height,caps.maxImageExtent.height);}
        if(!extent.width||!extent.height)return;
#ifdef Q_OS_MACOS
        resizeMetalLayer(layer,int(extent.width),int(extent.height),devicePixelRatioF());
#endif
        uint32_t n=0;check(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu,surface,&n,nullptr),"Formats");if(!n)throw std::runtime_error("No surface formats available.");std::vector<VkSurfaceFormatKHR> formats(n);check(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu,surface,&n,formats.data()),"Formats");auto format=formats.front();for(auto f:formats)if(f.format==VK_FORMAT_B8G8R8A8_UNORM)format=f;if(format.format==VK_FORMAT_UNDEFINED)format.format=VK_FORMAT_B8G8R8A8_UNORM;
        VkCompositeAlphaFlagBitsKHR alpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;for(auto a:{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})if(caps.supportedCompositeAlpha&a){alpha=a;break;}
        VkSwapchainCreateInfoKHR s{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};s.surface=surface;s.minImageCount=caps.minImageCount+1;if(caps.maxImageCount)s.minImageCount=qMin(s.minImageCount,caps.maxImageCount);s.imageFormat=format.format;s.imageColorSpace=format.colorSpace;s.imageExtent=extent;s.imageArrayLayers=1;s.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;s.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;s.preTransform=caps.currentTransform;s.compositeAlpha=alpha;s.presentMode=VK_PRESENT_MODE_FIFO_KHR;s.clipped=VK_TRUE;check(vkCreateSwapchainKHR(device,&s,nullptr,&swap),"Create swapchain");
        VkAttachmentDescription attachment{};attachment.format=format.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&ref;VkSubpassDependency dep{};dep.srcSubpass=VK_SUBPASS_EXTERNAL;dep.dstSubpass=0;dep.srcStageMask=dep.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;VkAttachmentDescription depth=attachment;depth.format=VK_FORMAT_D32_SFLOAT;depth.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;depth.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;VkAttachmentDescription attachments[]={attachment,depth};VkAttachmentReference depthRef{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};sub.pDepthStencilAttachment=&depthRef;dep.srcStageMask|=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;dep.dstStageMask=dep.srcStageMask;dep.dstAccessMask|=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=2;rp.pAttachments=attachments;rp.subpassCount=1;rp.pSubpasses=&sub;rp.dependencyCount=1;rp.pDependencies=&dep;check(vkCreateRenderPass(device,&rp,nullptr,&pass),"Create render pass");
        createDepth();createPipelines();
        check(vkGetSwapchainImagesKHR(device,swap,&n,nullptr),"Images");std::vector<VkImage> images(n);check(vkGetSwapchainImagesKHR(device,swap,&n,images.data()),"Images");for(auto image:images){VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=image;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=format.format;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};VkImageView view;check(vkCreateImageView(device,&v,nullptr,&view),"Create view");views.push_back(view);VkFramebufferCreateInfo f{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};f.renderPass=pass;VkImageView attachments[]={views.back(),depthView};f.attachmentCount=2;f.pAttachments=attachments;f.width=extent.width;f.height=extent.height;f.layers=1;VkFramebuffer frame;check(vkCreateFramebuffer(device,&f,nullptr,&frame),"Create framebuffer");frames.push_back(frame);VkSemaphoreCreateInfo sc{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};VkSemaphore sem;check(vkCreateSemaphore(device,&sc,nullptr,&sem),"Create presentation semaphore");presented.push_back(sem);}dirty=false;
    }
    void draw(){if(failed||!isVisible()||width()<1||height()<1)return;try{if(!instance)initialize();if(dirty)recreate();if(!swap)return;
        auto ready=vkWaitForFences(device,1,&fence,VK_TRUE,0);if(ready==VK_TIMEOUT)return;check(ready,"Wait for frame");if(geometryDirty)upload();if(overlayDirty)uploadOverlay();uint32_t index;auto result=vkAcquireNextImageKHR(device,swap,0,acquired,VK_NULL_HANDLE,&index);if(result==VK_NOT_READY||result==VK_TIMEOUT)return;if(result==VK_ERROR_OUT_OF_DATE_KHR){dirty=true;return;}if(result!=VK_SUBOPTIMAL_KHR)check(result,"Acquire image");else dirty=true;
        check(vkResetCommandBuffer(command,0),"Reset commands");VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin),"Begin commands");VkClearValue colors[2]{};colors[0].color={{float(background.redF()),float(background.greenF()),float(background.blueF()),1.f}};colors[1].depthStencil={1.f,0};VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rp.renderPass=pass;rp.framebuffer=frames[index];rp.renderArea.extent=extent;rp.clearValueCount=2;rp.pClearValues=colors;vkCmdBeginRenderPass(command,&rp,VK_SUBPASS_CONTENTS_INLINE);drawMesh();vkCmdEndRenderPass(command);check(vkEndCommandBuffer(command),"End commands");VkPipelineStageFlags stage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acquired;submit.pWaitDstStageMask=&stage;submit.commandBufferCount=1;submit.pCommandBuffers=&command;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&presented[index];check(vkResetFences(device,1,&fence),"Reset fence");check(vkQueueSubmit(queue,1,&submit,fence),"Submit frame");VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&presented[index];present.swapchainCount=1;present.pSwapchains=&swap;present.pImageIndices=&index;result=vkQueuePresentKHR(queue,&present);if(result==VK_ERROR_OUT_OF_DATE_KHR||result==VK_SUBOPTIMAL_KHR)dirty=true;else check(result,"Present");status->setText(QString("Vulkan · %1 · %2 × %3 · %4 vertices · %5 triangles").arg(gpuName).arg(extent.width).arg(extent.height).arg(mesh?mesh->vertices.size():0).arg(faceCount/3));
    }catch(const std::exception &e){timer.stop();failed=true;cleanup();status->setText("Could not initialize the Vulkan view: "+QString::fromUtf8(e.what()));}}
protected:
    void mousePressEvent(QMouseEvent *e)override{press=mouse=e->position().toPoint();}
    void mouseReleaseEvent(QMouseEvent *e)override{if(e->button()==Qt::LeftButton&&(press-e->position().toPoint()).manhattanLength()<4&&debug){
        int picked=debug->pick(matrix(),eye(),size(),e->position(),element==MeshElement::Mesh?MeshElement::Face:element);
        if(picked<0){scene->select(scene->active(),element,-1);return;}
        for(const auto &span:spans){int base=element==MeshElement::Vertex?span.vertexStart:span.faceStart,count=element==MeshElement::Vertex?span.vertexCount:span.faceCount;if(picked>=base&&picked<base+count){scene->select(span.id,element,element==MeshElement::Mesh?0:picked-base);break;}}
    }}
    void mouseMoveEvent(QMouseEvent *e)override{if(e->buttons())overlayDirty=true;auto delta=e->position().toPoint()-mouse;mouse=e->position().toPoint();if(e->buttons()&Qt::LeftButton){yaw+=delta.x()*.5f;pitch=qBound(-89.f,pitch+delta.y()*.5f,89.f);}if(e->buttons()&(Qt::RightButton|Qt::MiddleButton)){float a=qDegreesToRadians(yaw);target+=QVector3D(-std::cos(a)*delta.x(),delta.y(),std::sin(a)*delta.x())*distance*.001f;}}
    void wheelEvent(QWheelEvent *e)override{overlayDirty=true;distance=qBound(.2f,distance*float(std::exp(-e->angleDelta().y()*.001)),30.f);e->accept();}
    void showEvent(QShowEvent *e)override{QWidget::showEvent(e);dirty=true;if(!failed)timer.start();}
    void hideEvent(QHideEvent *e)override{timer.stop();QWidget::hideEvent(e);}
    void resizeEvent(QResizeEvent *e)override{dirty=true;overlayDirty=true;QWidget::resizeEvent(e);}
    bool event(QEvent *e)override{if(e->type()==QEvent::WinIdChange&&instance){cleanup();dirty=true;}if(e->type()==QEvent::PlatformSurface&&static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType()==QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed){cleanup();dirty=true;}return QWidget::event(e);}
};
}
VulkanPanel::VulkanPanel(QWidget *parent):QWidget(parent){setObjectName("vulkanPanel");scene_=new MeshScene(this);auto *layout=new QVBoxLayout(this);auto *status=new QLabel("Load a mesh from the 3D Settings panel.");status->setObjectName("vulkanStatus");status->setWordWrap(true);auto *surface=new Surface(status);surface_=surface;connect(qApp,&QApplication::paletteChanged,this,[this,surface](const QPalette &){if(!surface->customColors){surface->background=graphicsBackground();surface->gridColor=graphicsGrid();surface->textColor=qApp->palette().color(QPalette::Text);}surface->overlayDirty=true;surface->update();emit visualStyleChanged();});surface->scene=scene_;surface->setObjectName("vulkanSurface");auto *bar=new QHBoxLayout;layout->addLayout(bar);
    auto toggle=[&](QString label,bool *value){auto *box=new QCheckBox(label);box->setChecked(*value);bar->addWidget(box);connect(box,&QCheckBox::toggled,this,[value](bool enabled){*value=enabled;});};toggle("Faces",&surface->showFaces);toggle("Edges",&surface->showWire);toggle("Vertices",&surface->showPoints);toggle("Normals",&surface->showNormals);
    auto *home=new QPushButton("Frame All");bar->addWidget(home);connect(home,&QPushButton::clicked,this,[surface]{surface->yaw=35;surface->pitch=20;surface->distance=3.5;surface->target={};surface->overlayDirty=true;});auto *options=new QPushButton("Options…");bar->addWidget(options);
    connect(options,&QPushButton::clicked,this,[this,surface]{QDialog dialog(this);dialog.setWindowTitle("3D View Options");auto *form=new QFormLayout(&dialog);auto color=[&](QString label,QColor *value){auto *button=new QPushButton(value->name());form->addRow(label,button);connect(button,&QPushButton::clicked,&dialog,[&,button,value]{auto c=QColorDialog::getColor(*value,&dialog);if(c.isValid()){*value=c;surface->customColors=true;button->setText(c.name());}});};color("Background",&surface->background);color("Faces",&surface->modelColor);color("Vertices",&surface->vertexColor);color("Text",&surface->textColor);
        auto *vertexGradient=new QCheckBox;vertexGradient->setChecked(surface->gradientVertices);form->addRow("Gradient by vertex index",vertexGradient);connect(vertexGradient,&QCheckBox::toggled,&dialog,[surface](bool v){surface->gradientVertices=v;});
        auto *faceGradient=new QCheckBox;faceGradient->setChecked(surface->gradientFaces);form->addRow("Gradient by face index",faceGradient);connect(faceGradient,&QCheckBox::toggled,&dialog,[surface](bool v){surface->gradientFaces=v;});
        for(int i=0;i<3;++i){auto *button=new QPushButton(surface->gradientStops[i].name());form->addRow(QString("Gradient %1 %").arg(i*50),button);connect(button,&QPushButton::clicked,&dialog,[&,surface,button,i]{auto c=QColorDialog::getColor(surface->gradientStops[i],&dialog);if(c.isValid()){surface->gradientStops[i]=c;button->setText(c.name());surface->geometryDirty=true;}});}
        auto *textSize=new QDoubleSpinBox;textSize->setRange(1,4);textSize->setSingleStep(.25);textSize->setValue(surface->textScale);form->addRow("Label size",textSize);connect(textSize,&QDoubleSpinBox::valueChanged,&dialog,[surface](double v){surface->textScale=v;surface->overlayDirty=true;});color("Grid",&surface->gridColor);auto *grid=new QCheckBox;grid->setChecked(surface->showGrid);form->addRow("Show grid",grid);connect(grid,&QCheckBox::toggled,&dialog,[surface](bool v){surface->showGrid=v;});auto *light=new QCheckBox;light->setChecked(surface->lighting);form->addRow("Lighting",light);connect(light,&QCheckBox::toggled,&dialog,[surface](bool v){surface->lighting=v;});auto *angle=new QDoubleSpinBox;angle->setRange(-180,180);angle->setValue(surface->lightAngle);form->addRow("Light direction (°)",angle);connect(angle,&QDoubleSpinBox::valueChanged,&dialog,[surface](double v){surface->lightAngle=v;});auto *close=new QPushButton("Close");form->addRow(close);connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);dialog.exec();emit visualStyleChanged();});
    auto *debugBar=new QHBoxLayout;layout->addLayout(debugBar);
    auto *mode=new QComboBox;mode->setObjectName("meshSelectionMode");mode->addItems({"Vertices","Faces","Meshes"});debugBar->addWidget(new QLabel("Select"));debugBar->addWidget(mode);
    auto *labels=new QCheckBox("Visible indices");labels->setObjectName("meshLabels");debugBar->addWidget(labels);
    auto *first=new QSpinBox;first->setObjectName("meshLabelFirst");first->setRange(0,599999);first->setPrefix("From ");debugBar->addWidget(first);
    auto *limit=new QSpinBox;limit->setObjectName("meshLabelLimit");limit->setRange(1,1000);limit->setValue(200);limit->setPrefix("Count ");debugBar->addWidget(limit);
    auto *details=new QLabel("Mesh 0 · No selection · Zero-based indices");details->setObjectName("meshSelectionInfo");details->setWordWrap(true);
    auto syncSelection=[this,surface,details,mode]{
        auto selection=scene_->selection();surface->element=selection.kind;surface->selected=-1;
        {QSignalBlocker block(mode);mode->setCurrentIndex(int(selection.kind));}
        const auto *entry=scene_->entry(scene_->active());
        if(!entry){details->setText("Empty collection · Add a mesh in 3D Settings.");surface->overlayDirty=true;return;}
        for(const auto &span:surface->spans)if(span.id==entry->id&&selection.index>=0)surface->selected=selection.kind==MeshElement::Vertex?span.vertexStart+selection.index:selection.kind==MeshElement::Face?span.faceStart+selection.index:0;
        auto &m=*entry->data;int id=selection.index;QString title=QString("%1 · ID %2").arg(entry->name).arg(entry->id);
        if(id<0)details->setText(title+" · No selection");
        else if(selection.kind==MeshElement::Vertex){auto v=m.vertices[id];details->setText(title+QString(" · Vertex %1\nXYZ: %2, %3, %4 · UV: %5, %6").arg(id).arg(v.position[0]).arg(v.position[1]).arg(v.position[2]).arg(v.uv[0]).arg(v.uv[1])+(id<m.vertexOffsets.size()?QString(" · Offset: 0x%1").arg(m.vertexOffsets[id],0,16):QString(" · No source offset")));}
        else if(selection.kind==MeshElement::Face){details->setText(title+QString(" · Face %1 · Vertices: %2, %3, %4").arg(id).arg(m.triangles[id*3]).arg(m.triangles[id*3+1]).arg(m.triangles[id*3+2]));if(id<m.faceSources.size()){auto f=m.faceSources[id];details->setText(details->text()+QString("\nStrip: %1 · Slots: %2, %3, %4 · Offsets: 0x%5, 0x%6, 0x%7").arg(f.strip<0?QString("lista"):QString::number(f.strip)).arg(f.indexSlots[0]).arg(f.indexSlots[1]).arg(f.indexSlots[2]).arg(f.offsets[0],0,16).arg(f.offsets[1],0,16).arg(f.offsets[2],0,16));}}
        else details->setText(title+QString(" · %1 vertices · %2 faces").arg(m.vertices.size()).arg(m.triangles.size()/3));
        if(entry->stale)details->setText(details->text()+"\nPrevious snapshot: document modified.");surface->overlayDirty=true;
    };
    connect(scene_,&MeshScene::selectionChanged,this,syncSelection);
    connect(scene_,&MeshScene::collectionChanged,this,syncSelection);
    connect(scene_,&MeshScene::geometryChanged,this,[this,surface]{auto assembly=scene_->assemble();surface->mesh=assembly.data;surface->spans=assembly.spans;auto bounds=scene_->assemble(true);surface->debug=std::make_unique<MeshDebug>(*assembly.data,bounds.data.get());surface->geometryDirty=true;surface->overlayDirty=true;});
    connect(mode,&QComboBox::currentIndexChanged,this,[this,surface](int index){surface->element=MeshElement(index);surface->overlayDirty=true;scene_->select(scene_->active(),MeshElement(index),-1);});
    connect(labels,&QCheckBox::toggled,this,[surface](bool enabled){surface->showLabels=enabled;surface->overlayDirty=true;});
    connect(first,&QSpinBox::valueChanged,this,[surface](int value){surface->firstLabel=value;surface->overlayDirty=true;});
    connect(limit,&QSpinBox::valueChanged,this,[surface](int value){surface->labelLimit=value;surface->overlayDirty=true;});
    layout->addWidget(surface,1);layout->addWidget(details);layout->addWidget(status);auto *hint=new QLabel("Click: select · Drag: orbit · Right/middle drag: pan · Wheel: zoom");layout->addWidget(hint);auto *retry=new QPushButton("Retry Vulkan");layout->addWidget(retry);connect(retry,&QPushButton::clicked,surface,[surface]{surface->retry();});}
void VulkanPanel::setMesh(std::shared_ptr<MeshData> mesh){if(!mesh){scene_->clear();return;}if(!scene_->replace(scene_->active(),mesh))scene_->add(std::move(mesh));}
std::shared_ptr<const MeshData> VulkanPanel::meshSnapshot()const{auto *entry=scene_->entry(scene_->active());return entry?entry->data:nullptr;}
MeshVisualStyle VulkanPanel::visualStyle()const{auto *surface=static_cast<Surface *>(surface_);return {surface->modelColor,surface->vertexColor,surface->textColor,surface->gradientFaces,surface->gradientVertices,surface->gradientStops,surface->background,surface->gridColor,graphicsSelection()};}
