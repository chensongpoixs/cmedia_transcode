/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_CUDA_DEFINE_H_
#define _C_CUDA_DEFINE_H_
#include <cuda.h>
#include <iostream>
#include <opencv2/cudacodec.hpp>
#include <nvcuvid.h>
namespace  dsp
{
    
    static const char* GetVideoCodecString(cudaVideoCodec eCodec) {
        static struct {
            cudaVideoCodec eCodec;
            const char* name;
        } aCodecName[] = {
            { cudaVideoCodec_MPEG1,     "MPEG-1"       },
            { cudaVideoCodec_MPEG2,     "MPEG-2"       },
            { cudaVideoCodec_MPEG4,     "MPEG-4 (ASP)" },
            { cudaVideoCodec_VC1,       "VC-1/WMV"     },
            { cudaVideoCodec_H264,      "AVC/H.264"    },
            { cudaVideoCodec_JPEG,      "M-JPEG"       },
            { cudaVideoCodec_H264_SVC,  "H.264/SVC"    },
            { cudaVideoCodec_H264_MVC,  "H.264/MVC"    },
            { cudaVideoCodec_HEVC,      "H.265/HEVC"   },
            { cudaVideoCodec_VP8,       "VP8"          },
            { cudaVideoCodec_VP9,       "VP9"          },
            { cudaVideoCodec_NumCodecs, "Invalid"      },
            { cudaVideoCodec_YUV420,    "YUV  4:2:0"   },
            { cudaVideoCodec_YV12,      "YV12 4:2:0"   },
            { cudaVideoCodec_NV12,      "NV12 4:2:0"   },
            { cudaVideoCodec_YUYV,      "YUYV 4:2:2"   },
            { cudaVideoCodec_UYVY,      "UYVY 4:2:2"   },
        };

        if (eCodec >= 0 && eCodec <= cudaVideoCodec_NumCodecs) {
            return aCodecName[eCodec].name;
        }
        for (int i = cudaVideoCodec_NumCodecs + 1; i < sizeof(aCodecName) / sizeof(aCodecName[0]); i++) {
            if (eCodec == aCodecName[i].eCodec) {
                return aCodecName[eCodec].name;
            }
        }
        return "Unknown";
    }

    static const char* GetVideoChromaFormatString(cudaVideoChromaFormat eChromaFormat) {
        static struct {
            cudaVideoChromaFormat eChromaFormat;
            const char* name;
        } aChromaFormatName[] = {
            { cudaVideoChromaFormat_Monochrome, "YUV 400 (Monochrome)" },
            { cudaVideoChromaFormat_420,        "YUV 420"              },
            { cudaVideoChromaFormat_422,        "YUV 422"              },
            { cudaVideoChromaFormat_444,        "YUV 444"              },
        };

        if (eChromaFormat >= 0 && eChromaFormat < sizeof(aChromaFormatName) / sizeof(aChromaFormatName[0])) {
            return aChromaFormatName[eChromaFormat].name;
        }
        return "Unknown";
    }



}
static inline void checkCuda_DriverApiError(int code, const char* file, const char* func, const int line)
{
    if (code != CUDA_SUCCESS)
    {
        ::printf("[%s][%s][%d]GpuApiCallError code = %u\n", file, func, line, code);
        //cv::error(cv::Error::GpuApiCallError, getCudaDriverApiErrorMessage(code), func, file, line);
    }
}
#define   cudaSafeCall(expr)   checkCuda_DriverApiError(expr, __FILE__, __FUNCTION__, __LINE__)

#endif // _C_VIDEO_PARSER_H_